# 3. Build Option B: the ESP32-CAM Budget Rover

**Time:** a weekend, about 5–8 hours for a first-timer.
**You need:** the Option B parts, a laptop on your home WiFi, and a small screwdriver.

**How this version works:** the ESP32-CAM on the rover is a tiny WiFi camera + motor controller. It runs a small web server: visit `/capture` for a photo, or `/drive?...` to move. **Your laptop runs the Claude brain** (the same Python code as Option A). It fetches a photo over WiFi, asks Claude, and tells the rover how to move.

```
   ┌─────── rover ───────┐   WiFi    ┌────── your laptop ──────┐  internet  ┌────────┐
   │ ESP32-CAM ─ TB6612  │ ◀───────▶ │ python -m rover mission │ ◀────────▶ │ Claude │
   │   camera    motors  │  photos / │   (the brain)           │            └────────┘
   └─────────────────────┘  commands └─────────────────────────┘
```

> 🛑 **Golden rule:** unplug power before changing any wire.

---

## Phase 1: Set up the Arduino IDE (laptop)

The ESP32 is programmed with the **Arduino IDE**, an app for writing and uploading C++ programs to microcontrollers.

1. Download and install **Arduino IDE 2** from https://www.arduino.cc/en/software.
2. Open it and go to **File → Preferences** (on a Mac: **Arduino IDE → Settings**). In **"Additional boards manager URLs"**, paste:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
   Click OK.
3. Open **Tools → Board → Boards Manager**, search for **esp32**, and install **"esp32 by Espressif Systems"**. The download is a few hundred MB.
4. **USB driver.** The ESP32-CAM-MB programmer uses a **CH340** USB chip. Mac and Linux usually work out of the box. On Windows, if no new COM port appears in step 2 of Phase 2, search the web for "CH340 driver", install it from the manufacturer (WCH), and replug.

## Phase 2: Upload the rover firmware

1. Plug the ESP32-CAM into its **MB** programmer board. The pins only line up one way, with the camera facing away from the USB port. Then check that the **camera ribbon** is locked into the ESP32-CAM's connector: lift the small brown/black latch, insert, and press it down.
2. Connect the MB board to your laptop with a micro-USB **data** cable. Charge-only cables won't work.
3. In the Arduino IDE, open the `firmware/esp32cam_rover/esp32cam_rover.ino` file from this project (**File → Open**).
4. Near the top of the file, set **your WiFi name and password**:
   ```cpp
   const char* WIFI_NAME     = "YOUR_WIFI_NAME";
   const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   ```
   It must be a **2.4 GHz** network. The ESP32 can't use 5 GHz.
5. Under **Tools**, set:
   - **Board:** `esp32` → **AI Thinker ESP32-CAM**
   - **Port:** the new port that appeared, e.g. `COM3` on Windows or `/dev/cu.usbserial-…` on Mac
6. Click **Upload** (the → arrow button). Wait for "Connecting…" and then a progress bar.
   - Stuck on `Connecting....____` or a "Failed to connect" error? Hold the **IO0** button on the MB board, tap **RST** on the ESP32-CAM, release IO0, then click Upload again.
7. When it says "Done uploading", open **Tools → Serial Monitor**, set the speed (bottom right) to **115200 baud**, and press the **RST** button on the ESP32-CAM. You should see:
   ```
   Rover booting...
   Connecting to WiFi.....
   Rover ready! Open this in your browser:  http://192.168.1.50
   ```
   **Write down that address**, because your laptop needs it.
8. Open that address in your laptop's browser. You should see a photo from the camera, plus arrow buttons (which do nothing yet, since no motors are connected). Refresh to get a new photo. Press the 💡 button: the bright white LED next to the camera should switch on. That's the rover's **headlight**. Press it again to switch it off. 🎉

> **Errors?** `Camera init failed` → re-seat the camera ribbon. `Brownout detector was triggered` → not enough power: try a different USB port or cable, without a hub. Never-ending dots when connecting to WiFi → wrong name or password, or a 5 GHz network.

## Phase 3: Build the chassis

