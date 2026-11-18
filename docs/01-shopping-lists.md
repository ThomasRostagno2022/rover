# 1. Shopping lists

Prices are **rough US estimates** from Amazon, AliExpress, Adafruit, The Pi Hut, Pimoroni, etc. They change all the time. AliExpress is often about half the price of Amazon but takes 1–3 weeks to ship. The **search term** column tells you what to type into the shop's search box.

> 💡 **Buying tips for beginners**
> - Read the listing photos carefully. Check what's *included* (cables? screws? batteries?).
> - Many cheap modules arrive with **header pins loose in the bag**, which means you'd have to solder them. Look for "soldered", "pre-soldered" or "with headers" in the title.
> - Buying a 2-pack or 5-pack of tiny modules (sensors, drivers) often costs nearly the same as one, and gives you a spare when you break one. You will break one. Everyone does.
> - Don't buy a "Raspberry Pi starter kit". They're padded with stuff you won't use.

> 🔗 **Want direct links?** Jump to **[Buy links (US stores)](#buy-links-us-stores)** at the bottom of this page.

---

## Option A: Pi Rover (recommended) · total ≈ $100–125

| # | Part | Search term | Qty | ≈ Price | Notes |
|---|---|---|---|---|---|
| 1 | Raspberry Pi Zero 2 **WH** | `Raspberry Pi Zero 2 WH` | 1 | $18–22 | **WH = header pins pre-soldered**, so get WH, not W. The rover's computer. **Sold out?** See [If the Pi Zero 2 WH is sold out](#if-the-pi-zero-2-wh-is-sold-out). |
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
- (a) Buy one with the pins **already soldered**. The easiest is the **SparkFun "Motor Driver - Dual TB6612FNG (with Headers)"** (link below).
- (b) Solder them yourself. It's 16 joints and a perfect first soldering project: watch a "how to solder header pins" video first.
- (c) Use an **L298N** board instead (`L298N motor driver`, ~$4, screw terminals, no soldering). It has the same three control pins per motor (`ENA`/`IN1`/`IN2`), so **the code and wiring table work unchanged**: ENA plays the role of PWMA, ENB the role of PWMB. Remove the little jumper caps on ENA/ENB before connecting. Downside: it wastes about 2 V, so use **6 × AA** (a 6×AA holder) to power the motors.

③ **Distance sensor.** ⚠️ **Don't buy Adafruit's current RCWL-1601 (product 4742).** It talks over *I2C*, not the Trig/Echo pins our code uses. Get a generic **"RCWL-1601 … compatible with HC-SR04, 3V–5V"** module or an **HC-SR04P** (links below).

The classic **HC-SR04** is a 5 V part. Its "Echo" pin outputs 5 V, which can **damage** the Pi's 3.3 V pins. The **RCWL-1601** and **HC-SR04P** look nearly the same but work at 3.3 V. If you already have a plain HC-SR04, see the voltage-divider instructions in the Option A guide.

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

> 🔎 **Kit reality check (late 2026):** I searched and found **no cheap rocker-bogie kit that uses TT motors**. The ones out there are either much pricier (e.g. ServoCity's Bogie Runt Rover) or "6WD" with springs instead of a real rocker-bogie. So **route 2b (build your own) is the realistic choice** for Option C. That's more fun anyway.
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

---

## Buy links (US stores)

These links were checked in **November 2026**. Stores run out of stock and Amazon listings come and go. If a link is dead or sold out, use the **search term** from the tables above, or the backup link. Prices are what the store showed at the time, where it showed one.

**Why these stores?** Adafruit, SparkFun, Pololu and PiShop are hobby-electronics shops that sell genuine parts and document them well. Amazon is fine for generic bits like tape, batteries and wire, but check the "what's in the box" section of every listing.

### Option A (and the shared parts for B and C)

| # | Part | Buy here | Backup | Notes |
|---|---|---|---|---|
| 1 | Raspberry Pi Zero 2 **WH** | [PiShop: Pi Zero 2 W with Headers](https://www.pishop.us/product/raspberry-pi-zero-2w-with-headers/) (~$21) | [Adafruit 6008](https://www.adafruit.com/product/6008) · [SparkFun](https://www.sparkfun.com/raspberry-pi-zero-2-w-with-headers.html) · [Micro Center](https://www.microcenter.com/product/683270/raspberry-pi-raspberry-pi-zero-w-2-with-headers) · [CanaKit](https://www.canakit.com/raspberry-pi-zero-2-w.html) (choose "with pre-soldered header") | Often out of stock somewhere, so check all of them. Buy **one per customer** limits are common. |
| 2 | microSD card, 32 GB | [PiShop: SanDisk 32 GB, blank](https://www.pishop.us/product/microsd-card-32-gb-class-10-blank/) (~$15) | [Amazon search: SanDisk 32GB microSD](https://www.amazon.com/s?k=sandisk+ultra+32gb+microsd+a1) (often ~$7) | A blank card is fine: you'll put the OS on it yourself with Raspberry Pi Imager. |
| 3 | Camera (OV5647, **with Pi Zero cable**) | [Arducam 5MP OV5647 + Pi Zero cable (B0033+B0087)](https://www.arducam.com/b0033b0087-arducam-5mp-ov5647-mini-camera-video-module-with-15-pin-1-0mm-pitch-to-22-pin-0-5mm-and-15pin-to-15pin-1-0mm-ribbon-cable-for-raspberry-pi-model-a-b-b-pi-2-pi-3-pi-4-and-pi-zero.html) | [Amazon: Aoicrie OV5647 with Pi Zero ribbon](https://www.amazon.com/Aoicrie-Raspberry-Camera-Module-Compatible/dp/B07X145XM5) · [Amazon: 160° wide-angle OV5647 with Zero cable](https://www.amazon.com/Raspberry-Camera-Module-160FOV-Fisheye/dp/B083XMGSVP) | ⚠️ Some "for Pi Zero 2W" listings **don't include the Zero cable**. Check the box contents. For the wide-angle one, set `CAMERA_FOV_DEGREES = 160`. |
| 4 | 4WD chassis kit (2 acrylic layers, 4 TT motors) | [Amazon: YonPhsy 4WD 2-layer acrylic, TT 1:48](https://www.amazon.com/4WD-Robot-Chassis-Robotics-Raspberry/dp/B0G5N8P9YF) | [Amazon: 4WD smart chassis with 4 TT motors](https://www.amazon.com/Robot-Chassis-Motor-Arduino-Raspberry/dp/B07F759T89) · [Amazon search](https://www.amazon.com/s?k=4wd+robot+chassis+kit+tt+motor) | The YonPhsy plate is 260 × 150 mm, which is what the 3D model is based on. |
| 5 | TB6612FNG motor driver, **pins soldered** | [SparkFun: Dual TB6612FNG (with Headers)](https://www.sparkfun.com/sparkfun-motor-driver-dual-tb6612fng-with-headers.html) | [Amazon: SparkFun with Headers](https://www.amazon.com/SparkFun-Motor-Driver-TB6612FNG-Headers/dp/B07PV1S8HX) · [Amazon: 2-pack, pre-soldered](https://www.amazon.com/TB6612FNG-Driver-Pre%E2%80%91Soldered-Pin-Version/dp/B0HFMH9T4L) · [Pololu 713](https://www.pololu.com/product/713) ($4.95, **you solder the pins**) | For Option C, buy **two**. |
| 6 | Distance sensor, 3.3 V Trig/Echo type | [Amazon: RCWL-1601 4-pack, 3V–5V, HC-SR04 compatible](https://www.amazon.com/JESSINIE-RCWL-1601-Ultrasonic-Ranging-Compatible/dp/B0CB3YCFYY) | [Amazon: HC-SR04P 2-pack, 3V–5.5V](https://www.amazon.com/Ultrasonic-2cm-450cm-Interface-Compatible-Raspberry/dp/B0HNVHHFP6) | ⚠️ **Not** [Adafruit 4742](https://www.adafruit.com/product/4742): that one is I2C. |
| 7 | 4×AA battery holder with switch | [SparkFun PRT-12083](https://www.sparkfun.com/battery-holder-4xaa-with-cover-and-switch.html) (~$2.50) | [Pololu 1159](https://www.pololu.com/product/1159) · Adafruit 830 ([category page](https://www.adafruit.com/category/563)) | Skip if your chassis kit includes one with a switch. |
| 8 | AA batteries | [Amazon search: Eneloop AA with charger](https://www.amazon.com/s?k=eneloop+aa+4+pack+with+charger) | [Adafruit 3349: 4 alkaline AA](https://www.adafruit.com/product/3349) | NiMH (Eneloop) recommended, especially for Option C. |
| 9 | Small USB power bank | [Amazon search: slim power bank 5000mAh](https://www.amazon.com/s?k=slim+power+bank+5000mah) | [Adafruit: USB battery packs](https://www.adafruit.com/category/773) | Look for "2A" or "2.1A" output. Lighter is better. |
| 10 | Short USB-A → micro-USB cable | [Amazon search: short micro USB cable](https://www.amazon.com/s?k=short+micro+usb+cable+20cm) | | Must be micro-USB for the Pi Zero. |
| 11 | Jumper wires, F-F | [Adafruit 266: Female/Female 40 × 6"](https://www.adafruit.com/product/266) | [Amazon search: dupont jumper wires kit](https://www.amazon.com/s?k=dupont+jumper+wires+kit) | |
| 11b | Jumper wires, F-M | [Adafruit 826: Female/Male 40 × 6"](https://www.adafruit.com/product/826) ($3.95) | | Pi pins → breadboard |
| 11c | Jumper wires, M-M | [Amazon search: male to male jumper wires](https://www.amazon.com/s?k=male+to+male+jumper+wires) | | A few short ones for breadboard links |
| 12 | Mini breadboard | [Adafruit 65: Tiny Premium Breadboard](https://www.adafruit.com/product/65) | [Amazon search: mini breadboard 170](https://www.amazon.com/s?k=mini+breadboard+170) | Get 2 for Option C. |
| 13 | Foam tape, zip ties, Velcro | [Amazon search: double sided foam mounting tape](https://www.amazon.com/s?k=double+sided+foam+mounting+tape) | | Any hardware store works too. |
| 14 | White LEDs (headlights) | [Adafruit 754: Super Bright White 5mm, 25-pack](https://www.adafruit.com/product/754) ($6.95) | | 3.0 V, 20 mA, narrow ~25° beam: good headlights. |
| 15 | Resistor kit | [Amazon: ALLECIN 25-value 1/4 W kit](https://www.amazon.com/ALLECIN-1-4w-Resistor-Kit/dp/B0CQCMDF95) | [Amazon search: 1/4 watt resistor kit](https://www.amazon.com/s?k=1%2F4+watt+resistor+kit) | Needs 47 Ω, 100 Ω and 1 kΩ. |
| 16 | PN2222 transistors | [Amazon: Adafruit PN2222 10-pack](https://www.amazon.com/Adafruit-NPN-Bipolar-Transistors-PN2222/dp/B00XW2OK7M) | [Amazon: PN2222A 10-pack (onsemi)](https://www.amazon.com/Juried-Engineering-Semiconductor-PN2222-Transistors/dp/B08CGPFDD4) | Pinout **E-B-C** with the flat side facing you. |

### If the Pi Zero 2 WH is sold out

In 2026 the Pi Zero 2 W is often out of stock everywhere. Raspberry Pi has said demand is very high and chip-packaging shortages, caused by the AI hardware boom, limit production. **You don't have to wait.** Here are your choices, best first:

| Choice | Price | Code changes? | What's different |
|---|---|---|---|
| **1. Raspberry Pi 3 Model A+** ⭐ | ~$25 | **None** | Same 40 pins, already soldered, so all wiring tables still apply. A bit bigger (65 × 56 mm) and uses a bit more power. Uses the **standard** camera cable, not the Zero one. In production until at least 2030. |
| 2. Pi Zero 2 W *without* header + a hammer-in header | ~$15 + ~$8 | None | No soldering: you tap the pins in with a hammer and a jig. Fiddly but works. Only worth it if the no-header version is in stock. |
| 3. Raspberry Pi 4 Model B (1 or 2 GB) | ~$45–60 | None | Works, but bigger (85 × 56 mm), pricier, and needs a power bank with **USB-C, 3 A** output. |
| 4. Switch to **Option B** (ESP32-CAM) | ~$15 for 2 | — | No Raspberry Pi at all; your laptop is the brain. |

**Where to buy the Pi 3 A+:** [PiShop](https://www.pishop.us/product/raspberry-pi-3-model-a-plus-512mb-ram/) · [Adafruit 4027](https://www.adafruit.com/product/4027) · [SparkFun](https://www.sparkfun.com/raspberry-pi-3-a.html) · [CanaKit](https://www.canakit.com/raspberry-pi-3-model-a-plus.html) · [Micro Center](https://www.microcenter.com/product/514076/raspberry-pi-3-model-a-board) · Europe: [Kubii](https://www.kubii.com/en/nano-computers/2334-raspberry-pi-3-a-models-3272496313453.html) (~€28.50) · UK: [The Pi Hut](https://thepihut.com/products/raspberry-pi-3-model-a-plus) · all official sellers: [raspberrypi.com](https://www.raspberrypi.com/products/raspberry-pi-3-model-a-plus/)

**If you go with the Pi 3 A+, change these two things:**
1. **Camera cable:** you need the **standard 15-pin to 15-pin** ribbon, *not* the narrow Pi Zero one. The [Arducam kit](https://www.arducam.com/b0033b0087-arducam-5mp-ov5647-mini-camera-video-module-with-15-pin-1-0mm-pitch-to-22-pin-0-5mm-and-15pin-to-15pin-1-0mm-ribbon-cable-for-raspberry-pi-model-a-b-b-pi-2-pi-3-pi-4-and-pi-zero.html) includes **both** cables, so it works either way. Avoid cameras sold as "for Pi Zero only".
2. **Power:** it still uses micro-USB, but wants a bit more current. Use a power bank with **2.4 A** (or more) output.

**Hammer-in header (choice 2):** [Pimoroni GPIO Hammer Header kit](https://shop.pimoroni.com/collections/raspberry-pi/products/gpio-hammer-header) (get the version *with the jig*) · [PiShop (male header)](https://www.pishop.us/product/hammer-header-male-solderless-raspberry-pi-connector/)

**Raspberry Pi 3 Model B+** (~$35–45) is often in stock when the smaller boards aren't. When I searched, these sellers listed it as in stock: [SparkFun (US)](https://www.sparkfun.com/raspberry-pi-3-b.html) · [RS (Ireland/EU)](https://ie.rs-online.com/web/p/raspberry-pi/1373331) · [SOS Electronic (EU)](https://soselectronic.com/en-fr/products/raspberry-pi/raspberry-pi-3-model-b-1-292268?settings=1). It fits the top plate, takes the standard camera cable and works with the code unchanged. Every official seller in your country is listed under **Buy** on [raspberrypi.com](https://www.raspberrypi.com/products/raspberry-pi-3-model-b-plus/).

**Any Raspberry Pi from the Pi 3 onward works with this project, with no code changes:** Pi Zero 2 W, Pi 3 A+, Pi 3 B, Pi 3 B+, Pi 4, Pi 5. That makes a **used board** a good option: check eBay, Facebook Marketplace, or local classifieds (e.g. leboncoin in France). Ask the seller to confirm it boots. Notes:
- **Pi 3 B / 3 B+:** bigger (85 × 56 mm) but fits the top plate; standard camera cable; micro-USB power, 2.5 A.
- **Pi 5:** uses the *narrow* camera cable (like the Zero); wants a 5 A USB-C supply, but runs this rover fine on a 3 A power bank because nothing is plugged into its USB ports.
- ❌ **Not** the original Pi Zero / Zero W (version 1): too slow, and the Claude library doesn't install well on it.

If no Pi is available at a price you like, **build Option B now.** ESP32-CAM boards are sold by many sellers and are rarely all sold out. Everything else in your cart (chassis, motor driver, batteries, wires, power bank) is the same.

**Get alerted when Pi Zeros come back:** [rpilocator.com](https://rpilocator.com) tracks stock at official sellers and can notify you.

### Amazon-only cart (everything from one store)

Every part is available on Amazon (amazon.com). These links were found in **November 2026**. Amazon listings change often and this guide can't check live stock, so if one says "Currently unavailable", use the search link next to it.

⚠️ **About the Raspberry Pi on Amazon:** Amazon doesn't sell Raspberry Pis directly. Third-party sellers do, often at a markup, and some only sell "kits" padded with extras: I saw Pi Zero 2 W kits at **$125–150**, about 6× the real price. **Don't pay more than about $30 for a Pi Zero 2 W, $35 for a Pi 3 A+, or $75 for a Pi 4 (2 GB).** Buy the bare board, not a kit.

| Part | Amazon link | Backup: Amazon search |
|---|---|---|
| **Computer, first choice:** Pi Zero 2 W with soldered header | [Waveshare Pi Zero 2 W, pre-soldered header](https://www.amazon.com/Raspberry-Pre-Soldered-Quad-Core-Cortex-A53-Bluetooth/dp/B09LTCF8DD) · [Pi Zero 2 WH, colour-coded header](https://www.amazon.com/Zero-Pre-Soldered-Color-Coded-Quad-Core-Bluetooth/dp/B0DS68NPGF) | [search](https://www.amazon.com/s?k=raspberry+pi+zero+2+w+pre-soldered+header) |
| **Computer, or:** Pi 3 Model A+ | [Waveshare Pi 3 A+](https://www.amazon.com/Waveshare-Raspberry-Pi-Model-Cortex-A53/dp/B07KPCF2B6) · [Pi 3 A+ board](https://www.amazon.com/Raspberry-Pi-3-Computer-Board/dp/B07KKBCXLY) | [search](https://www.amazon.com/s?k=raspberry+pi+3+model+a%2B) |
| **Computer, or:** Pi 4 Model B 2 GB | [Pi 4 Model B 2 GB](https://www.amazon.com/Raspberry-Model-2019-Quad-Bluetooth/dp/B07TD42S27) | [search](https://www.amazon.com/s?k=raspberry+pi+4+model+b+2gb) |
| **Or skip the Pi:** ESP32-CAM 2-pack (Option B) | [AITRIP ESP32-CAM-MB](https://www.amazon.com/AITRIP-ESP32-CAM-ESP32-CAM-MB-Compatible-Raspberry/dp/B097BLT24K) · [Aideepen](https://www.amazon.com/ESP32-CAM-MB-Aideepen-ESP32-CAM-Bluetooth-Arduino/dp/B0948ZFTQZ) | [search](https://www.amazon.com/s?k=esp32-cam-mb+2+pack) |
| microSD 32 GB | — | [SanDisk Ultra 32GB](https://www.amazon.com/s?k=sandisk+ultra+32gb+microsd+a1) |
| Camera + cable | [Aoicrie OV5647 (Pi Zero ribbon + standard cable)](https://www.amazon.com/Aoicrie-Raspberry-Camera-Module-Compatible/dp/B07X145XM5) | [search](https://www.amazon.com/s?k=ov5647+camera+pi+zero+cable) |
| 4WD chassis | [YonPhsy 2-layer acrylic, TT motors](https://www.amazon.com/4WD-Robot-Chassis-Robotics-Raspberry/dp/B0G5N8P9YF) · [4WD chassis, 4 TT motors](https://www.amazon.com/Robot-Chassis-Motor-Arduino-Raspberry/dp/B07F759T89) | [search](https://www.amazon.com/s?k=4wd+robot+chassis+kit+tt+motor) |
| Motor driver, pre-soldered | [SparkFun TB6612FNG with Headers](https://www.amazon.com/SparkFun-Motor-Driver-TB6612FNG-Headers/dp/B07PV1S8HX) · [2-pack pre-soldered](https://www.amazon.com/TB6612FNG-Driver-Pre%E2%80%91Soldered-Pin-Version/dp/B0HFMH9T4L) | [search](https://www.amazon.com/s?k=tb6612fng+pre-soldered) |
| Distance sensor (3–5 V) | [RCWL-1601 4-pack](https://www.amazon.com/JESSINIE-RCWL-1601-Ultrasonic-Ranging-Compatible/dp/B0CB3YCFYY) · [HC-SR04P 2-pack](https://www.amazon.com/Ultrasonic-2cm-450cm-Interface-Compatible-Raspberry/dp/B0HNVHHFP6) | [search](https://www.amazon.com/s?k=rcwl-1601+3v) |
| 4×AA holder with switch | [Ogrmar 2-pack, switch + cover](https://www.amazon.com/Ogrmar-Switch-Battery-Holder-Leads/dp/B075G8XZLM) · [Onwon 2-pack](https://www.amazon.com/Onwon-Battery-Holder-Leads-Switch/dp/B01DEYV00W) | [search](https://www.amazon.com/s?k=4+aa+battery+holder+switch+leads) |
| AA batteries | — | [Eneloop AA + charger](https://www.amazon.com/s?k=eneloop+aa+4+pack+with+charger) |
| Power bank (≥ 2.4 A; 3 A USB-C for a Pi 4) | — | [slim 5000 mAh](https://www.amazon.com/s?k=slim+power+bank+5000mah) |
| USB cable, short | — | [micro-USB](https://www.amazon.com/s?k=short+micro+usb+cable+20cm) · [USB-C (Pi 4)](https://www.amazon.com/s?k=short+usb+c+cable+20cm) |
| Jumper wires (M-F, M-M, F-F) | [ELEGOO 120 pcs, 3 types](https://www.amazon.com/Elegoo-EL-CP-004-Multicolored-Breadboard-arduino/dp/B01EV70C78) · [20 cm, 3 types](https://www.amazon.com/120Pcs-Breadboards-Jumper-Female-Female/dp/B07H84JQKX) | [search](https://www.amazon.com/s?k=dupont+jumper+wires+kit) |
| Mini breadboards | [DaFuRui 12-pack, sticky back](https://www.amazon.com/DaFuRui-tie-Points-Solderless-Breadboard-Compatible/dp/B07KGQ7H8B) | [search](https://www.amazon.com/s?k=mini+breadboard+170) |
| White LEDs | [CHANZON 100× 5 mm white, 3 V 20 mA](https://www.amazon.com/Transparent-Intensity-Lighting-Electronics-Components/dp/B01AUI4VQU) | [search](https://www.amazon.com/s?k=5mm+white+led+100+pack) |
| Resistor kit | [ALLECIN 25 values](https://www.amazon.com/ALLECIN-1-4w-Resistor-Kit/dp/B0CQCMDF95) | [search](https://www.amazon.com/s?k=1%2F4+watt+resistor+kit) |
| PN2222 transistors | [Adafruit PN2222 10-pack](https://www.amazon.com/Adafruit-NPN-Bipolar-Transistors-PN2222/dp/B00XW2OK7M) · **sold out? use 2N3904:** [microtivity 10-pack](https://www.amazon.com/microtivity-Bipolar-Amplifier-Switching-Transistors/dp/B0092CRKDC) · [100-pack](https://www.amazon.com/2N3904-Transistors-Switching-Amplifier-Components/dp/B0GQMT6VRC) | [search 2N3904](https://www.amazon.com/s?k=2n3904+transistor) |
| Foam tape, zip ties | — | [foam mounting tape](https://www.amazon.com/s?k=double+sided+foam+mounting+tape) |
| Option C: extra TT motors + wheels | [Stemedu 4 sets](https://www.amazon.com/Gearbox-Motor-200RPM-Wheel-Arduino/dp/B07R13MLJD) | [search](https://www.amazon.com/s?k=tt+motor+65mm+wheel+4+sets) |

**Not in the US?** The same search terms work on amazon.ca, amazon.co.uk, amazon.fr, amazon.de and others; just copy the text after `k=` in a search link.

### Option B extras

| Part | Buy here | Backup | Notes |
|---|---|---|---|
| ESP32-CAM + MB programmer, 2-pack | [Amazon: AITRIP 2× ESP32-CAM-MB with OV2640](https://www.amazon.com/AITRIP-ESP32-CAM-ESP32-CAM-MB-Compatible-Raspberry/dp/B097BLT24K) | [Amazon: Aideepen 2× ESP32-CAM-MB](https://www.amazon.com/ESP32-CAM-MB-Aideepen-ESP32-CAM-Bluetooth-Arduino/dp/B0948ZFTQZ) | Micro-USB versions match the guide. |
| USB → jumper power cable | [Amazon search: USB to dupont cable 5V](https://www.amazon.com/s?k=usb+to+dupont+cable+5v) | | Power bank → ESP32-CAM 5V/GND |

### Option C extras (build-your-own rocker-bogie)

| Part | Buy here | Notes |
|---|---|---|
| TT motors + 65 mm wheels, 4 sets | [Amazon: Stemedu 4 sets TT motor + wheel](https://www.amazon.com/Gearbox-Motor-200RPM-Wheel-Arduino/dp/B07R13MLJD) | Buy **two** packs (8 motors, 6 used, 2 spares), or one pack + the 4WD kit's motors. |
| Second TB6612 + second mini breadboard | see rows 5 and 12 | |
| Foam board / plywood, M4 bolts + nylon lock-nuts | any craft or hardware store | |

### Price estimate for Option A (rough ranges, not live prices)

| | Low | High |
|---|---|---|
| Pi Zero 2 WH | $20 | $22 |
| microSD | $7 | $15 |
| Camera | $10 | $15 |
| Chassis | $15 | $22 |
| Motor driver (pre-soldered) | $6 | $12 |
| Distance sensor (multi-pack) | $7 | $10 |
| Battery holder + AA batteries | $8 | $22 |
| Power bank + cable | $13 | $18 |
| Wires, breadboard, tape | $14 | $18 |
| Headlights (LEDs, resistors, transistors) | $13 | $18 |
| **Total** | **≈ $113** | **≈ $172** |

> 💰 **Honest note:** buying everything from US hobby shops lands **above** the earlier ~$125 estimate, mainly because you pay for whole packs (25 LEDs, 600 resistors, 4 sensors) when you need only a few. You keep the leftovers for future projects. To stay **under $125**: buy alkaline AAs instead of Eneloop + charger, skip the resistor kit (use one LED with the 47 Ω resistor from any electronics shop), and buy the generic parts on AliExpress if you can wait 2–3 weeks.
