"""
The common "shape" every rover body must have.

The brain (Claude) never talks to motors directly. It talks to a Rover object
that has these methods. That way the exact same brain drives:
  * a Raspberry Pi rover          (hardware/pi.py)
  * an ESP32-CAM rover over WiFi  (hardware/esp32.py)
  * a pretend rover on your laptop (hardware/sim.py)

This idea -- hiding the details of hardware behind a simple interface -- is
called a "hardware abstraction layer" and every real robot uses one.
"""

from __future__ import annotations

import time

from .. import config


class Rover:
    """Base class. Subclasses fill in the hardware-specific bits."""

    name = "rover"
    has_lights = False   # set to True by bodies that have headlights
    lights_on = False    # current state of the headlights

    # ---- things each body must implement ---------------------------------
    def capture_jpeg(self) -> bytes:
        """Take a photo and return it as JPEG bytes."""
        raise NotImplementedError

    def distance_cm(self) -> float | None:
        """Distance to the nearest thing in front, or None if no sensor."""
        return None

    def set_motors(self, left: float, right: float) -> None:
        """Set wheel power from -1.0 (full reverse) to 1.0 (full forward)."""
        raise NotImplementedError

    def stop(self) -> None:
        self.set_motors(0, 0)

    def set_lights(self, on: bool) -> str:
        """Switch the headlights on/off. Bodies with lights override _lights()."""
        if not self.has_lights:
            return "no headlights fitted"
        self._lights(on)
        self.lights_on = on
        return f"headlights {'on' if on else 'off'}"

    def _lights(self, on: bool) -> None:
        pass

    def close(self) -> None:
        self.stop()
        if self.lights_on:
            self.set_lights(False)

    # ---- movement built on top of set_motors ------------------------------
    def run_for(self, left: float, right: float, seconds: float,
                watch_obstacles: bool = False) -> str:
        """
        Run the motors for `seconds`, then stop.

        If watch_obstacles is True, check the distance sensor ~20 times a
        second and slam the brakes if something gets too close. This is the
        rover's REFLEX: it works even if Claude makes a mistake, and it is
        much faster than asking Claude (which takes a few seconds).
        """
        self.set_motors(left, right)
        end = time.monotonic() + seconds
        result = "ok"
        try:
            while time.monotonic() < end:
                if watch_obstacles:
                    d = self.distance_cm()
                    if d is not None and d < config.SAFETY_STOP_CM:
                        result = f"SAFETY STOP: obstacle at {d:.0f} cm"
                        break
                time.sleep(0.05)
        finally:
            self.stop()
        return result

    def forward(self, cm: float) -> str:
        cm = max(0.0, min(cm, config.MAX_MOVE_CM))
        d = self.distance_cm()
        if d is not None and d < config.SAFETY_STOP_CM:
            return f"REFUSED: obstacle already at {d:.0f} cm"
        s = config.DRIVE_SPEED
        return self.run_for(s, s, cm / config.CM_PER_SECOND, watch_obstacles=True)

    def backward(self, cm: float) -> str:
        cm = max(0.0, min(cm, config.MAX_MOVE_CM))
        s = config.DRIVE_SPEED
        return self.run_for(-s, -s, cm / config.CM_PER_SECOND)

    def turn_left(self, degrees: float) -> str:
        degrees = max(0.0, min(degrees, config.MAX_TURN_DEGREES))
        s = config.TURN_SPEED
        # Spin in place: left wheels backwards, right wheels forwards.
        return self.run_for(-s, s, degrees / config.DEGREES_PER_SECOND)

    def turn_right(self, degrees: float) -> str:
        degrees = max(0.0, min(degrees, config.MAX_TURN_DEGREES))
        s = config.TURN_SPEED
        return self.run_for(s, -s, degrees / config.DEGREES_PER_SECOND)

    # Let you write `with make_rover(...) as rover:` so motors always stop.
    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
        return False
