# Focus Lamp: a robot lamp with local AI

A desk, bedside or living-room lamp that is also a small robot arm. It lights what you're doing, nudges you off your phone, helps you wind down and wake up, and **all its AI runs on the lamp itself**: no photos or data leave your home.

- **3D model:** https://claude.ai/artifact/LxxbL16V8uhb5gAGGupxjh (or open `cad/lamp-3d.html`)
- **CAD file:** `cad/lamp.scad`. Open it in OpenSCAD and change `POSE` to see each mode.

Status: **concept design**. Nothing here has been built or tested on real hardware yet. Prices are estimates from November 2026 listings.

---

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

| | Version | Estimate |
|---|---|---|
| **A** | Smart lamp, **no motion** (fixed stand, all the AI and light features) | **≈ $254–346** |
| **B** | Robot lamp, **3 joints** (turns, leans, tilts its head) | **≈ $358–473** |
| **C** | Robot lamp, **4 joints** (adds an elbow, as in the 3D model) | **≈ $386–504** |

The full parts list with per-part prices is in the 3D model's cost panel. The two big items:
- **Raspberry Pi 5, 8 GB: ≈ $125–175.** It launched at $80, but memory shortages pushed it up several times in 2026, and sources disagree on today's exact price. It's needed to run the vision model on the lamp.
- **4 smart servos (Feetech STS3215, 12 V): ≈ $28–31 each.** Cheaper hobby servos buzz and jitter, which conflicts with the no-noise requirement.

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
