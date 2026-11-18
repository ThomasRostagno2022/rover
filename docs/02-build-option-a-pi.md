# 2. Build Option A: the Pi Rover

**Time:** one weekend, about 6–10 hours total for a first-timer.
**You need:** the Option A parts, a laptop on your home WiFi, and a small Phillips screwdriver.

We'll do this in **7 phases**. Each one ends with a test, so if something breaks you know exactly where.

```
Phase 1  Prepare the SD card                (laptop)
Phase 2  First boot + connect over WiFi     (Pi on your desk, no wiring yet)
Phase 3  Install the rover software
Phase 4  Build the chassis                  (mechanical)
Phase 5  Camera                             → test
Phase 6  Motors + motor driver              → test  (wheels in the air!)
Phase 7  Distance sensor                    → test
Phase 8  Headlights                         → test
Then:    Calibrate, and hand the wheel to Claude (guide 5)
```

> 🛑 **Golden rule:** always **switch off / unplug power before changing any wire.** Plugging wires into a powered Pi is the #1 way beginners kill them.

---

## Phase 1: Prepare the SD card (on your laptop)

The SD card is the Pi's "hard drive". We'll put the operating system on it, *Raspberry Pi OS*, which is a version of Linux.

1. On your laptop, download and install **Raspberry Pi Imager** from https://www.raspberrypi.com/software/.
2. Put the microSD card into your laptop (use a USB card reader if needed).
3. Open Raspberry Pi Imager and choose:
   - **Device:** Raspberry Pi Zero 2 W (or **Raspberry Pi 3** if you're using a Pi 3 A+ because the Zero was sold out)
   - **Operating system:** *Raspberry Pi OS (other)* → **Raspberry Pi OS Lite (64-bit)**. "Lite" has no desktop: we control it by typing commands over WiFi, which is faster and lighter.
   - **Storage:** your SD card. Double-check it's the SD card and not another drive!
4. Click **Next**. When asked about **OS customisation / settings**, choose **Edit settings** and fill in:
   - **Hostname:** `rover`
   - **Username:** `rover` and a **password** you'll remember
   - **Wireless LAN:** your WiFi name (SSID) and password, plus your **country**. ⚠️ The Pi Zero 2 W only supports **2.4 GHz** WiFi. Most home routers broadcast both 2.4 and 5 GHz, so this usually just works.
   - **Locale:** your time zone and keyboard layout
   - **Services** tab: ✅ **Enable SSH**, with "Use password authentication"
5. Save, then click **Yes** to write. It takes about 5–10 minutes. When it says it's done, remove the card.

> The Imager's menus change a little between versions. If something is named differently, look for the same idea: device, OS, storage, then settings for hostname, user, WiFi and SSH.

---

## Phase 2: First boot and connecting over WiFi

1. Put the SD card into the Pi's slot (contacts facing the board, and it only fits one way).
2. Plug a micro-USB cable from a phone charger or your power bank into the port labelled **PWR IN**. The Pi Zero has two micro-USB ports: **PWR IN** is the one nearest the corner. The other, labelled **USB**, is for data. (**Pi 3 A+:** it has only one micro-USB port, so use that.)
3. A small green LED flickers. **Wait 3–5 minutes** the first time, because the Pi sets itself up and may reboot once.
4. On your laptop, open a terminal:
   - **Mac:** open the *Terminal* app
   - **Windows 10/11:** open *PowerShell* or *Terminal*
   - **Linux:** you know where it is 🙂
5. Type the following command and press Enter:
   ```bash
   ssh rover@rover.local
   ```
   - The first time it asks *"Are you sure you want to continue connecting?"*. Type `yes`.
   - Enter the password you chose. **Nothing appears while you type a password.** That's normal.
6. If you see a prompt like `rover@rover:~ $`, **you're in!** You are now typing commands *on the rover's computer*.

**If `rover.local` is not found:** log into your WiFi router's web page, find the device list, look for "rover", and use its IP address instead, for example `ssh rover@192.168.1.37`. Or wait two more minutes and retry. See [troubleshooting](06-troubleshooting-and-next-steps.md) for more.

To leave the SSH session later, type `exit`. To shut the Pi down safely, **always** run the following command and wait for the green LED to stop flickering before unplugging:

```bash
sudo shutdown now
```

---

## Phase 3: Install the rover software (on the Pi, via SSH)

Copy and paste these commands one block at a time. Each line starting with `sudo` asks for your password the first time.

**1. Update everything.** This takes about 10–20 minutes on a Pi Zero, so be patient:
```bash
sudo apt update && sudo apt full-upgrade -y
```

**2. Install the tools the rover needs:**
```bash
sudo apt install -y git python3-venv python3-gpiozero python3-lgpio python3-pil
```
```bash
sudo apt install -y --no-install-recommends python3-picamera2
```
What these are:
- `git` downloads code
- `python3-venv` creates private Python environments
- `gpiozero` + `lgpio` control the GPIO pins
- `pil` (Pillow) measures how dark each photo is, so Claude can decide when to use the headlights
- `picamera2` controls the camera

**3. Download this project onto the Pi.** If the project is on your GitHub, run:
```bash
cd ~
git clone https://github.com/<your-github-username>/rover.git
cd rover
```
> If your GitHub repo is **private**, the easiest way is to copy it from your laptop instead. In a terminal **on your laptop** (not the SSH one), from the folder that *contains* the `rover` folder, run:
> ```bash
> scp -r rover rover@rover.local:~/
> ```

**4. Create the Python environment and install the Claude library:**
```bash
cd ~/rover
python3 -m venv --system-site-packages .venv
source .venv/bin/activate
pip install -r requirements.txt
```
- `--system-site-packages` lets the environment use the gpiozero and picamera2 you installed with `apt`.
- `source .venv/bin/activate` "enters" the environment. Your prompt now starts with `(.venv)`. **You must run this activate line every time you log in** before running rover commands. The `pip install` can take 5–10 minutes on a Pi Zero.

**5. Shut down before wiring:**
```bash
sudo shutdown now
```

---

## Phase 4: Build the chassis

Every chassis kit is slightly different, so follow its leaflet if it has one. These are the general steps:

1. **Peel the protective paper** off both acrylic plates. It's fiddly. Use a fingernail at a corner.
2. **Mount the 4 motors** to the *bottom* plate. Each motor is held by two small acrylic or metal brackets and two long screws with nuts.
   - Put all motors so their **wires face inwards**, toward the middle of the chassis.
   - The motor shafts (the white D-shaped axles) must point **outwards**.
3. **Push the wheels** onto the D-shaped shafts. Line up the flat side. It's a firm push, so support the motor from behind while you press.
4. **Screw the brass spacers (standoffs)** into the bottom plate. The top plate will sit on them later.
5. **Group the motor wires:** gather the 2 left motors' wires on the left and the 2 right motors' wires on the right. Leave them loose for now.
6. Put the **4×AA battery holder** on the bottom plate, between the motors, with double-sided tape or the kit's screws. **Leave the batteries out for now.**
7. Don't attach the top plate yet. You'll want access to the wiring.

**Planned layout:**

```
                FRONT
     ┌────────────────────────────┐
     │  [camera]   [ultrasonic]   │  ← top plate, front edge
  ◯══│                            │══◯   wheels
     │   [Pi Zero]   [power bank] │  ← top plate
     │                            │
     │  [breadboard + TB6612]     │  ← bottom plate (between the plates)
  ◯══│  [4×AA battery holder]     │══◯
     └────────────────────────────┘
                BACK
```

---

## Phase 5: The camera → test

1. **Power off.** Find the camera connector on the Pi Zero: a thin black latch on the short end, **opposite** the SD card.
   > **Using a Pi 3 A+?** Its camera connector is the long slot labelled **CAMERA**, between the HDMI port and the audio jack. Use the standard-width (15-pin) cable, lift the latch **upwards**, and insert the cable with the **metal contacts facing the HDMI port**. Everything else in this guide is the same, including every pin number.
2. Gently **pull the black latch outwards** by about 1–2 mm with your fingernails. Don't pull hard, because it's fragile.
3. Slide the **narrow end** of the camera ribbon in, with the **shiny metal contacts facing the board** (down). Push the latch back in to lock it. Tug gently: the cable shouldn't come out.
4. Make sure the other end is locked into the camera board the same way. It usually comes pre-connected.
5. Power on, wait about 1 minute, SSH in, and run:
   ```bash
   rpicam-hello --list-cameras
   ```
   You should see your camera listed (e.g. `ov5647` or `imx708`). If it says *"No cameras available"*, power off and re-seat the ribbon. 90% of the time, the cable is in upside down or not fully in.
6. Take a test photo:
   ```bash
   rpicam-jpeg -o test.jpg --width 640 --height 480
   ```
7. Copy the photo to your laptop to look at it. Run this in a **laptop** terminal:
   ```bash
   scp rover@rover.local:~/test.jpg .
   ```
   Open `test.jpg`. Blurry? Some cheap cameras have a focus ring you can gently twist.

**Mounting the camera:** stick it to the **front edge of the top plate** with foam tape, **tilted slightly downwards** (about 10–15°). It should see the floor starting about 20–30 cm in front of the rover, plus the room beyond. If photos come out upside down, set `CAMERA_UPSIDE_DOWN = True` in `rover/config.py`.

---

## Phase 6: Motors and motor driver → test

This is the most wiring. Take it slowly and tick off each wire.

### 6a. Put the TB6612 on the breadboard

Press the TB6612 module into the mini breadboard so that it **straddles the gap in the middle**. Each pin then sits in its own row of connected holes. That gives you 4 spare holes next to every pin for plugging in wires.

> **Breadboard basics:** on each side of the centre gap, the 5 holes in a short row are connected to each other. Nothing is connected across the gap.

### 6b. Know your Pi's pins

The Pi's 40 pins are numbered in a zig-zag, starting with **pin 1**. Pin 1 is the one with a **square** solder pad on the back of the board, at the end nearest the SD card. On the Pi you can also run `pinout` to print a map.

```
            Pi Zero header (looking at the top, SD card on the LEFT)
   pin 2  4  6  8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40   ← board edge
       ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●
       ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●
   pin 1  3  5  7  9 11 13 15 17 19 21 23 25 27 29 31 33 35 37 39
```

Check this against https://pinout.xyz, which has a picture you can click on.

### 6c. Wiring table: Pi → motor driver

Use **female-to-male** jumpers: female end on the Pi pin, male end into the breadboard row of the TB6612 pin.

| ✔ | Pi physical pin | Pi name | → | TB6612 pin | What it does |
|---|---|---|---|---|---|
| ☐ | **1** | 3.3V | → | **VCC** | Powers the driver's logic chip |
| ☐ | (breadboard jumper) | VCC row | → | **STBY** | Standby. Must be HIGH, or the driver stays asleep. |
| ☐ | **6** | GND | → | **GND** | Common ground (rule #2!) |
| ☐ | **11** | GPIO17 | → | **AIN1** | Left motor direction |
| ☐ | **13** | GPIO27 | → | **AIN2** | Left motor direction |
| ☐ | **15** | GPIO22 | → | **PWMA** | Left motor on/off |
| ☐ | **16** | GPIO23 | → | **BIN1** | Right motor direction |
| ☐ | **18** | GPIO24 | → | **BIN2** | Right motor direction |
| ☐ | **22** | GPIO25 | → | **PWMB** | Right motor on/off |

For STBY, use a short **male-to-male** jumper from a spare hole in the VCC row to a spare hole in the STBY row.

### 6d. Wiring table: batteries and motors → motor driver

| ✔ | From | → | TB6612 pin |
|---|---|---|---|
| ☐ | Battery holder **red** wire (+) | → | **VM** (motor power) |
| ☐ | Battery holder **black** wire (−) | → | **GND** (any GND pin) |
| ☐ | Left-front motor wire 1 **and** left-rear motor wire 1 | → | **AO1** |
| ☐ | Left-front motor wire 2 **and** left-rear motor wire 2 | → | **AO2** |
| ☐ | Right-front motor wire 1 **and** right-rear motor wire 1 | → | **BO1** |
| ☐ | Right-front motor wire 2 **and** right-rear motor wire 2 | → | **BO2** |

- The two motors on the same side are wired **in parallel** (both wires into the same rows), so they always do the same thing.
- If a motor's wires are bare or tinned, push the ends straight into the breadboard holes. If they're too floppy, twist the two motors' matching wires together first.
- Don't worry about which motor wire is "1" and which is "2". If a wheel spins backwards, you'll fix it in software, or by swapping its two wires.

```
                      ┌───────── TB6612FNG ─────────┐
 Battery + ──────────▶│ VM                     PWMA │◀── Pi pin 15
 Pi pin 1 (3.3V) ────▶│ VCC ──┐                AIN2 │◀── Pi pin 13
 Pi pin 6 (GND) ─────▶│ GND   │                AIN1 │◀── Pi pin 11
 Battery − ──────────▶│ GND   └──────────────▶ STBY │
 Left motors ◀────────│ AO1                    BIN1 │◀── Pi pin 16
 Left motors ◀────────│ AO2                    BIN2 │◀── Pi pin 18
 Right motors ◀───────│ BO2                    PWMB │◀── Pi pin 22
 Right motors ◀───────│ BO1                     GND │
                      └─────────────────────────────┘
```

> ⚠️ **Pin order varies between TB6612 boards.** Always go by the **labels printed on your board**, not by the position in this drawing.

### 6e. Test the motors

1. **Double-check every wire against the tables.** In particular, battery **+ goes to VM** and never to VCC or the Pi.
2. Put the batteries in the holder, but keep its **switch OFF**.
3. **Lift the rover** so the wheels are off the table. Rest it on a mug or a roll of tape.
4. Power the Pi from the power bank, wait 1 minute, then SSH in.
5. Switch the battery holder **ON**.
6. Run:
   ```bash
   cd ~/rover && source .venv/bin/activate
   python -m rover check --hardware pi
   ```
   It takes a photo, reads the sensor (which isn't fitted yet, so readings will be strange, but that's fine), then asks you to press Enter and spins the **left** wheels, the **right** wheels, then **both**, each **forward**. Step 4 of the check, the Claude test, needs your API key. If you haven't set it up yet, it stops with a message there. That's fine for now.
7. **Watch each side:**
   - A side spun **backwards**? Edit `rover/config.py` with `nano rover/config.py`, set `LEFT_MOTOR_REVERSED = True` (or `RIGHT_…`), then save with Ctrl+O, Enter, and exit with Ctrl+X.
   - **Left and right are swapped** (it said "LEFT" but the right side moved)? Swap the GPIO numbers of the left and right motors in `config.py`.
   - **Nothing moved?** See [troubleshooting](06-troubleshooting-and-next-steps.md#motors-dont-move).
8. Run the check again until all three moves are correct.

---

## Phase 7: Distance sensor → test

**Power off first** (`sudo shutdown now`, and switch the battery holder off).

Mount the sensor at the **front**, facing straight ahead and level, below or next to the camera. Make sure the wheels and chassis aren't in its "view". Then wire it with **female-to-female** jumpers:

| ✔ | Sensor pin | → | Pi physical pin | Pi name |
|---|---|---|---|---|
| ☐ | **VCC** | → | **17** | 3.3V |
| ☐ | **GND** | → | **9** | GND |
| ☐ | **Trig** | → | **29** | GPIO5 |
| ☐ | **Echo** | → | **31** | GPIO6 |

> **Only have a 5 V HC-SR04?** Power it from **pin 2 (5V)** instead, and *protect the Pi* from the 5 V echo signal with two resistors (a **voltage divider**):
> ```
> Sensor Echo ──[ 1 kΩ ]──┬──▶ Pi pin 31 (GPIO6)
>                         └──[ 2 kΩ ]──▶ GND
> ```
> This turns 5 V into about 3.3 V. Don't skip it.

**Test:** power on, SSH in, then run:

```bash
cd ~/rover && source .venv/bin/activate
python -m rover check --hardware pi
```

In step 2, move your hand toward and away from the sensor. The numbers should change. A constant 200 cm means "nothing detected": check the Trig and Echo wires (they're easy to swap by mistake).

---

## Phase 8: Headlights → test

Claude can only drive as well as it can see. With headlights the rover can explore under furniture, in dim rooms and at night. During missions **Claude decides** when to switch them on and off.

**Power off first.** Then choose one of the two versions. Both use the same Pi pin and the same code.

### Why not just plug an LED into a GPIO pin?

You can, but it will be **dim**. A white LED needs about 3 V to light up, and a GPIO pin gives only 3.3 V and at most about 16 mA. That leaves almost nothing to push current through it. So the "bright" version uses a **transistor**: a tiny electronic switch. The Pi's weak signal turns the transistor on, and the transistor lets current flow from the Pi's stronger **5 V** pin through the LEDs. This is the same idea as the motor driver, in miniature. It's one of the most useful tricks in electronics.

### Version 1 (simple, dim): one LED straight from the pin

Good enough to light things up to about 30–50 cm away.

```
Pi pin 37 (GPIO26) ──[ 47 Ω ]──▶ LED long leg (+)
                                 LED short leg (−) ──▶ Pi pin 39 (GND)
```

### Version 2 (bright, recommended): two LEDs switched by a transistor

```
 Pi pin 4 (5V) ──┬──[ 100 Ω ]──▶ LED 1 long leg (+) ── LED 1 short leg (−) ──┐
                 └──[ 100 Ω ]──▶ LED 2 long leg (+) ── LED 2 short leg (−) ──┤
                                                                             ▼
                                                                  C (collector)
 Pi pin 37 (GPIO26) ──[ 1 kΩ ]──────────────────────────────────▶ B (base)      PN2222A
                                                                  E (emitter)
                                                                             │
 Pi pin 39 (GND) ◀───────────────────────────────────────────────────────────┘
```

Build it on the breadboard:

| ✔ | Connection |
|---|---|
| ☐ | Push the **transistor** into the breadboard, each leg in its own row. With the **flat side facing you**, a PN2222A's legs are **E, B, C** from left to right. A **2N3904** works exactly the same and has the same E, B, C order, so use it if PN2222s are sold out. ⚠️ Other transistor types (e.g. some "2N2222", or the BC547/BC337 common in Europe, which are C, B, E) have a different order, so check the listing or datasheet. |
| ☐ | **Pi pin 37** (GPIO26) → a **1 kΩ** resistor → the **B** (base) row |
| ☐ | **E** (emitter) row → **Pi pin 39** (GND) |
| ☐ | **Pi pin 4** (5V) → a free row (the "5 V row") |
| ☐ | For **each** LED: 5 V row → a **100 Ω** resistor → the LED's **long leg**; the LED's **short leg** → the **C** (collector) row |

**Mounting:** bend the LED legs so the LEDs point forward and slightly down, on either side of the camera. Don't put them right next to the lens, or the camera will see glare. Use extension jumper wires (F-M) to reach the front of the rover, and tape or hot-glue the LEDs in place.

**LED legs:** the **long** leg is + (anode). If the LED has been trimmed, the − side has a small flat edge on the rim. An LED put in backwards simply doesn't light; it's not damaged.

**Test:** power on, then run:

```bash
python -m rover check --hardware pi
```

Step 3b blinks the headlights 3 times. You can also press `l` in teleop to toggle them. Not working? See [troubleshooting](06-troubleshooting-and-next-steps.md#headlights-dont-light).

Don't want headlights? Set `USE_HEADLIGHTS = False` in `rover/config.py`. Claude then won't be offered the light actions.

---

## Finish the build

1. Put the **top plate** on the spacers. Mount the Pi and the power bank on top with foam tape or Velcro. Leave the Pi's micro-USB ports reachable.
2. Tidy the wires with zip ties so nothing touches the wheels.
3. Check the rover sits level and the camera and sensor have a clear view.

## Drive it yourself (teleop)

Before letting Claude drive, drive it yourself. Put it on the **floor** and run:

```bash
python -m rover teleop --hardware pi
```

Type `w`, `a`, `s` or `d` and press Enter to move, `l` to toggle the headlights, `p` to take a photo, and `q` to quit. Try driving it toward a wall: the distance sensor should stop it about 20 cm away. 🎉

## Calibrate

Claude says things like "forward 20 cm" or "turn 45°". The rover needs to know how long to run its motors to achieve that. Run:

```bash
python -m rover calibrate --hardware pi
```

Follow the prompts: measure with a tape measure, estimate the turn, and copy the two printed lines into `rover/config.py`. Do it 2–3 times and average. Redo it if you change floors (carpet vs. wood) or batteries.

**Next → [5. Hand the wheel to Claude](05-claude-brain.md)**
