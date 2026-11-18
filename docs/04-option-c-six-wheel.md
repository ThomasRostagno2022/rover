# 4. Option C: the 6-Wheel "Moon" Rover

Option C looks and drives like a real planetary rover. It has **six wheels on a rocker-bogie suspension**, the design used on NASA's Sojourner, Spirit, Opportunity, Curiosity and Perseverance. It can climb over obstacles up to about one wheel-diameter high while keeping all six wheels on the ground.

**The electronics and software are the same as Option A.** Only the chassis and the motor wiring change. So the plan is:

1. Read this page to understand what's different.
2. Do the **[Option A guide](02-build-option-a-pi.md)**, phases 1–3 (SD card, WiFi, software) and phase 5 (camera), exactly as written.
3. Build the 6-wheel chassis (below) in place of Option A's phase 4.
4. Do the motor wiring (below) in place of Option A's phase 6.
5. Continue with Option A's phases 7 (distance sensor) and 8 (headlights), then calibration, then [guide 5](05-claude-brain.md).

---

## How rocker-bogie suspension works

```
   side view (one side; the other side is a mirror image)

                      body pivot ●───────────── body
                               ╱   ╲
                   rocker     ╱     ╲   rocker
                             ╱       ╲
                            ╱         ● bogie pivot
                           ╱         ╱ ╲
                          ╱   bogie ╱   ╲ bogie
                       (◯)       (◯)     (◯)
                      front     middle    rear
```

- Each side has a **rocker**: a long arm that swings on a pivot attached to the body. Its front end holds the **front wheel**, and its back end holds the **bogie pivot**.
- The **bogie** is a shorter arm that swings on that pivot and holds the **middle and rear wheels**.
- There are **no springs**. Each arm tilts freely, so the wheels follow the ground.
- A **differential** connects the left and right rockers so that when one side tilts up, the other tilts down. That keeps the body at the *average* angle, which stops it flopping over and keeps the camera steadier.

## The chassis: two routes

### Route 1 (easier, if you can find one): buy a kit

