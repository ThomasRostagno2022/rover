# Mini Moon Rover, driven by Claude

A small wheeled robot you build yourself. It has a camera, and **Claude drives it**. You give it a mission in plain English, like *"explore the living room and find my red shoe"*. The rover takes a photo, sends it to Claude, and Claude picks the next move: "turn left 30°" or "forward 20 cm". The rover makes that move, takes another photo, and repeats until the mission is done.

```
     ┌──────────── the "sense → think → act" loop ────────────┐
     │                                                        │
 📷 SENSE              🧠 THINK                    🛞 ACT       │
 take a photo   ──▶   Claude looks at it   ──▶   motors move ──┘
 + distance            and picks ONE move          (with a safety
   reading             (as JSON)                    reflex that stops
                                                    for obstacles)
```

Real planetary rovers like Curiosity and Perseverance work the same basic way. They take pictures, plan a short drive, carry it out carefully, and look again.

**Assumed experience: none.** The guides explain every step, starting with what a volt is.

---

## Pick your build

All three options fit the **$100–150 budget**. They use the same Python "brain" code from this repo.

|  | **Option A: Pi Rover** ⭐ recommended | **Option B: ESP32-CAM Budget Rover** | **Option C: 6-Wheel "Moon" Rover** |
|---|---|---|---|
| Looks like | 4-wheel robot car | 4-wheel robot car | 6-wheel rocker-bogie, like a real Mars/Moon rover |
| Brain runs on | The rover itself (Raspberry Pi Zero 2 W) | Your laptop, over WiFi | The rover itself (Raspberry Pi Zero 2 W) |
| Obstacle reflex | ✅ ultrasonic sensor | ❌ (camera only) | ✅ ultrasonic sensor |
| Headlights | ✅ 2 white LEDs | ✅ built-in flash LED | ✅ 2 white LEDs |
| Approx. cost | **~$100–125** | **~$55–75** | **~$130–150** |
| Soldering | None | None | Maybe (depends on kit) |
| Difficulty | ★★☆ | ★★☆ (finicky to program the first time) | ★★★ (more mechanical building) |
| You learn | Linux, Python, GPIO, PWM, motor drivers, sensors, AI | Microcontrollers, Arduino/C++, WiFi, web APIs, AI | Everything in A, plus suspension mechanics |
| Fully self-contained? | Yes, it just needs WiFi | Needs your laptop nearby and switched on | Yes, it just needs WiFi |

**Not sure? Pick Option A.** It's the best mix of cost, learning and capability. Everything runs on the robot, and Linux plus Python are skills you'll use forever. If you want something bigger later, Option C reuses all of Option A's electronics.

Every option also needs about **$5–10 of Claude API credit** to start. That covers a lot of driving (see [costs](docs/05-claude-brain.md#what-does-it-cost)).

---

## The learning path

Work through these in order. Each step ends with something working that you can test.

| # | Guide | What you'll do |
|---|---|---|
| 0 | [Concepts for absolute beginners](docs/00-concepts.md) | Learn voltage, current, GPIO, PWM, H-bridges, and how an AI can drive a robot |
| 1 | [Shopping lists for all 3 options](docs/01-shopping-lists.md) | Buy the right parts, including what to search for and what to avoid |
| ★ | **Try the brain now, no hardware needed** ([below](#try-claude-driving-before-your-parts-arrive)) | Watch Claude "drive" through photos you took with your phone |
| 2 | [Build Option A: Pi Rover](docs/02-build-option-a-pi.md) | Assemble, wire, install software, add headlights, test each part |
| 3 | [Build Option B: ESP32-CAM Rover](docs/03-build-option-b-esp32.md) | Assemble, wire, flash firmware, test |
| 4 | [Build Option C: 6-Wheel Rover](docs/04-option-c-six-wheel.md) | Option A's electronics on a rocker-bogie chassis |
| 5 | [The Claude brain: missions, prompts, costs](docs/05-claude-brain.md) | Get an API key, run your first autonomous mission, and learn how the code works |
| 6 | [Troubleshooting + what to build next](docs/06-troubleshooting-and-next-steps.md) | Fix common problems, then add upgrades such as a pan camera, mapping and voice |

---

## Try Claude driving before your parts arrive

The `sim` "hardware" uses photos from a folder in place of a camera, and it only *prints* the motor commands. You can run it on any laptop with Python 3.10+:

```bash
git clone <this repo> rover && cd rover
python3 -m venv .venv
source .venv/bin/activate          # Windows: .venv\Scripts\activate
pip install -r requirements.txt
export ANTHROPIC_API_KEY='sk-ant-...'   # Windows: set ANTHROPIC_API_KEY=sk-ant-...
```

Next, take 5–10 photos with your phone held about 10 cm above the floor, moving around your room as a rover would. Put them in `sim_images/`. Then run:

```bash
python -m rover mission --hardware sim "Explore the room and find a shoe"
```

You'll see Claude describe each photo and choose a move. Open the `missions/<date>/report.html` file afterwards to review the whole run.

---

## Project layout

```
rover/                     Python package: the software
  __main__.py              commands: check, teleop, calibrate, mission
  brain.py                 talks to Claude: photo in, one move out
  config.py                ← YOUR settings (pins, speeds, calibration, model)
  hardware/
    base.py                movement + the safety reflex (shared by all bodies)
    pi.py                  Option A/C: Raspberry Pi + camera + TB6612 + ultrasonic
    esp32.py               Option B: talks to the ESP32-CAM over WiFi
    sim.py                 no hardware: photos from a folder
  vision.py                measures photo brightness (told to Claude for headlight decisions)
firmware/esp32cam_rover/   Option B: Arduino program for the ESP32-CAM
docs/                      the step-by-step guides
sim_images/                put test photos here for the simulator
missions/                  created automatically: photos + logs of each run
```

## Commands at a glance

```bash
python -m rover check     --hardware pi       # test camera, distance sensor, motors, Claude
python -m rover teleop    --hardware pi       # drive it yourself (w/a/s/d, l = lights, + Enter)
python -m rover calibrate --hardware pi       # measure cm/second and degrees/second
python -m rover mission   --hardware pi "Find the red cup and stop next to it"
```

For Option B, use `--hardware esp32 --esp32-url http://<rover-ip>`. For the simulator, use `--hardware sim`.

## Safety, in one paragraph

This is a small, slow, battery-powered toy, but treat it with respect. **Lift the wheels off the ground the first time you power the motors.** Never short battery wires together. If you use lithium cells (Option C upgrade), use a proper charger and never charge them unattended. Start missions in a clear area of floor, **never on a table**, and keep `Ctrl+C` handy: it stops the motors instantly. The rover has three layers of protection. Claude is told to be careful, the code limits every move to at most 60 cm or 180°, and the distance sensor stops it automatically near obstacles.
