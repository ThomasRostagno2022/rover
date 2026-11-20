# Mini Nod Lamp: the simple version

The goal of this version is to **see it work** with as few parts as possible. No box, no turntable, no woodworking. A ready-made pan-and-tilt mount is the lamp's "neck", a few LEDs under half a ping-pong ball are its "head", and everything plugs into a breadboard.

```
        ( ping-pong ball half: the diffuser )
               ▲  8 RGBW LEDs inside
        ┌──────┴──────┐
        │ tilt servo  │   ← ready-made pan-tilt kit
        │ pan servo   │
        └──────┬──────┘
     taped to a heavy mug or a book
               │ wires
   [ breadboard + ESP32 ]──USB── phone charger
               │ Wi-Fi
   [ laptop: lamp_brain.py + Claude ]  ← you type here
```

**Cost:** about **$45–65**. **Time to a working lamp:** about three evenings.

You can try the Claude part **today, before buying anything**:

```bash
cd lamp/mini-nod/brain
pip install -r requirements.txt
python lamp_brain.py --sim
```

Type something like *"partner's asleep, I want to read 20 more minutes"*, and watch what the lamp would do.

---

## Parts

| # | Part | What to search for | ~Price |
|---|---|---|---|
| 1 | ESP32 dev board, **30-pin**, USB-C, pins pre-soldered (a 3-pack gives you spares) | "AITRIP 3PCS 30Pins USB-C ESP-WROOM-32" | $15–20 |
| 2 | Pan-tilt mount with 2 micro servos | "pan tilt kit SG90 servos" (or the assembled Adafruit Mini Pan-Tilt Kit, ~$19) | $8–19 |
| 3 | RGBW LED strip, 5 V, 60 LEDs/m, with JST connectors | "SK6812 RGBW 5V 60 LED JST" | $12–20 |
| 4 | Breadboard + jumper wires kit | "830 breadboard jumper wires kit" | $8–12 |
| 5 | USB-C to 2-pin screw-terminal adapter, plus a 5 V 2–3 A phone charger you already own | "USB C female to 2 pin screw terminal adapter" | $7–10 |
| 6 | Capacitor, 470–1000 µF, 10 V or more (keeps the servos from resetting the board) | "electrolytic capacitor assortment" | $5–8 |
| 7 | Ping-pong balls | any | $3 |
| | **Total** | | **~$45–65** |

**Optional:** an AM312 mini motion sensor for the night glow (about $9 for a pack), and kitchen foil or copper tape for the touch pad.

**Tools:** scissors for the LED strip, a craft knife for the ping-pong ball, a hot glue gun or tape, and a multimeter (about $15, optional but handy).

**Why 30-pin?** It fits a breadboard with a free row of holes on each side. Wider 38-pin boards cover every hole. All the pins the lamp uses (4, 16, 18, 19, 34) exist on both.

**Why not a breadboard power module?** The common MB102 module is rated around 700 mA, which two servos plus the LEDs can exceed. The USB-C adapter passes the charger's full current.

**No soldering.** Cut the LED strip so you keep the end **with the connector**, about 8 LEDs, and push its wires into the breadboard.

---

## Wiring

```
Phone charger ─USB-C─► screw-terminal adapter ─two jumper wires─► breadboard rails
                         + rail (5V)          − rail (GND)
ESP32 5V/VIN  ◄───────── + rail      ESP32 GND ◄──── − rail
Servo red (both)  ◄───── + rail      Servo brown (both) ◄── − rail
LED strip +5V     ◄───── + rail      LED strip GND ◄── − rail
Capacitor across + and − rails (the stripe on the capacitor goes to −)

ESP32 GPIO 18 ──► pan servo orange (signal)
ESP32 GPIO 19 ──► tilt servo orange (signal)
ESP32 GPIO 16 ──► LED strip DIN (data)
ESP32 GPIO 4  ──► a wire to a square of foil or copper tape (touch pad)
ESP32 GPIO 34 ──► motion sensor OUT (optional; sensor VCC to ESP32 3V3, GND to − rail)
```