> 🔎 When I searched (November 2026), I found **no cheap rocker-bogie kit with TT motors**. The real ones (e.g. ServoCity's Bogie Runt Rover) cost much more and use different motors, and cheap "6WD" kits use springs, not a rocker-bogie. Route 2 is the realistic choice, but look again: new kits appear all the time.

Search for `rocker bogie 6WD robot chassis`. **Check before buying:**
- ✅ Motors are **3–6 V "TT" or "N20" gear motors**. ❌ **Avoid 12 V motors** (often labelled "25GA" or "37GA" with "12V"). They need a bigger battery and driver, which breaks the budget.
- ✅ Includes 6 motors, 6 wheels, all the arms and pivots, and a body plate.
- ✅ Has room on top for a Pi Zero, a power bank and a mini breadboard. The Pi Zero is tiny, so most kits fit.

Assemble it following the kit's instructions, then skip to [Wiring 6 motors](#wiring-6-motors).

### Route 2 (more learning): build your own

**Materials (≈ $20–30):**
- 6 × `TT motor with wheel` (sold as sets: the same yellow gear motors and 65 mm wheels as the 4WD kits)
- One sheet of **5 mm foam board** or **3 mm plywood**, A3 size, or stiff corrugated plastic from an old sign
- A **plastic food container** about 20 × 12 × 6 cm, to use as the body. It holds all the electronics.
- 4 × M4 bolts (30 mm) with nylon lock-nuts + 8 washers (the pivots)
- 1 × M4 bolt (40 mm) + lock-nut + washers (differential pivot)
- Zip ties, a hot glue gun, a craft knife, a ruler, a drill or a sharp skewer for the holes
- 2 × paper clips or stiff wire (differential links)

**Cut list (per side, so cut 2 of each):**

| Piece | Size | Holes |
|---|---|---|
| Rocker | 24 × 3 cm strip | Body pivot at 10 cm from the front end; bogie pivot hole 2 cm from the rear end |
| Bogie | 14 × 3 cm strip | Pivot hole in the exact centre (7 cm) |
| Differential post | 6 × 2 cm strip | Small hole near the top for the wire link |

**Assembly:**

1. **Motors onto the arms.** Zip-tie (and hot-glue) one motor under the **front end** of each rocker, and one under **each end** of each bogie. The axles point outward, and the wheels must not touch anything when they spin. Use a small dab of hot glue so the motors can be removed later.
2. **Bogies onto rockers.** Bolt each bogie to its rocker's rear hole: bolt, washer, rocker, washer, bogie, lock-nut. **Tighten until snug, then back off a little.** The bogie must swing freely. This is the most important rule of the whole build.
3. **Rockers onto the body.** Make a hole in each long side wall of the food container, about 2 cm below the rim and centred front-to-back. Bolt each rocker on at its pivot hole the same way. Again, it must swing freely.
4. **Differential.** Hot-glue a **differential post** standing straight up on each rocker, right above its body pivot. Cut a **15 cm strip** (the differential bar) and bolt it at its centre to the top of the lid (or a bridge across the container) with the 40 mm bolt, so it can swing like a see-saw. Link each end of the bar to the top of the post on that side with a bent paper clip. Now press one rocker down: the other should go up, and the body should stay level-ish.
5. **Check:** put it on the floor and push it slowly over a book. All six wheels should stay on the ground and the body should tilt only gently.

Put the 4×AA holder inside the container, low down, to keep the rover from tipping. Put the Pi, the power bank and the breadboard inside or on the lid, and mount the camera and sensor at the front.

> 🎨 **Mast camera:** for the real rover look, mount the camera on a **15–20 cm "mast"** (a pencil, dowel or straw glued upright at the front). A higher camera sees further and over obstacles. Set `CAMERA_HEIGHT_CM` in `rover/config.py` to match.

---

## Wiring 6 motors

One TB6612 channel can handle about **1.2 A**, which is enough for two small motors but not three. So we use **two TB6612 boards** and **wire their control pins together**. The Pi sends the same signals to both boards, and **the software doesn't change at all**.

| Board | Channel A (left side) | Channel B (right side) |
|---|---|---|
| **TB6612 #1** | Left **front** + left **middle** motors (in parallel) | Right **front** + right **middle** motors (in parallel) |
| **TB6612 #2** | Left **rear** motor | Right **rear** motor |

**Steps:**

1. Wire **TB6612 #1 exactly as in [Option A, phase 6](02-build-option-a-pi.md#phase-6-motors--motor-driver--test)**: Pi pins → AIN1, AIN2, PWMA, BIN1, BIN2, PWMB, VCC, GND; VCC → STBY; battery → VM and GND.
2. Put **TB6612 #2** on a second mini breadboard, or on the same one if it's a bigger 400-hole board.
3. With male-to-male jumpers, connect **each control pin of board #2 to the same pin of board #1**:
   `AIN1↔AIN1`, `AIN2↔AIN2`, `PWMA↔PWMA`, `BIN1↔BIN1`, `BIN2↔BIN2`, `PWMB↔PWMB`, `VCC↔VCC`, `STBY↔STBY`, `GND↔GND`.
   That's 9 jumpers.
4. **Battery + → VM of both boards** (a jumper from board #1's VM row to board #2's VM row).
5. Connect the motors as in the table above.
6. **Test with the wheels in the air** (prop the body up on a box): `python -m rover check --hardware pi`. **All three left wheels must spin the same way**, and likewise on the right. If one wheel spins backwards, swap **that motor's** two wires.

**Batteries:** use **NiMH** AA rechargeables. Six motors drain alkalines quickly and make the Pi's motor control sluggish.

Then continue with Option A's phases 7 (distance sensor) and 8 (headlights), **calibrate** (six wheels turn more slowly than four, so your numbers will differ), and hand the wheel to Claude in **[guide 5](05-claude-brain.md)**.

**Climbing tip:** rocker-bogie works best **slowly**. Try `DRIVE_SPEED = 0.5` in `config.py`, then try missions with "obstacle courses" made of books and cables.
