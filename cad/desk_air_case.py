"""
Desk Air enclosure: printable parts for two designs.

  Option A "Wedge": a desk-radio wedge, screen leaning back 15 deg,
                    knob and speaker grille in a column on the right.
  Option B "Float": a rounded slab hovering on a recessed plinth, screen
                    leaning back 10 deg, knob on top, speaker on the left side,
                    CO2 sensor breathing through top and back vents.

Run:  pip install manifold3d numpy  &&  python desk_air_case.py
Writes binary STL files to ./stl/ (millimetres).

Coordinates: x = width (left to right), y = depth (front 0 -> back +),
z = height (desk = 0). Plates are exported lying flat, front face down.

Numbers marked MEASURE are assumptions to check against your parts before printing.
"""
import math
import os
import numpy as np
from manifold3d import Manifold, CrossSection, JoinType, set_circular_segments

set_circular_segments(64)
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "stl")

# ---------------------------------------------------------------- parts we hold
MOD_W, MOD_H, MOD_T = 121.9, 75.6, 15.3      # CrowPanel Advance 4.3" module (Elecrow spec)
ACT_W, ACT_H = 95.5, 54.0                      # visible screen (Elecrow spec)
SCREEN_DX, SCREEN_DY = 0.0, 0.0                # MEASURE: visible screen offset from module centre
HOLE_DX, HOLE_DY = 111.5 / 2, 65.2 / 2         # M3 mounting holes (from Elecrow PCB file)
STANDOFF_H = 5.0                               # MEASURE: glass front to circuit-board front
WALL = 2.4
PLATE_T = 3.0
FIT = 0.25                                     # clearance around the front plate (visible reveal)
KNOB_HOLE_D = 7.5                              # encoder bushing (Adafruit 5880, PEC11-style)
SPK_W, SPK_H = 36.0, 26.0                      # MEASURE: speaker footprint + 1 mm (CQRobot 35 x 25)
SCD_HOLES = (20.32, 12.7)                      # Adafruit 1.0 x 0.7 in board hole spacing
USB_SLOT = (14.0, 8.0)                         # MEASURE: panel-mount USB-C extension opening
USB_SCREWS = 28.0                              # MEASURE: its screw spacing (0 to skip)


# ---------------------------------------------------------------- helpers
def box(x0, x1, y0, y1, z0, z1):
    return Manifold.cube([x1 - x0, y1 - y0, z1 - z0]).translate([x0, y0, z0])


def rrect(w, h, r):
    r = max(0.01, min(r, w / 2 - 0.01, h / 2 - 0.01))
    return CrossSection.square([w - 2 * r, h - 2 * r], center=True).offset(r, JoinType.Round)


def rbox(cx, cy, w, h, z0, z1, r):
    """Rounded box: rounded in XY, extruded z0..z1."""
    return Manifold.extrude(rrect(w, h, r), z1 - z0).translate([cx, cy, z0])


def cyl(x, y, z0, z1, d, d2=None):
    return Manifold.cylinder(z1 - z0, d / 2, (d2 if d2 else d) / 2).translate([x, y, z0])


def face_frame(tilt_deg):
    """3x4 matrix mapping plate-local (u across, v up the slope, w out of the face)
    to world, for a front face hinged on the front-bottom edge (y = 0, z = 0)."""
    t = math.radians(tilt_deg)
    s, c = math.sin(t), math.cos(t)
    return [[1, 0, 0, 0], [0, s, -c, 0], [0, c, s, 0]]


def on_face(m, tilt):
    return m.transform(face_frame(tilt))


def union(parts):
    return Manifold.batch_boolean(parts, __import__("manifold3d").OpType.Add) if len(parts) > 1 else parts[0]


