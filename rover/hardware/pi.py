"""
Option A / C body: Raspberry Pi + Pi camera + TB6612FNG motor driver
(+ optional ultrasonic distance sensor).

Libraries used (both come pre-installed on Raspberry Pi OS):
  * gpiozero  -- turns GPIO pins on/off and makes PWM for motor speed
  * picamera2 -- talks to the Pi camera
"""

from __future__ import annotations

import io
import time

from .. import config
from .base import Rover


class PiRover(Rover):
    name = "raspberry-pi"

    def __init__(self, use_camera: bool = True):
        # Imported here so the rest of the project still works on a laptop
        # where these Pi-only libraries are not installed.
        from gpiozero import Motor

        # A gpiozero Motor drives one channel of the TB6612:
        #   forward pin  -> AIN1/BIN1
        #   backward pin -> AIN2/BIN2
        #   enable pin   -> PWMA/PWMB (held high = channel switched on)
        # Speed is controlled by PWM (switching very fast on/off) on the
        # forward/backward pins.
        self.left = Motor(forward=config.LEFT_MOTOR_IN1,
                          backward=config.LEFT_MOTOR_IN2,
                          enable=config.LEFT_MOTOR_PWM, pwm=True)
        self.right = Motor(forward=config.RIGHT_MOTOR_IN1,
                           backward=config.RIGHT_MOTOR_IN2,
                           enable=config.RIGHT_MOTOR_PWM, pwm=True)

        self.sensor = None
        if config.USE_DISTANCE_SENSOR:
            from gpiozero import DistanceSensor
            self.sensor = DistanceSensor(echo=config.ULTRASONIC_ECHO,
                                         trigger=config.ULTRASONIC_TRIGGER,
                                         max_distance=2.0)

        # Headlights: one GPIO pin switches the LED(s) on and off.
        self.headlight = None
        if config.USE_HEADLIGHTS:
            from gpiozero import LED
            self.headlight = LED(config.HEADLIGHT_PIN)
            self.has_lights = True

        self.camera = None
        self.usbcam = None
        if use_camera and config.CAMERA_TYPE == "usb":
            self._start_usb_camera()
        elif use_camera:
            from picamera2 import Picamera2
            from libcamera import Transform

            flip = 1 if config.CAMERA_UPSIDE_DOWN else 0
            self.camera = Picamera2()
            cam_config = self.camera.create_still_configuration(
                main={"size": (config.IMAGE_WIDTH, config.IMAGE_HEIGHT)},
                transform=Transform(hflip=flip, vflip=flip),
            )
            self.camera.configure(cam_config)
            self.camera.start()
            time.sleep(1.5)  # let the camera adjust exposure / white balance

    def _lights(self, on: bool) -> None:
        if on:
            self.headlight.on()
        else:
            self.headlight.off()

    def _start_usb_camera(self) -> None:
        """Open a USB webcam with OpenCV (a popular computer-vision library)."""
        try:
            import cv2
        except ImportError:
            raise SystemExit(
                "CAMERA_TYPE is \"usb\" but OpenCV isn't installed. Run:\n"
                "    sudo apt install -y python3-opencv"
            )
        self._cv2 = cv2
        cam = cv2.VideoCapture(config.USB_CAMERA_INDEX, cv2.CAP_V4L2)
        if not cam.isOpened():
            raise SystemExit(
                f"No USB webcam found at index {config.USB_CAMERA_INDEX}. Is it "
                "plugged in? Run `ls /dev/video*` to see the cameras the Pi "
                "can find, and try USB_CAMERA_INDEX = 1 or 2 in config.py."
            )
        cam.set(cv2.CAP_PROP_FRAME_WIDTH, config.IMAGE_WIDTH)
        cam.set(cv2.CAP_PROP_FRAME_HEIGHT, config.IMAGE_HEIGHT)
        cam.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        self.usbcam = cam
        time.sleep(1.5)  # let the webcam adjust exposure / white balance

    def _capture_usb_jpeg(self) -> bytes:
        cv2 = self._cv2
        # Webcams keep a few old frames queued up. Throw them away so the
        # photo shows what's in front of the rover NOW, not before the move.
        for _ in range(4):
            self.usbcam.grab()
        ok, frame = self.usbcam.read()
        if not ok or frame is None:
            raise RuntimeError("The USB webcam didn't return a picture. Try unplugging it and plugging it back in.")
        h, w = frame.shape[:2]
        if (w, h) != (config.IMAGE_WIDTH, config.IMAGE_HEIGHT):
            frame = cv2.resize(frame, (config.IMAGE_WIDTH, config.IMAGE_HEIGHT))
        if config.CAMERA_UPSIDE_DOWN:
            frame = cv2.rotate(frame, cv2.ROTATE_180)
        ok, buf = cv2.imencode(".jpg", frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
        if not ok:
            raise RuntimeError("Couldn't convert the webcam picture to JPEG.")
        return buf.tobytes()

    def capture_jpeg(self) -> bytes:
        if self.usbcam is not None:
            return self._capture_usb_jpeg()
        if self.camera is None:
            raise RuntimeError("Camera was not started")
        buf = io.BytesIO()
        self.camera.capture_file(buf, format="jpeg")
        return buf.getvalue()

    def distance_cm(self) -> float | None:
        if self.sensor is None:
            return None
        # gpiozero reports metres, capped at max_distance.
        return self.sensor.distance * 100

    @staticmethod
    def _apply(motor, value: float, reversed_: bool) -> None:
        if reversed_:
            value = -value
        value = max(-1.0, min(1.0, value))
        if value > 0:
            motor.forward(value)
        elif value < 0:
            motor.backward(-value)
        else:
            motor.stop()

    def set_motors(self, left: float, right: float) -> None:
        self._apply(self.left, left, config.LEFT_MOTOR_REVERSED)
        self._apply(self.right, right, config.RIGHT_MOTOR_REVERSED)

    def close(self) -> None:
        super().close()  # stops motors and switches the lights off
        if self.headlight is not None:
            self.headlight.close()
        if self.usbcam is not None:
            self.usbcam.release()
        if self.camera is not None:
            self.camera.stop()
            self.camera.close()
        self.left.close()
        self.right.close()
        if self.sensor is not None:
            self.sensor.close()