**Rules that prevent most problems:**
- Every ground is connected together.
- Servos and LEDs get power from the 5 V rail, never from the ESP32's 3.3 V pin.
- Wire colours on LED strips vary. Read the labels printed on the strip: **5V, DIN, GND**.
- Plug the ESP32's own USB cable into your laptop only for uploading code. When it runs from the breadboard power, you can unplug the laptop.

---

## Build steps

### Evening 1: light and motion on the desk (about 2 h)
1. Install **Arduino IDE 2**. In Boards Manager, install **esp32 by Espressif**. In Library Manager, install **Adafruit NeoPixel** and **ESP32Servo**.
2. Open `firmware/mini_nod/mini_nod.ino`. Fill in your Wi-Fi name and password at the top.
3. Wire it as above, select board **ESP32 Dev Module**, and upload.
4. Open the Serial Monitor at 115200. You should see `Lamp ready at http://192.168.x.x`. The LEDs glow warm and the head nods.

**It works when:** the lamp says hello with a glow and a nod.

### Evening 2: make it a lamp (about 1–2 h)
1. Cut a ping-pong ball in half. Stick the 8 LEDs in a small ring on the pan-tilt's top plate, and glue the half-ball over them.
2. Tape or glue the pan-tilt base to something heavy: a mug full of coins, a book, a wooden block.
3. **Centre it.** Visit `http://<lamp-ip>/pose?pan=0&tilt=0` in your browser. If the head doesn't face forward and level, change `PAN_CENTER` / `TILT_CENTER` in the code and upload again. If up and down are swapped, set `TILT_SIGN = -1`.
4. Try the web API in your browser:
   - `/light?kelvin=2700&brightness=40`
   - `/gesture?name=nod`, then `shake`, `droop`, `perk_up`, `happy`, `curious`
   - `/sunrise?minutes=1`
5. Tap the foil: the reading light comes on with a nod. Tap again: off.

**It works when:** it looks and moves like a little lamp, and the tap works.

### Evening 3: Claude joins (about 1 h)
1. On your laptop:
   ```bash
   cd lamp/mini-nod/brain
   pip install -r requirements.txt
   export ANTHROPIC_API_KEY='sk-ant-...'
   python lamp_brain.py --lamp http://192.168.x.x
   ```
2. Talk to it:
   - *"partner's asleep, I want to read 20 more minutes"*
   - *"make it feel like a rainy café"*
   - *"wake me at 7:15 tomorrow, gently"*
   - *"five more minutes?"* (try it after midnight)
   - *"I'm stressed about tomorrow's presentation"*
3. Edit `brain/personality.txt` to change its character, then restart.

**It works when:** you type, and the lamp answers with light, a gesture and a message.

---

## How the pieces fit

| Where | What runs there | Why there |
|---|---|---|
| **ESP32 (in the lamp)** | Smooth fades, head moves, gestures, the daily sunrise alarm, fade-out timers, the touch pad, the night glow | Instant, and keeps working if Wi-Fi or the laptop is off |
| **Laptop, later the Pi** | `lamp_brain.py`: your messages → Claude → tool calls → lamp | Easy to change, and talking to Claude is simplest from Python |

Claude controls the lamp through 9 **tools**:
- `set_light`, `set_color`, `turn_off`: light
- `move_head`, `gesture`: body
- `start_sunrise`, `set_wake_alarm`, `fade_off_after`: time
- `get_state`: what the lamp is doing now

It decides which to use from what you type and its personality. That's the "AI agent with tools" pattern used in real products, small enough to read in one sitting.

---

## Notes

- **The servos are noisier than the stepper motors** in the full plan. Micro servos buzz a little while moving, and sometimes twitch at power-on. That's the price of simplicity. They're silent when still.
- **If the board resets when the servos move,** the power is dipping. Check the capacitor, and use a 2 A or stronger charger.
- **If the LEDs flicker or show wrong colours,** check the strip is the RGBW type. Otherwise change `NEO_GRBW` in the code to `NEO_RGBW`.
- **Cost of Claude:** each message is a small request at low effort, roughly a cent or less.
- **Next steps once it works:**
  - text it from your phone with a Telegram bot
  - let it read your calendar and plan tomorrow's sunrise
  - move the brain to the Pi so it runs all the time
  - build the nicer body from [`../nod-lamp/README.md`](../nod-lamp/README.md)
