# 1. Shopping lists

Prices are **rough US estimates** from Amazon, AliExpress, Adafruit, The Pi Hut, Pimoroni, etc. They change all the time. AliExpress is often about half the price of Amazon but takes 1–3 weeks to ship. The **search term** column tells you what to type into the shop's search box.

> 💡 **Buying tips for beginners**
> - Read the listing photos carefully. Check what's *included* (cables? screws? batteries?).
> - Many cheap modules arrive with **header pins loose in the bag**, which means you'd have to solder them. Look for "soldered", "pre-soldered" or "with headers" in the title.
> - Buying a 2-pack or 5-pack of tiny modules (sensors, drivers) often costs nearly the same as one, and gives you a spare when you break one. You will break one. Everyone does.
> - Don't buy a "Raspberry Pi starter kit". They're padded with stuff you won't use.

---

## Option A: Pi Rover (recommended) · total ≈ $100–125

| # | Part | Search term | Qty | ≈ Price | Notes |
|---|---|---|---|---|---|
| 1 | Raspberry Pi Zero 2 **WH** | `Raspberry Pi Zero 2 WH` | 1 | $18–22 | **WH = header pins pre-soldered**, so get WH, not W. The rover's computer. |
| 2 | microSD card, 32 GB | `32GB microSD A1 class 10` (SanDisk, Samsung) | 1 | $6–9 | Holds the operating system. Avoid no-name cards. |
| 3 | Camera for Pi Zero | `Raspberry Pi Zero camera OV5647` | 1 | $10–15 | **Must say "Zero"** or include a Zero cable. The Pi Zero uses a *narrower* camera connector than other Pis. See note ① |
| 4 | Robot car chassis | `4WD smart robot car chassis kit TT motor` | 1 | $15–22 | Should include 2 acrylic plates, 4 yellow "TT" gear motors, 4 wheels, screws, spacers, and a battery holder. Prefer motors **with wires already soldered**. |
| 5 | Motor driver | `TB6612FNG motor driver module` | 1–2 | $4–8 | See note ② about soldering |
| 6 | Distance sensor, **3.3 V** type | `RCWL-1601` or `HC-SR04P` | 1 | $2–4 | See note ③. Often sold in 5-packs. |
| 7 | AA battery holder, 4×AA, with switch | `4 AA battery holder with switch` | 1 | $2–4 | Powers the **motors**. Skip it if the chassis kit includes one. |
| 8 | 4 AA batteries | NiMH rechargeable (Eneloop or similar) + charger, **or** alkaline | 4 | $5 (alkaline) / $15–20 (NiMH + charger) | NiMH is better long-term: more current, rechargeable. |
| 9 | Small USB power bank | `power bank 5000mAh slim` (≥ 2 A output) | 1 | $10–15 | Powers the **Pi**. Small and light is best. |
| 10 | USB-A → micro-USB cable, short | `short micro USB cable 20cm` | 1 | $3 | Power bank → Pi |
| 11 | Jumper wire kit | `dupont jumper wires male female 40 pin` (M-M, M-F, F-F) | 1 | $6–7 | You'll mostly use **F-F** (female-female). |
| 12 | Mini breadboard | `mini breadboard 170 tie points` | 1 | $3–5 | For sharing the 3.3 V and GND connections. Usually sticky-backed. |
| 13 | Mounting bits | double-sided foam tape, zip ties, small Velcro strips | — | $5 | |
| 14 | Headlights: white LEDs | `5mm white LED` (a pack of 20+) | 2 | $2–3 | Lets the rover see in the dark |
| 15 | Headlights: resistor kit | `resistor assortment kit` | 1 | $5–7 | You need 100 Ω and 1 kΩ (and 47 Ω for the simple version). Handy forever. |
| 16 | Headlights: NPN transistor | `PN2222A transistor` | 1 | $2–3 (pack) | An electronic switch so the Pi can power *bright* LEDs. See the Option A guide, Phase 8. |
| | | | | **≈ $100–125** | |

**Optional but useful:**
- `soldering iron kit` ($15–25). Only needed if your driver board comes with loose pins (note ②). Also a great skill to learn.
- `USB microSD card reader` ($5), if your laptop has no SD slot.
- `multimeter` ($10–15) for measuring voltage and checking connections. This is the #1 debugging tool in electronics.

**Notes:**

① **Camera.** The cheap OV5647 "Pi Zero camera" is fine. If you want sharper photos, the official **Raspberry Pi Camera Module 3** (~$25) works too, but buy a `Raspberry Pi Zero camera cable` (~$3) with it. A **wide-angle** camera (~120° field of view) helps the rover see more at once. If you get one, set `CAMERA_FOV_DEGREES = 120` in `rover/config.py`.

