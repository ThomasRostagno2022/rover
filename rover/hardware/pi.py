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

        self.camera = None
        if use_camera:
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

    def capture_jpeg(self) -> bytes:
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
        self.stop()
        if self.camera is not None:
            self.camera.stop()
            self.camera.close()
        self.left.close()
        self.right.close()
        if self.sensor is not None:
            self.sensor.close()
