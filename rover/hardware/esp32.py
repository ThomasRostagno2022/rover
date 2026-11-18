"""
Option B body: an ESP32-CAM on the rover, controlled over WiFi.

The ESP32 runs the small program in firmware/esp32cam_rover/. It is a tiny
web server with these addresses:
    /capture                  -> returns a JPEG photo
    /drive?l=1&r=1&ms=500     -> runs left/right wheels (-1, 0 or 1) for ms
    /stop                     -> stops the motors

This Python file runs on your LAPTOP and simply visits those addresses.
Claude is called from the laptop too, so your API key never lives on the robot.

Only the Python standard library is used here (urllib), no extra installs.
"""

from __future__ import annotations

import time
import urllib.request

from .. import config
from .base import Rover


class Esp32Rover(Rover):
    name = "esp32-cam"

    def __init__(self, base_url: str | None = None):
        self.base_url = (base_url or config.ESP32_URL).rstrip("/")
        # Quick hello so we fail early with a helpful message.
        try:
            self._get("/stop", timeout=5)
        except OSError as e:
            raise SystemExit(
                f"Could not reach the rover at {self.base_url} ({e}).\n"
                "Is it switched on, on the same WiFi as this computer, and is "
                "the address right? (It is printed on the Arduino Serial "
                "Monitor when the ESP32 boots.)"
            ) from e

    def _get(self, path: str, timeout: float = 10) -> bytes:
        with urllib.request.urlopen(self.base_url + path, timeout=timeout) as r:
            return r.read()

    def capture_jpeg(self) -> bytes:
        return self._get("/capture")

    # The ESP32 version has no distance sensor (the ESP32-CAM has almost no
    # spare pins), so distance_cm() stays None from the base class.

    def set_motors(self, left: float, right: float) -> None:
        # The ESP32 handles timing itself, see run_for below. A plain
        # set_motors(0, 0) is a stop.
        if left == 0 and right == 0:
            self._get("/stop")
        else:
            raise NotImplementedError("Use run_for() with the ESP32 rover")

    def run_for(self, left: float, right: float, seconds: float,
                watch_obstacles: bool = False) -> str:
        # The ESP32 firmware only does full on / off per wheel (no speed
        # control) to keep the wiring simple, so we send the direction only.
        def sign(x: float) -> int:
            return (x > 0) - (x < 0)

        ms = int(max(0.0, min(seconds, 3.0)) * 1000)
        # The ESP32 drives for `ms`, stops by itself, then answers. Even if
        # WiFi drops mid-move the rover still stops on its own.
        self._get(f"/drive?l={sign(left)}&r={sign(right)}&ms={ms}",
                  timeout=ms / 1000 + 10)
        time.sleep(0.2)  # let the rover settle so the next photo isn't blurry
        return "ok"
