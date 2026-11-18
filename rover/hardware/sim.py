"""
A pretend rover for trying the Claude brain BEFORE your parts arrive.

Put some photos (JPEG) taken from about 10 cm off the floor into the
sim_images/ folder -- e.g. walk around your room with your phone held low.
The "camera" returns those photos in order, and the "motors" just print what
they would do. No hardware needed: this runs on any laptop.
"""

from __future__ import annotations

import pathlib

from .base import Rover

DEFAULT_DIR = pathlib.Path(__file__).resolve().parents[2] / "sim_images"


class SimRover(Rover):
    name = "simulator"

    def __init__(self, image_dir: str | None = None):
        folder = pathlib.Path(image_dir) if image_dir else DEFAULT_DIR
        self.images = sorted(
            p for p in folder.glob("*") if p.suffix.lower() in (".jpg", ".jpeg")
        )
        if not self.images:
            raise SystemExit(
                f"No .jpg photos found in {folder}.\n"
                "Take a few photos from about 10 cm above the floor (like a "
                "rover would see) and put them there."
            )
        self.index = 0

    def capture_jpeg(self) -> bytes:
        path = self.images[self.index % len(self.images)]
        self.index += 1
        print(f"   [sim camera] showing {path.name}")
        return path.read_bytes()

    def set_motors(self, left: float, right: float) -> None:
        if left or right:
            print(f"   [sim motors] left={left:+.2f} right={right:+.2f}")

    def run_for(self, left: float, right: float, seconds: float,
                watch_obstacles: bool = False) -> str:
        self.set_motors(left, right)
        print(f"   [sim motors] ... for {seconds:.2f}s, then stop")
        return "ok (simulated)"
