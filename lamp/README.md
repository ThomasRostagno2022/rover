# Focus Lamp: a robot lamp with local AI

A desk, bedside or living-room lamp that is also a small robot arm. It lights what you're doing, nudges you off your phone, helps you wind down and wake up, and **all its AI runs on the lamp itself**: no photos or data leave your home.

- **3D model:** https://claude.ai/artifact/LxxbL16V8uhb5gAGGupxjh (or open `cad/lamp-3d.html`)
- **CAD file:** `cad/lamp.scad`. Open it in OpenSCAD and change `POSE` to see each mode.

Status: **concept design**. Nothing here has been built or tested on real hardware yet. Prices are estimates from November 2026 listings.

---

## Your requirements (decided)

### 1. As silent as possible
| Noise source | What we do |
|---|---|
| **Motors** | Slow moves only, and **no movement at all during sleep hours**. Version C uses **brushless gimbal motors** with smooth control (FOC), the kind used in camera stabilisers. Makers report them near-silent at low speed, but they need careful tuning. A **spring-balanced arm** (like a classic architect's lamp) means the motors barely work. |
| **Cooling fan** | The official Pi 5 cooler stays **off below 50 °C**, and a Pi 5 with a heatsink idles around 45 °C. Heavy AI jobs are scheduled for when you're out, so the fan shouldn't run at night. The threshold can be raised in settings. |
| **Light dimming** | LEDs are driven at **25 kHz or more**: above human hearing (no buzz) and flicker-free. The IEEE 1789 flicker guideline references about 3 kHz; 25 kHz+ is a common vendor recommendation. |
| **Power** | A **fanless** adapter from a good brand (cheap ones can whine), and **no relays** (they click). |
| **Controls** | A **touch strip** on the base: silent and works in the dark. No buttons, no microphone. |

### 2. Wake-up light (sunrise)
- **Starts 30 minutes before your alarm.** It begins as a barely visible deep red-amber glow, warms to soft white, and reaches **about 250 lux at your pillow** at wake time.
- **The evidence:** small lab studies found a 30-minute "artificial dawn" rising to roughly 100–300 lux improved how alert and good-humoured people *felt* after waking, compared with no light or a light switched on suddenly. They didn't find better cognitive performance. It's a modest benefit, but a real one.
- **A smooth, dark start** needs very fine dimming steps: ordinary 8-bit dimming visibly "steps" at low brightness. A small **Raspberry Pi Pico** handles the lights with fine-grained, high-frequency dimming.
- **Reliability:** the sunrise runs on the Pico, **independently of the AI computer**, so it still works if the Pi is busy, updating or restarting. Snooze with a long press on the base.
- No sound by default, and **no movement until you're awake**.

### 3. LED: good quality, with colour
Two layers:
- **Main light: tunable white, CRI 95+, 2200 K (candle-warm) to 5000 K (daylight).** High CRI means colours and skin look natural, which matters for reading. Such strips exist (for example a 95+ CRI, 2200–5000 K, 24 V strip). We'd use a short length arranged in the head, with ~1000 lumens of headroom for the sunrise.
- **Colour layer: an RGB ring** for the deep red-amber start of the sunrise, night light, focus colour and mood. Colours are TBD.
- **Why not one all-in-one RGB+white LED?** In my research, products combining RGB, warm white and CRI 95 either don't go warm enough (often 2800 K at the warm end) or quote inconsistent CRI. Two layers give better light.

### 4. Power: plugged into the wall, not a battery
- **The wake-up light needs the lamp on all night**, every night. A battery would need charging every day.
- The Pi 5 alone draws about **2–3 W idle** (reported). Running 24/7 that's roughly **25 kWh a year, a few dollars**, depending on your electricity rate.
- Batteries would add cost, weight, heat and fire-safety care for no real benefit on a desk or nightstand.
- An **RTC battery** (a tiny clock battery for the Pi 5) and the Pico keep the time and alarm correct through power cuts.
- One slim cable from a fanless wall adapter.

## What it does

### Everywhere
| Feature | How it works |
|---|---|
| **Smart light** | Warm, high-quality white LED for reading, plus an RGB ring that shows the lamp's "mood". It dims and brightens with the room using a light sensor, not the camera. |
| **Wave to control** | Wave = on/off, palm up = brighter, palm down = dimmer. Hand detection runs on the lamp. There is **no microphone**. |
| **Expressive moves** | It turns to look at you when you arrive, "stretches" at breaks, and droops to rest. Moves are slow and rare to keep it quiet. |
| **Privacy by design** | Everything is processed on the lamp. A physical camera shutter, a small "camera on" light, no microphone, and it works offline. |

### At the desk
- **Focus sessions** (25–50 min): the ring turns blue and the light follows your work.
- **Phone nudge:** if it sees a phone in your hand during a session, it slowly turns to look at you. No sound.
- **Break reminder** after a long stretch.
- **End-of-day desk reset:** one photo, then a 2-minute tidy list on your phone. Takes the lamp about 10–25 s to think.
- **Weekly focus summary:** focus time, phone pickups, reading time.

### By the bed
- **Wind-down light:** dim amber from 9 pm, lower posture.
- **Reading light** aimed at your book, not your partner's side.
- **Phone-in-bed nudge** (optional) and a **sleep timer** that fades out.
- **Sunrise wake-up:** rises and brightens over 20–30 min before your alarm.
- **Camera-off mode:** close the shutter and the light, timer and sunrise all still work.

### In the living room
- **Reading nook light** that follows the book in your lap.
- **Screen-time awareness:** phone time on the sofa shown as a gentle ring colour.
- **Weekly plant check** of a nearby plant.
- **Ambient mood light**, off when the room is empty.

---

## How the "all on the lamp" AI works

| Job | Model type | Speed on a Pi 5 | How often |
|---|---|---|---|
| Phone / hand / person detection | Small object-detection model | Several checks per second | Continuously, while someone is there |
| Desk reset, plant check | Small vision-language model (Moondream class) | ~8–25 s per image (reported) | Once a day / week |
| Weekly summary | Small text model | Slow but fine | Once a week |

These local models are **less capable than Claude**: good at "two mugs and loose papers", weaker at nuanced judgment. That's the trade-off for nothing leaving home. The lamp's settings and history live on the lamp, and you view them on your phone through a page it serves on your home WiFi only.

---

## Estimated cost

Updated after the requirements above (better LEDs, Pico controller, fanless power, RTC battery, touch strip):

| | Version | Estimate |
|---|---|---|
| **A** | Smart lamp, **no motion**: silent, all light, wake-up and AI features on a fixed stand | **≈ $304–433** |
| **B** | Robot lamp, 4 joints with **quiet servos** | **≈ $434–590** |
| **C** | Robot lamp, 4 joints with **near-silent brushless motors** and a spring-balanced arm | **≈ $442–686** |

The full parts list with per-part prices is in the 3D model's cost panel. The two big items:
- **Raspberry Pi 5, 8 GB: ≈ $125–175.** It launched at $80, but memory shortages pushed it up several times in 2026, and sources disagree on today's exact price. It's needed to run the vision model on the lamp.
- **4 smart servos (Feetech STS3215, 12 V): ≈ $28–31 each.** Cheaper hobby servos buzz and jitter, which conflicts with the no-noise requirement.
- **Brushless gimbal motors** (version C) are estimates. They vary a lot in price, and need motor drivers and tuning.

**This is well above the $50–150 per-project budget.** Ways to bring it down:
1. **Build in stages:** version A first, then add motors later. The cost is spread out and everything is reused.
2. **Fewer joints:** version B does every feature with one fewer servo.
3. **Re-check the Pi price** before buying: it has moved a lot this year. A **Pi 5 with 4 GB** might run the smallest vision model and would be cheaper, but it needs testing.
4. **Relax "all on the lamp"** for the once-a-day jobs: a Pi 3 B+ plus a home computer doing the heavy thinking saves ~$100+, and still nothing leaves home.

---

## Honest risks

- **Speed:** local vision models take seconds per photo. That's fine for daily checks, but too slow for anything instant.
- **Tipping:** an arm reaching out needs a heavy base, hence the steel plate.
- **Heat:** the Pi 5 needs its fan under AI load. The fan is quiet but not silent.
- **Colour LEDs on a Pi 5** need a specific library (the Pi 5 drives them differently from older Pis). It's solvable, but a known gotcha.
- **Noise:** servos are quiet-ish (one vendor quotes ≈ 45 dB while moving, not independently verified). Moves are slow and rare by design.
- **Camera in the bedroom:** the shutter and camera-off mode exist for that. Agree on it with your partner.

## Suggested build stages (1–3 h/week)

1. **Weeks 1–3:** Pi 5 + light + camera on a fixed stand (version A). Gesture control and the reading light.
2. **Weeks 4–6:** phone and person detection, focus sessions, ring colours, the phone dashboard.
3. **Weeks 7–10:** add the servos and the arm (version B or C), and expressive moves.
4. **Weeks 11+:** the daily desk reset with the local vision model, weekly summaries, sunrise mode.
