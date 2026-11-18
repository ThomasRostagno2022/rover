# 7. What the rover can really do

This page is an honest look at what to expect **before** you build. Some numbers come from geometry and datasheets, so they're solid. Others, like how long Claude takes to answer, are **estimates you'll measure yourself** once it's running. Every mission prints the real values.

You can explore most of this visually in the [3D model](https://claude.ai/artifact/VqTWkRmQ2hFEWxdDdbjGNp): turn on **Rover's-eye view** to see roughly what Claude sees from each build.

---

## The one-paragraph summary

The rover is **slow but smart**. It understands what it's looking at: it knows a sock from a cable, can find "the red cup", reads a big sign, and notices a door is open. That kind of understanding is very hard for classic hobby robots. But it thinks for several seconds before each move, it has no map, and it sees the world from the height of a cat's paw. It's great at **"look around this room and find/check/describe something on the floor"**. It's bad at **anything fast, anything high up, and anything that needs remembering a whole house.**

---

## 1. How fast is a mission?

Every step goes: take a photo → send it to Claude → wait for the answer → move → settle.

| Piece | Time | Solid or estimate? |
|---|---|---|
| Take a photo on the Pi Zero | ~0.3–0.5 s | estimate |
| Upload the photo over WiFi | ~0.2–0.5 s | estimate |
| **Claude decides** | **~3–10 s** | **estimate: the big unknown** |
| Drive a 20 cm move | ~0.7 s at 30 cm/s | from calibration |
| Settle before the next photo | 0.3 s | set in code |
| **Total per step** | **≈ 5–12 s** | |

So the rover covers roughly **1–2.5 metres per minute** in short hops. Crossing a 5 m living room takes about **2–5 minutes and 20–40 steps**. Its wheels could do 50–68 cm/s, so the brain, not the motors, is what limits it.

**To make it faster:** try `--model claude-haiku-5-5` (much quicker answers, less clever), keep `--effort low`, and let Claude take longer moves in open space.

## 2. What can it see?

The camera has a 640 × 480 picture and a ~60° wide view.

| | Option A/B (camera 10.5 cm high, tilted 12° down) | Option C (mast, 27 cm high, tilted 18° down) |
|---|---|---|
| Nearest floor it can see | **15 cm** in front of the lens | 31 cm |
| Highest thing it sees 1 m away | 31 cm off the floor | 36 cm |
| Highest thing it sees 3 m away | 71 cm off the floor | 55 cm |

**What that means:**

- ✅ **It's excellent at floor level.** It sees under sofas, beds and tables, which is where lost things hide. It sees table and chair legs, shoes, bags, toys, pet bowls, cables and doorways.
- ❌ **It can't see on top of tables, counters or beds.** At 75 cm, a tabletop is above its view unless the rover is 3+ m away, and then it's too small to read. "Find my keys on the kitchen table" won't work.
- ❌ **It has a blind spot right in front of it.** Anything in the first 15 cm (30 cm for C) is invisible. Once the rover gets close to a small thing like a sock, it can no longer see it. Claude has to remember it, which is what `note_to_self` is for.

**How small can things be?** Here's how many pixels wide an object appears in the photo:

| Object | 1 m away | 2 m | 3 m | 5 m |
|---|---|---|---|---|
| 8 cm cup | 49 px | 24 px | 16 px | 10 px |
| 30 cm shoe | 182 px | 92 px | 61 px | 37 px |
| Text with 2 cm letters | 12 px | 6 px | 4 px | 2 px |

As a rule of thumb, Claude can recognise an object reliably once it's about 20+ px across, and can read text with letters about 10+ px tall. So it can **spot a cup from about 2–3 m, a shoe from across any room, and read a sign with 2 cm letters only from about 1 m.** To find small things, the rover must explore and get close.

**How far away is it?** With one camera, Claude judges distance from clues like size and where an object meets the floor, the way you would with one eye closed. Expect errors of maybe ±30–50%. That's why moves are short and the distance sensor is there.

## 3. What can stop it? (terrain and obstacles)

| | A / B (4WD) | C (rocker-bogie) |
|---|---|---|
| Ground clearance | ~2.3 cm (under the motors) | ~2.3 cm at the wheels, **6 cm** under the body |
| Climbs a step of | ~1.5–2 cm | **~4–5 cm** (one book) |
| Hard floors, thin rugs | ✅ | ✅ |
| Thick or shaggy carpet | ⚠️ slow; turns become unpredictable, so recalibrate on it | ✅ better |
| Door thresholds (1–2 cm) | ⚠️ maybe | ✅ |
| Cables on the floor | ⚠️ can snag a wheel | ✅ climbs over |
| Stairs, table edges | ❌ **dangerous**: see safety below | ❌ same |

**The distance sensor has gaps.** It's mounted about 6 cm high and points straight ahead, so it:
- ✅ catches walls, furniture, boxes, legs, feet
- ❌ misses low things (cables, socks, thin books), soft things (curtains, cushions, fur) and objects hit at a steep angle
- ❌ **can't detect a drop.** It looks forward, not down.

> ⚠️ **Never run missions on a table, or near open stairs.** Claude is told to avoid edges, and it usually recognises them, but "usually" isn't good enough for a fall. A **cliff sensor** (a cheap downward-facing infrared sensor, see upgrades) would fix this properly.

## 4. Lights, WiFi and batteries

- **In the dark:** the two LEDs have a narrow (~25°) beam aimed slightly down. In the 3D model's dark room you can see they light the floor about **0.3–1 m** ahead, not the far side of the room. In the dark the rover can avoid obstacles and find things nearby, but it can't survey a whole room. Brighter or wider LEDs are an easy upgrade.
- **WiFi:** the rover must stay on your home WiFi, because every step needs the internet. In a WiFi dead spot, the code stops the motors and retries every 5 seconds. It won't wander off blind.
- **Batteries:**
  - The **computer** runs about **5–8 hours** on a 5000 mAh power bank (a Pi Zero 2 W plus camera uses ~2–3 W).
  - The **motors** run about **2.5 hours of non-stop driving** on 4 NiMH AAs. Since the rover spends most of each step thinking rather than moving, that's well over **10 hours of missions**.
  - Batteries won't be your limit. **Your patience and your API budget will.**

## 5. What does an hour of driving cost?

At roughly 400 steps per hour of non-stop autonomous driving:

| Model | Per step | Per hour of driving |
|---|---|---|
| Claude Opus 5.5 (default) | ~1.6¢ | **~$6** |
| Claude Sonnet 5.5 | ~0.8¢ | ~$3 |
| Claude Haiku 5.5 | ~0.04¢ | **~$0.15** |

A typical mission is 10–40 steps, so it costs **$0.15–0.60 on Opus** or **under 2¢ on Haiku**. Before leaving it driving for long stretches, set a spending limit in the Anthropic console. An idea worth testing: Haiku for routine driving, Opus only for hard missions.

---

## What missions will work?

| Mission | Expect | Why |
|---|---|---|
| "Look around and describe this room" | ✅ great | Pure vision + turning |
| "Find the red cup / my shoe / the cat toy" (on the floor, same room) | ✅ good | Vision is strong; search by turning in 45° steps |
| "Look under the sofa and tell me what's there" | ✅ great, and a real use | Its low camera is an *advantage* here |
| "Is the dog's water bowl empty?" | ✅ good | Floor-level object, easy to judge |
| "Drive to the door" / "go to the open space" | ✅ good | Big, obvious targets |
| "Find a way around the boxes to the lander" | ✅ decent | Short moves + the reflex; best on Option C |
| "Read what the sign on the floor says" | ⚠️ only up close | Text needs to be large or within ~1 m |
| "Go into the kitchen" (another room) | ⚠️ sometimes | No map: works if the doorway is visible, and gets lost if not |
| "Go to the kitchen and come back here" | ❌ unreliable | Turning errors add up and it has no map or memory of the route |
| "Find my keys on the table" | ❌ | It can't see that high |
| "Follow me" / "chase the cat" | ❌ | 5–12 s between looks is far too slow for moving things |
| "Explore the whole house and map it" | ❌ (for now) | No map; a great *advanced* project, see guide 6 |
| Anything near stairs or on a table | ❌ unsafe | No cliff sensor |

### Where Claude makes the biggest difference

A classic hobby robot with the same parts can avoid walls and follow a line. It **can't** take an instruction in plain English, know what a "backpack" is, tell a cable from a crack in the floor, or explain what it saw. Those are the parts Claude adds. The rover's limits come from its **body**: low camera, one eye, slow steps, no map. Most of the upgrades in guide 6 target exactly these limits.

---

## Let's measure it together

The numbers above are predictions. Here's how to find the real ones. Experiment 1 needs **no hardware**, so you can do it this week.

1. **Vision test, with no hardware.** Lie your phone flat on the floor, leaning it so the camera is about 10 cm up and points slightly down. Take 10 photos around your home: under the sofa, a doorway, a cup 1 m away and 3 m away, a cable, a dark corner. Put them in `sim_images/` and run:
   ```bash
   python -m rover mission --hardware sim "Describe what you see and say how far away the nearest object is."
   ```
   Then open `report.html` and compare what Claude said with reality. This tells you, for *your* home, what Claude can and can't recognise at rover height.
2. **Speed and cost test** (after building). Run the same short mission three times with each model:
   ```bash
   python -m rover mission --hardware pi --model claude-haiku-5-5 "Turn in a full circle and describe the room."
   ```
   Compare the "s to decide" times and the cost printed at the end against the tables above.
3. **Navigation test.** Make a fixed course: start line, two obstacles, a red cup 2 m away. Run `"Find the red cup and stop 20 cm in front of it"` 5 times per model. Write down successes, steps and cost. Now you have your own benchmark, and every change you make (prompt, camera tilt, model) can be measured against it.