def save_stl(m, name):
    os.makedirs(OUT, exist_ok=True)
    mesh = m.to_mesh()
    v = np.asarray(mesh.vert_properties)[:, :3].astype(np.float32)
    t = np.asarray(mesh.tri_verts).astype(np.int64)
    a, b, c = v[t[:, 0]], v[t[:, 1]], v[t[:, 2]]
    n = np.cross(b - a, c - a)
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
    rec = np.zeros(len(t), dtype=[("n", "<f4", 3), ("a", "<f4", 3), ("b", "<f4", 3), ("c", "<f4", 3), ("x", "<u2")])
    rec["n"], rec["a"], rec["b"], rec["c"] = n, a, b, c
    path = os.path.join(OUT, name)
    with open(path, "wb") as f:
        f.write(b"Desk Air enclosure".ljust(80, b" "))
        f.write(np.uint32(len(t)).tobytes())
        f.write(rec.tobytes())
    lo, hi = m.bounding_box()[:3], m.bounding_box()[3:]
    print(f"{name:28s} {len(t):7d} tris  size {hi[0]-lo[0]:.1f} x {hi[1]-lo[1]:.1f} x {hi[2]-lo[2]:.1f} mm  vol {m.volume()/1000:.1f} cm3  {m.status()}")
    return path


# ---------------------------------------------------------------- shared plate
def front_plate(pw, ph, pr, mod_cu, mod_cv, ledge_pts, plate_v0, extras=None):
    """Front plate in plate-local coords (u, v, w); front face at w = 0, back at w = -PLATE_T.
    pw/ph: plate size; plate spans v from plate_v0 to plate_v0 + ph, centred on u = 0."""
    plate = rbox(0, plate_v0 + ph / 2, pw, ph, -PLATE_T, 0, pr)
    cu, cv = mod_cu + SCREEN_DX, mod_cv + SCREEN_DY
    win_w, win_h = ACT_W + 1.0, ACT_H + 1.0
    cut = [rbox(cu, cv, win_w, win_h, -PLATE_T - 1, 1, 1.0)]
    cut.append(Manifold.hull(union([rbox(cu, cv, win_w, win_h, -1.6, -1.59, 1.0),
                                    rbox(cu, cv, win_w + 3.2, win_h + 3.2, 0.01, 0.6, 2.0)])))
    for sx in (-1, 1):
        for sy in (-1, 1):
            plate = plate + cyl(mod_cu + sx * HOLE_DX, mod_cv + sy * HOLE_DY, -PLATE_T - STANDOFF_H, -PLATE_T + 0.01, 6.5)
            cut.append(cyl(mod_cu + sx * HOLE_DX, mod_cv + sy * HOLE_DY, -PLATE_T - STANDOFF_H - 1, -1.0, 2.6))
    for (u, v) in ledge_pts:                    # blind holes for M2 screws coming through the ledge
        cut.append(cyl(u, v, -PLATE_T - 0.1, -PLATE_T + 2.0, 1.8))
    if extras:
        plate, more = extras(plate)
        cut += more
    return plate - union(cut)


def plate_to_print(p):
    """Flip so the front face lies on the bed (z = 0)."""
    return p.mirror([0, 0, 1])


