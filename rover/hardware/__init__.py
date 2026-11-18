"""Pick the right rover body by name."""

from __future__ import annotations

from .base import Rover


def make_rover(kind: str, esp32_url: str | None = None,
               sim_images: str | None = None, use_camera: bool = True) -> Rover:
    if kind == "pi":
        from .pi import PiRover
        return PiRover(use_camera=use_camera)
    if kind == "esp32":
        from .esp32 import Esp32Rover
        return Esp32Rover(esp32_url)
    if kind == "sim":
        from .sim import SimRover
        return SimRover(sim_images)
    raise ValueError(f"Unknown hardware '{kind}' (use pi, esp32 or sim)")