② **Motor driver soldering.** Many TB6612FNG boards ship with the header pins *loose*. You have three choices:
- (a) Find a listing that says the pins are **soldered** (search `TB6612FNG soldered`).
- (b) Solder them yourself. It's 16 joints and a perfect first soldering project: watch a "how to solder header pins" video first.
- (c) Use an **L298N** board instead (`L298N motor driver`, ~$4, screw terminals, no soldering). It has the same three control pins per motor (`ENA`/`IN1`/`IN2`), so **the code and wiring table work unchanged**: ENA plays the role of PWMA, ENB the role of PWMB. Remove the little jumper caps on ENA/ENB before connecting. Downside: it wastes about 2 V, so use **6 × AA** (a 6×AA holder) to power the motors.

③ **Distance sensor.** The classic **HC-SR04** is a 5 V part. Its "Echo" pin outputs 5 V, which can **damage** the Pi's 3.3 V pins. The **RCWL-1601** and **HC-SR04P** look nearly the same but work at 3.3 V. If you already have a plain HC-SR04, see the voltage-divider instructions in the Option A guide.

---

## Option B: ESP32-CAM Budget Rover · total ≈ $55–75

| # | Part | Search term | Qty | ≈ Price | Notes |
|---|---|---|---|---|---|
| 1 | ESP32-CAM **with MB programmer board** | `ESP32-CAM MB` or `ESP32-CAM with ESP32-CAM-MB` | 1 (2-pack recommended) | $10–18 | Comes with the OV2640 camera. The "MB" is a little USB board it plugs into for programming. **Make sure it's the "AI-Thinker" version** (almost all are). |
| 2 | Robot car chassis | `4WD smart robot car chassis kit TT motor` | 1 | $15–22 | Same as Option A |
| 3 | Motor driver | `TB6612FNG motor driver module` (or L298N, see Option A note ②) | 1 | $4–8 | |
| 4 | 4×AA holder with switch + batteries | as Option A | 1 | $7–20 | Motor power |
| 5 | Small USB power bank | `power bank 5000mAh slim` | 1 | $10–15 | ESP32 power: keeps it separate from noisy motors |
| 6 | USB → jumper wire power cable | `USB to dupont cable` or `USB male breakout board` | 1 | $3–5 | Gets 5 V from the power bank into the ESP32-CAM's 5V and GND pins |
| 7 | Jumper wire kit | as Option A | 1 | $6–7 | |
| 8 | Micro-USB **data** cable | (you probably have one) | 1 | $0–3 | For programming from your laptop. Some cables are charge-only and won't work! |
| — | Headlight | *nothing to buy* | — | $0 | The ESP32-CAM has a bright white "flash" LED built in, next to the camera |
| | | | | **≈ $55–75** | |

You also need **a laptop** (Windows, Mac or Linux) that stays switched on and on the same WiFi as the rover while it drives. The laptop runs the brain.

> ⚠️ The ESP32-CAM is cheap partly because quality control is poor. Getting a 2-pack is good insurance.

---

## Option C: 6-Wheel "Moon" Rover · total ≈ $130–150

Option C uses **all of Option A's electronics**, *except the 4WD chassis*, plus:

| # | Part | Search term | Qty | ≈ Price | Notes |
|---|---|---|---|---|---|
| — | Everything in Option A except item 4 (chassis), including the headlight parts | | | ≈ $85–105 | |
| 1 | A **second** TB6612FNG | (buy the 2-pack in Option A) | 1 | $0–5 | Six motors need more current than one board can give |
| 2a | **Either:** 6-wheel rocker-bogie chassis kit **with 3–6 V motors** | `rocker bogie 6WD robot chassis TT motor` | 1 | $40–60 | ⚠️ Check the motor voltage. **Avoid kits with 12 V motors** (they need a different battery and driver). |
| 2b | **Or:** build your own rocker-bogie (the guide gives a design) | `TT motor with wheel` 6-pack + foam board/plywood + M3 bolts | — | $20–30 | More work, more learning, cheaper |
| | | | | **≈ $130–150** | |

Use **NiMH** AA batteries for Option C, because six motors draw more current than alkalines handle well.

---

## Everybody: Claude API credit

| Item | Where | Cost |
|---|---|---|
| Anthropic API account + prepaid credit | https://console.anthropic.com | Start with **$5–10** |

See [docs/05-claude-brain.md](05-claude-brain.md) for how far that goes (short answer: a lot of missions).

Next:
- Building **Option A** → [2. Build the Pi Rover](02-build-option-a-pi.md)
- Building **Option B** → [3. Build the ESP32-CAM Rover](03-build-option-b-esp32.md)
- Building **Option C** → do Option A's guide but read [4. Six-wheel rover](04-option-c-six-wheel.md) first
- Waiting for parts? Try the [simulator](../README.md#try-claude-driving-before-your-parts-arrive) now!