# ---------------------------------------------------------------- Option A: Wedge
def option_a():
    TILT = 15.0
    BEZEL, COL_W = 9.0, 44.0
    W = MOD_W + 2 * BEZEL + COL_W
    HS = MOD_H + 2 * BEZEL
    D, HB = 80.0, 62.0
    t = math.radians(TILT)
    top_y, top_z = HS * math.sin(t), HS * math.cos(t)

    prof = CrossSection([[(0, 0), (D, 0), (D, HB), (top_y, top_z)]])
    to_world = [[0, 0, 1, -W / 2], [1, 0, 0, 0], [0, 1, 0, 0]]
    outer = Manifold.extrude(prof, W).transform(to_world)
    inner = Manifold.extrude(prof.offset(-WALL, JoinType.Miter), W - 2 * WALL).transform(to_world).translate([WALL, 0, 0])

    # front: pocket for the plate, ledge, through-opening
    POCKET_IN, LEDGE = 4.0, 4.0
    pocket = on_face(rbox(0, HS / 2, W - 2 * POCKET_IN, HS - 2 * POCKET_IN, -PLATE_T, 5, 3), TILT)
    through = on_face(rbox(0, HS / 2, W - 2 * (POCKET_IN + LEDGE), HS - 2 * (POCKET_IN + LEDGE), -30, 1, 2), TILT)
    ledge_pts = [(sx * (W / 2 - POCKET_IN - LEDGE / 2), v) for sx in (-1, 1) for v in (POCKET_IN + LEDGE / 2, HS - POCKET_IN - LEDGE / 2)]
    ledge_holes = [on_face(cyl(u, v, -12, 0.5, 2.2), TILT) for (u, v) in ledge_pts]

    body = outer - inner - pocket - through - union(ledge_holes)

    # CO2 sensor chamber, back top right
    cx0, cx1 = W / 2 - WALL - 48, W / 2 - WALL
    cy0, cz0 = D - 26, HB - 38
    shell = box(cx0 - 1.6, cx1, cy0 - 1.6, D, cz0 - 1.6, HB + 40)
    hollow = box(cx0, W, cy0, D + 1, cz0, HB + 50)
    chamber = (shell - hollow) ^ inner
    chamber = chamber - box(cx0 + 18, cx0 + 30, cy0 - 3, cy0 + 1, cz0, cz0 + 7)     # cable pass-through
    scx, scy = (cx0 + cx1) / 2, D - 14
    posts = union([cyl(scx + sx * SCD_HOLES[0] / 2, scy + sy * SCD_HOLES[1] / 2, cz0 - 0.5, cz0 + 4, 5.0) for sx in (-1, 1) for sy in (-1, 1)])
    post_holes = union([cyl(scx + sx * SCD_HOLES[0] / 2, scy + sy * SCD_HOLES[1] / 2, cz0, cz0 + 5, 2.2) for sx in (-1, 1) for sy in (-1, 1)])
    body = body + chamber + posts - post_holes
    vents = union([box(cx0 + 6 + i * 5.5, cx0 + 8.2 + i * 5.5, D - WALL - 1, D + 1, cz0 + 8, cz0 + 30) for i in range(7)])
    body = body - vents

    # back: USB-C panel jack
    body = body - on_back_slot(0, 14, D)
    if USB_SCREWS:
        body = body - union([Manifold.cylinder(WALL + 2, 1.6).rotate([-90, 0, 0]).translate([sx * USB_SCREWS / 2, D - WALL - 1, 14]) for sx in (-1, 1)])

    # base: washer cradles inside, foot recesses outside
    for x in (-16, 16):
        body = body + (cyl(x, D / 2, WALL - 0.1, WALL + 3, 29.2) - cyl(x, D / 2, WALL, WALL + 4, 26.0))
    for x in (-(W / 2 - 18), W / 2 - 18):
        for y in (18, D - 14):
            body = body - cyl(x, y, -0.1, 1.0, 10.5)

    # front plate
    col_cu = W / 2 - (BEZEL + COL_W) / 2
    mod_cu, mod_cv = -W / 2 + BEZEL + MOD_W / 2, BEZEL + MOD_H / 2
    pw, ph = W - 2 * POCKET_IN - 2 * FIT, HS - 2 * POCKET_IN - 2 * FIT

    def extras(plate):
        cuts = [cyl(col_cu, 30, -PLATE_T - 1, 1, KNOB_HOLE_D), cyl(col_cu, 30, -PLATE_T - 0.1, -PLATE_T + 1.0, 16)]
        for r in range(5):
            for c in range(6):
                cuts.append(cyl(col_cu - 12.5 + c * 5, 62 + r * 5, -PLATE_T - 1, 1, 2.4))
        ring = rbox(col_cu, 72, SPK_W + 3.2, SPK_H + 3.2, -PLATE_T - 5, -PLATE_T + 0.01, 2) - rbox(col_cu, 72, SPK_W, SPK_H, -PLATE_T - 6, -PLATE_T + 0.5, 1)
        return plate + ring, cuts

    plate = front_plate(pw, ph, 2.0, mod_cu, mod_cv, ledge_pts, POCKET_IN + FIT, extras)
    return body, plate, {"W": W, "HS": HS, "D": D, "tilt": TILT, "mod": (mod_cu, mod_cv)}