Follow **[Phase 4 of the Option A guide](02-build-option-a-pi.md#phase-4-build-the-chassis)**: it's the same chassis. Mount the motors and wheels, put the battery holder on the bottom plate, and group the wires left and right.

## Phase 4: Wire it up

**Unplug the ESP32-CAM from the MB board.** The MB board covers the pins we need. (To upload new firmware later, move it back onto the MB board.)

### ESP32-CAM pins

```
         ESP32-CAM (camera side up, antenna end at the top)
              ┌───────────────────────┐
              │      [ camera ]       │
              │                       │
     5V    ●──┤                       ├──●  3V3
     GND   ●──┤                       ├──●  IO16
     IO12  ●──┤                       ├──●  IO0
     IO13  ●──┤                       ├──●  GND
     IO15  ●──┤                       ├──●  VCC (out)
     IO14  ●──┤                       ├──●  U0R
     IO2   ●──┤                       ├──●  U0T
     IO4   ●──┤                       ├──●  GND
              └───────────────────────┘
```

Check the labels printed on **your** board. They're on the underside of most boards.

### Wiring table

Put the TB6612 on the mini breadboard, straddling the centre gap (see [Option A, Phase 6a](02-build-option-a-pi.md#6a-put-the-tb6612-on-the-breadboard)). Use **female-to-male** jumpers from the ESP32-CAM pins into the breadboard rows.

| ✔ | From | → | To | Why |
|---|---|---|---|---|
| ☐ | ESP32-CAM **IO14** | → | TB6612 **AIN1** | Left motor |
| ☐ | ESP32-CAM **IO15** | → | TB6612 **AIN2** | Left motor |
| ☐ | ESP32-CAM **IO13** | → | TB6612 **BIN1** | Right motor |
| ☐ | ESP32-CAM **IO12** | → | TB6612 **BIN2** | Right motor |
| ☐ | ESP32-CAM **3V3** | → | TB6612 **VCC** | Driver logic power |
| ☐ | VCC row (M-M jumper) | → | TB6612 **STBY** | Wake the driver up |
| ☐ | VCC row (M-M jumper) | → | TB6612 **PWMA** | Left channel always enabled (full speed) |
| ☐ | VCC row (M-M jumper) | → | TB6612 **PWMB** | Right channel always enabled |
| ☐ | ESP32-CAM **GND** | → | TB6612 **GND** | Common ground |
| ☐ | Battery holder **red (+)** | → | TB6612 **VM** | Motor power |
| ☐ | Battery holder **black (−)** | → | TB6612 **GND** | Common ground |
| ☐ | Left motors (both, in parallel) | → | **AO1 / AO2** | |
| ☐ | Right motors (both, in parallel) | → | **BO1 / BO2** | |
| ☐ | Power bank USB cable **5V** (red) | → | ESP32-CAM **5V** | Powers the ESP32 |
| ☐ | Power bank USB cable **GND** (black) | → | ESP32-CAM **GND** (the other GND pin) | |

Notes:
- This version runs the motors at **full speed only** (no PWM). That keeps the wiring simple. Moves are short pulses, and the laptop decides how long each one lasts.
- **GPIO12** is a "boot strapping" pin: if it's HIGH while the ESP32 starts, it won't boot. The TB6612 normally holds it low. If your rover won't boot with the motors wired but boots without them, unplug the IO12 wire while powering on, then plug it back in.
- Mount the ESP32-CAM at the **front**, camera facing forward and tilted slightly down. Foam tape works well. Keep the antenna end (top) away from the batteries and motors for better WiFi.

### Test the motors

1. Lift the rover so the wheels are in the air.
2. Switch on the motor batteries and plug in the power bank.
3. Wait about 10 seconds, then open the rover's address in your browser and press the arrow buttons. Each press drives for 0.4 s.
4. If a side spins the wrong way, change its `LEFT_DIRECTION` or `RIGHT_DIRECTION` from `1` to `-1` in the `.ino` file and re-upload (on the MB board). You can also swap that motor pair's two wires on the breadboard.

> No Serial Monitor now that it's off the MB board? The address stays the same in most cases. If it ever changes, put it back on the MB board and check the Serial Monitor, or look in your router's device list for "esp32".

## Phase 5: Install the brain on your laptop

In a terminal on your laptop:

```bash
git clone https://github.com/<your-github-username>/rover.git
cd rover
python3 -m venv .venv
```
```bash
source .venv/bin/activate
```
On Windows, activate it with this instead:
```bash
.venv\Scripts\activate
```
Then install the libraries:
```bash
pip install -r requirements.txt
```

Test the connection. Replace the address with your rover's:

```bash
python -m rover check --hardware esp32 --esp32-url http://192.168.1.50
```

It saves `test_photo.jpg` and, if your API key is set, asks Claude to describe it. To avoid typing `--esp32-url` every time, put the address in `ESP32_URL` in `rover/config.py`.

Then drive it yourself on the floor:

```bash
python -m rover teleop --hardware esp32
```

And **calibrate**: the rover always runs at full speed, so your numbers will be bigger than the Pi version's:

```bash
python -m rover calibrate --hardware esp32
```

Copy the printed values into `rover/config.py`.

**Headlight:** the built-in flash LED is very bright and gets **warm** if left on for many minutes. That's fine for missions, but switch it off when you're done; the code switches it off automatically when a mission or teleop ends. Press `l` in teleop to toggle it.

> ⚠️ **No distance sensor on this version.** The ESP32-CAM has almost no free pins once the camera and motors are connected. Claude must avoid obstacles using the camera alone, so keep missions in open spaces and keep moves short (the code limits every move to at most 3 s). An easy upgrade is in [next steps](06-troubleshooting-and-next-steps.md#upgrades).

**Next → [5. Hand the wheel to Claude](05-claude-brain.md)**
