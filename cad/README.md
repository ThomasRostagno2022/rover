# Desk Air enclosure

Two printable designs for the CrowPanel Advance 4.3" desk display. Renders in `renders/`, print files in `stl/`.

| | Option A: Wedge | Option B: Float |
|---|---|---|
| Look | Desk-radio wedge, screen tilted 15°, knob + speaker grille beside the screen | Rounded slab hovering on a dark plinth, screen tilted 10°, knob on top, speaker grille on the left side |
| Size (W × D × H) | 184 × 80 × 90 mm | 160 × 70 × 108 mm (with plinth) |
| Parts | `A_wedge_body`, `A_wedge_front_plate`, `knob_cap` | `B_float_body`, `B_float_front_plate`, `B_float_plinth`, `knob_cap` |

## Before printing: measure these (edit the top of `desk_air_case.py`, then run it)
- `STANDOFF_H` – distance from the front of the screen glass to the front of the circuit board (default 5 mm).
- `SCREEN_DX/DY` – if the visible screen isn't centred on the module.
- `SPK_W/H` – your speaker's footprint + 1 mm.
- `USB_SLOT`, `USB_SCREWS` – your panel-mount USB-C extension's opening and screw spacing.

Regenerate: `pip install manifold3d numpy` then `python desk_air_case.py`.

## Print settings (PLA or PETG)
- 0.2 mm layers, 3 walls, 15 % infill.
- Front plates: front face down on a textured or smooth bed for a clean finish. No supports.
- Wedge body: print standing on its back face. Float body: print upside down (top on the bed). Supports only for the USB-C slot if your slicer asks.
- Plinth (Float): as exported, no supports. Print it in black for the shadow-gap look.
- Knob cap: as exported. Print in an accent colour (orange looks great on Bone or Graphite).

## Hardware
- 4 × M3 × 6 mm screws (display board to front-plate posts)
- 4 × M2 × 6 mm screws (front plate through the body ledge) – or a few dots of glue
- 2 × M2.5 × 5 mm screws (CO2 sensor to its posts, optional)
- Float only: 4 × M3 × 10 mm countersunk screws (plinth to body)
- 2 × steel washers, 25 mm across (weight), 4 × 10 mm rubber feet
- Panel-mount USB-C extension, 20–30 cm
- STEMMA QT cable 15–20 cm (sensor sits in the back chamber)