def on_back_slot(x, z, D):
    slot = rrect(USB_SLOT[0], USB_SLOT[1], 2.0)
    return Manifold.extrude(slot, WALL + 2).rotate([-90, 0, 0]).translate([x, D - WALL - 1, z])


# ---------------------------------------------------------------- Option B: Float
def option_b():
    TILT = 10.0
    W, D, H, R = 160.0, 70.0, 100.0, 10.0
    t = math.radians(TILT)
    HS = H / math.cos(t)
    n = (0.0, math.cos(t), -math.sin(t))

    outer = rbox(0, D / 2, W, D, 0, H, R).trim_by_plane(n, 0.0)
    inner = rbox(0, D / 2, W - 2 * WALL, D - 2 * WALL, WALL, H - WALL, R - WALL).trim_by_plane(n, WALL)

    POCKET_IN, LEDGE = 5.0, 4.0
    PW = 136.0
    pocket = on_face(rbox(0, HS / 2, PW, HS - 2 * POCKET_IN, -PLATE_T, 6, 4), TILT)
    through = on_face(rbox(0, HS / 2, PW - 2 * LEDGE, HS - 2 * (POCKET_IN + LEDGE), -30, 1, 3), TILT)
    ledge_pts = [(sx * (PW / 2 - LEDGE / 2), v) for sx in (-1, 1) for v in (POCKET_IN + LEDGE / 2, HS - POCKET_IN - LEDGE / 2)]
    ledge_holes = [on_face(cyl(u, v, -12, 0.5, 2.2), TILT) for (u, v) in ledge_pts]
    body = outer - inner - pocket - through - union(ledge_holes)

    # knob on top, right
    kx, ky = W / 2 - 28, 52.0
    body = body - cyl(kx, ky, H - WALL - 1, H + 1, KNOB_HOLE_D)

    # CO2 chamber, back top left, with top "chimney" slots and back slots
    cx0, cx1 = -W / 2 + WALL, -W / 2 + WALL + 46
    cy0, cz0 = D - 26, H - 40
    shell = box(-W, cx1 + 1.6, cy0 - 1.6, D, cz0 - 1.6, H + 10)
    hollow = box(-W - 1, cx1, cy0, D + 1, cz0, H + 20)
    chamber = (shell - hollow) ^ inner
    chamber = chamber - box(cx1 - 14, cx1 - 2, cy0 - 3, cy0 + 1, cz0, cz0 + 7)
    scx, scy = (cx0 + cx1) / 2, D - 14
    posts = union([cyl(scx + sx * SCD_HOLES[0] / 2, scy + sy * SCD_HOLES[1] / 2, cz0 - 0.5, cz0 + 4, 5.0) for sx in (-1, 1) for sy in (-1, 1)])
    post_holes = union([cyl(scx + sx * SCD_HOLES[0] / 2, scy + sy * SCD_HOLES[1] / 2, cz0, cz0 + 5, 2.2) for sx in (-1, 1) for sy in (-1, 1)])
    body = body + chamber + posts - post_holes
    top_slots = union([box(cx0 + 9 + i * 6.5, cx0 + 11.2 + i * 6.5, D - 22, D - 7, H - WALL - 1, H + 1) for i in range(5)])
    back_slots = union([box(cx0 + 9 + i * 6.5, cx0 + 11.2 + i * 6.5, D - WALL - 1, D + 1, cz0 + 8, H - 12) for i in range(5)])
    body = body - top_slots - back_slots

    # speaker grille + locator on the left side wall
    sy0, sz0 = 38.0, 40.0
    grille = union([Manifold.cylinder(WALL + 2, 1.2).rotate([0, 90, 0]).translate([-W / 2 - 1, sy0 - 10 + c * 5, sz0 - 12.5 + r * 5]) for r in range(6) for c in range(5)])
    ring = box(-W / 2 + WALL - 0.1, -W / 2 + WALL + 5, sy0 - SPK_H / 2 - 1.6, sy0 + SPK_H / 2 + 1.6, sz0 - SPK_W / 2 - 1.6, sz0 + SPK_W / 2 + 1.6) \
        - box(-W / 2, -W / 2 + WALL + 6, sy0 - SPK_H / 2, sy0 + SPK_H / 2, sz0 - SPK_W / 2, sz0 + SPK_W / 2)
    body = body - grille + ring

    # back: USB-C
    body = body - on_back_slot(0, 16, D)
    if USB_SCREWS:
        body = body - union([Manifold.cylinder(WALL + 2, 1.6).rotate([-90, 0, 0]).translate([sx * USB_SCREWS / 2, D - WALL - 1, 16]) for sx in (-1, 1)])

    # floor bosses for the plinth screws
    boss_xy = [(sx * 40, y) for sx in (-1, 1) for y in (18, D - 18)]
    for (x, y) in boss_xy:
        body = body + cyl(x, y, WALL - 0.1, WALL + 5, 7.0) - cyl(x, y, -1, WALL + 6, 2.6)

    # plinth (shadow-gap base)
    PL_H = 8.0
    plinth = rbox(0, D / 2, W - 12, D - 12, -PL_H, 0, 6)
    for x in (-16, 16):
        plinth = plinth - cyl(x, D / 2, -4, 0.1, 26.0)
    for (x, y) in boss_xy:
        plinth = plinth - cyl(x, y, -PL_H - 1, 1, 3.4) - cyl(x, y, -PL_H - 0.01, -PL_H + 1.6, 6.6, 3.4)
    for x in (-(W / 2 - 20), W / 2 - 20):
        for y in (13, D - 13):
            plinth = plinth - cyl(x, y, -PL_H - 0.1, -PL_H + 1.0, 10.5)

    # front plate (no knob or grille on this one)
    mod_cu, mod_cv = 0.0, 16 + MOD_H / 2
    pw, ph = PW - 2 * FIT, HS - 2 * POCKET_IN - 2 * FIT
    plate = front_plate(pw, ph, 4.0, mod_cu, mod_cv, ledge_pts, POCKET_IN + FIT)
    return body, plate, plinth.translate([0, 0, PL_H]), {"W": W, "HS": HS, "D": D, "H": H, "tilt": TILT, "knob": (kx, ky)}


