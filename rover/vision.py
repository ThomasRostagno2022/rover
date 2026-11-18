"""Small helpers for looking at photos in code (no AI needed)."""

from __future__ import annotations

import io


def brightness(jpeg: bytes) -> float | None:
    """
    Average brightness of a photo: 0 = pitch black, 255 = pure white.

    We shrink the photo to a tiny grey thumbnail first, which makes this fast
    even on a Raspberry Pi Zero. Returns None if Pillow (the image library)
    isn't installed, so the rover still works without it.
    """
    try:
        from PIL import Image, ImageStat
    except ImportError:
        return None
    with Image.open(io.BytesIO(jpeg)) as img:
        small = img.convert("L").resize((64, 48))
        return ImageStat.Stat(small).mean[0]