# ---------------------------------------------------------------- knob cap
def knob_cap():
    D_OUT, HGT = 24.0, 15.0
    cap = Manifold.cylinder(HGT - 1.5, D_OUT / 2) + Manifold.cylinder(1.5, D_OUT / 2, D_OUT / 2 - 1.5).translate([0, 0, HGT - 1.5])
    for i in range(36):
        a = 2 * math.pi * i / 36
        cap = cap - cyl(math.cos(a) * (D_OUT / 2 + 0.35), math.sin(a) * (D_OUT / 2 + 0.35), 2.0, HGT + 1, 1.6)
    d_hole = Manifold.cylinder(11.0, 3.075) ^ box(-4, 4, -4, 1.6, 0, 11.0)    # 6 mm D-shaft, flat at 4.5 mm
    cap = cap - d_hole.translate([0, 0, -0.01])
    cap = cap - box(-0.6, 0.6, 3.5, 9.5, HGT - 0.8, HGT + 1)                   # pointer groove
    return cap


# ---------------------------------------------------------------- main
if __name__ == "__main__":
    a_body, a_plate, a_info = option_a()
    b_body, b_plate, b_plinth, b_info = option_b()
    knob = knob_cap()

    save_stl(a_body, "A_wedge_body.stl")
    save_stl(plate_to_print(a_plate), "A_wedge_front_plate.stl")
    save_stl(b_body, "B_float_body.stl")
    save_stl(plate_to_print(b_plate), "B_float_front_plate.stl")
    save_stl(b_plinth, "B_float_plinth.stl")
    save_stl(knob, "knob_cap.stl")
