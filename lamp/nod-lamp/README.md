# Mini Nod Lamp: build plan and shopping list

A small bedside lamp, about 18–20 cm tall, with a personality. Two quiet motors let it **turn** and **tilt**, so it can look at you, nod, shake its head, droop and perk up. Simple things run on the lamp itself. Claude handles the things that need understanding: your messages, your calendar and your sleep.

- **No soldering.** Everything connects with screw terminals, lever connectors and plug-in wires.
- **No 3D printing.** The body is a craft-store wooden box, a lazy-susan bearing, a dowel and a small aluminium tin.
- **Budget:** about **$95–125** for parts, plus about $45 if you add the always-on battery in stage 9. Prices are estimates.
- **Time:** about 10 sessions of 1–3 hours. Each session ends with something you can see working.

3D concept of the bigger version: [`nod-lamp-3d.html`](nod-lamp-3d.html). The mini version uses the same idea with a turning base added.

---

## How it works

```
   YOUR PHONE (Telegram)         CALENDAR        WATCH SLEEP SUMMARY
            │                       │                    │
            ▼                       ▼                    ▼
   ┌──────────────────── THE BRAIN (Python) ────────────────────┐
   │  runs on your laptop at first, later on the Raspberry Pi   │
   │  Claude + tools: set_light · move_head · gesture ·         │
   │                  add_rule · read_calendar · read_sleep ·   │
   │                  send_message                              │
   └──────────────────────────┬─────────────────────────────────┘
                              │ Wi-Fi (simple web commands)
                              ▼
   ┌──────────────── THE LAMP (ESP32) ──────────────────────────┐
   │  Reflexes, instant and offline: sunrise · night walk ·     │
   │  touch · the rules you taught it · gestures                │
   │  Parts: LEDs · pan motor · tilt motor · 2 motion sensors · │
   │         touch pad                                          │
   └────────────────────────────────────────────────────────────┘
```

**Why split it?** The lamp has to react instantly and keep working if the internet is down. That's what the ESP32 is for. Talking to Claude, Telegram and your calendar is much easier from Python on a computer that's always plugged in. The Pi 3 B+ is perfect for this, so it finally gets a job. Until then, your laptop works.

---

## Shopping list

### Electronics

| # | Part | What to search for | Qty | ~Price | Notes |
|---|---|---|---|---|---|
| 1 | ESP32 board **and** a matching screw-terminal breakout board | "ESP32 DevKitC 38 pin with screw terminal breakout board" | 1 set | $12–20 | Buy them **as a bundle** so the widths match. Headers must come pre-soldered. |
| 2 | Stepper motors + driver boards | "ELEGOO 28BYJ-48 ULN2003 5 sets" | 1 pack (uses 2) | ~$14–15 | 5 V geared motors. Slow and fairly quiet. The gears hold position when power is off. |
| 3 | Addressable RGBW LED strip, 5 V, 60 LEDs/m, with JST-SM connectors | "SK6812 RGBW 5V 60 LED JST" (pick the "warm white" W option) | 1 m | $12–20 | Cut 8–10 LEDs **from the end with the connector**, so nothing needs soldering. The rest are spares. |
| 4 | JST-SM 3-pin pigtails | "JST SM 3 pin connector pigtail" | 1 pack | $6–8 | Plugs into the strip. The other end has bare wires for screw terminals. |
| 5 | Mini motion sensors | "AM312 mini PIR" | 1 pack (uses 2) | ~$9–13 | Work at 3.3 V. Left and right sensors tell the lamp which side you're on. |
| 6 | Jumper wires | "Dupont jumper wires female to female and male to female" | 1 kit | $6–8 | Connect the driver boards and sensors. |
| 7 | Lever connectors | "Wago 221 lever connectors assortment" | 1 small pack | $8–12 | Join the 5 V and ground wires without soldering. |
| 8 | USB-C to screw-terminal adapter | "USB C female breakout screw terminal" | 1–2 | $5–8 | Brings 5 V into the lamp. |
| 9 | 5 V 3 A USB-C wall charger + cable | any phone charger you have | 1 | $0–12 | Powers it during the build. |
| 10 | Copper foil tape | "copper foil tape conductive adhesive" | 1 roll | $6–8 | The touch pad on top of the base. |

### Body (craft or hardware store, or Amazon)

| # | Part | What to search for | Qty | ~Price | Notes |
|---|---|---|---|---|---|
| 11 | Unfinished wooden box with lid, about 4 × 4 × 2.5 in | "unfinished wood box with lid 4 inch" | 1 | $5–10 | The base. Electronics inside, pan motor under the lid. |
| 12 | Lazy-susan bearing, 3–4 in | "lazy susan bearing 3 inch" | 1 | $5–8 | Turntable for the top. Takes the weight off the pan motor. |
| 13 | Wooden disc, 3–4 in | "wood circles 3.5 inch unfinished" | 1 pack | $5–8 | Sits on the bearing and carries the neck. |
| 14 | 5 mm flange shaft couplers | "5mm flange coupling shaft coupler" | 2 | $6–9 | Connect each motor shaft to the disc and to the head. |
| 15 | Neck: 1/2 in wooden dowel or aluminium tube, about 15 cm | hardware store | 1 | $2–5 | Wires run alongside it, or inside if it's a tube. |
| 16 | Head: small aluminium tin, about 2.5 in wide | "2 oz round aluminum tin with lid" | 1 pack | $6–9 | The lamp shade. LEDs inside. |
| 17 | Diffuser: frosted acrylic sheet or vellum paper | "frosted acrylic sheet thin" or "vellum paper" | 1 | $5–8 | Cut a circle to cover the tin's opening. |
| 18 | Small screws, nuts and an L-bracket | "M3 screw nut assortment" + "small L bracket" | 1 each | $8–12 | Mount the motors. |

**Electronics + body: about $95–125.** You'll have spare motors, LEDs and sensors for experiments.

### Stage 9 only: battery

| Part | What to search for | ~Price | Notes |
|---|---|---|---|
| Voltaic V25 power bank | "Voltaic V25" | ~$45 | It has an **always-on mode**, so it doesn't switch itself off when the lamp draws very little power. Normal power banks often do. Use its USB-A output for always-on. |

### Tools

These are tools you probably already have, or can get cheaply:
- drill with 3, 6 and 10 mm bits, and a 3/4–1 in spade bit for the wire hole
- small hand saw
- screwdrivers, including a small one for the screw terminals
- hot glue gun
- wire stripper
- **multimeter**, about $15. It's the most useful tool for finding wiring mistakes.
- sandpaper, and wood oil if you want a nice finish

### Software (all free)

- **Arduino IDE 2**, with the ESP32 board package and the libraries *Adafruit NeoPixel*, *AccelStepper* and *ArduinoJson*.
- **Python 3.11+**, with `anthropic`, `python-telegram-bot` and `requests`. Later you'll add the Google Calendar libraries.
- **Telegram** on your phone, to talk to the lamp.
- A Claude API key, which you already need for the rover.

---

## Wiring plan

All power is 5 V from the USB-C adapter, shared through a lever connector. **All grounds connect together.** The ESP32's pins only send signals. Never power motors or LEDs from the ESP32's 3.3 V pin.

| From | To | Wire |
|---|---|---|
| USB-C adapter 5 V | lever connector "5V" → ESP32 `5V`/`VIN`, both driver boards `+`, LED strip `+5V` | red |
| USB-C adapter GND | lever connector "GND" → ESP32 `GND`, both driver boards `−`, LED strip `GND`, both motion sensors `GND` | black |
| ESP32 GPIO **16** | LED strip **data** (via the JST pigtail) | green or white |
| ESP32 GPIO **25, 26, 27, 14** | **Pan** driver IN1, IN2, IN3, IN4 | jumper wires |
| ESP32 GPIO **18, 19, 21, 22** | **Tilt** driver IN1, IN2, IN3, IN4 | jumper wires |
| ESP32 GPIO **34** | Left motion sensor OUT (its VCC to ESP32 `3V3`) | jumper |
| ESP32 GPIO **35** | Right motion sensor OUT (its VCC to ESP32 `3V3`) | jumper |
| ESP32 GPIO **4** (touch) | Copper tape pad on top of the base | any wire |

Wire colours on LED strips vary by brand. **Check the labels printed on the strip** (5V, DIN, GND) before connecting.

---

## Build plan: 10 sessions

Each session ends with a small "it works!" moment.

### 1. Hello lamp (1 h)
- Install Arduino IDE 2 and the ESP32 board package.
- Plug in the ESP32 and upload the "Blink" example.
- **Demo:** the little LED on the board blinks. Your toolchain works.

### 2. First light (1–2 h)
- Cut 8–10 LEDs from the strip's connector end.
- Wire power and data through the screw terminals.
- Upload a test that fades from warm to cool white, then plays a 30-second "sunrise".
- **Demo:** a sunrise on your desk.

### 3. First nod (1–2 h)
- Connect one motor and its driver board.
- Run it with AccelStepper: small moves, back and forth, at different speeds.
- Learn about steps per degree. The 28BYJ-48 has about 4,096 steps per turn in half-step mode.
- **Demo:** the bare motor shaft "nods".

### 4. Build the body (2–3 h, power tools)
- **Base box:** drill a 10 mm hole in the lid centre for the pan motor shaft, and a larger hole for wires. Mount the pan motor under the lid with M3 screws.
- **Turntable:** screw the lazy-susan bearing to the lid. Screw the wooden disc onto the bearing. Attach the disc to the pan motor shaft with a flange coupler, so the motor turns the disc and the bearing carries the weight.
- **Neck:** glue and screw the dowel or tube into the disc, slightly off-centre toward the back.
- **Tilt motor:** mount it on an L-bracket at the top of the neck, with its shaft sideways.
- **Head:** drill a 3 mm hole pattern in the side of the tin and attach it to the tilt shaft with the second flange coupler. Stick the LEDs inside in a small circle, and glue the diffuser over the opening.
- **Wires:** run them down the neck through the wire hole. Leave a little slack loop, because the top turns.
- **Demo:** it looks like a lamp.

### 5. Look around (1–2 h)
- Wire both motors.
- Write a "pose" function, `pose(pan°, tilt°, speed)`, and a homing routine. At power-on the lamp gently presses the head against its rest position and calls that zero.
- Add limits so it can't twist its wires: pan only within ±120°.
- **Demo:** it looks left, right, up and down on command.

### 6. Senses (1–2 h)
- Mount the two motion sensors on the front of the box, angled slightly left and right.
- Add the copper-tape touch pad on top.
- Logic:
  - motion on one side → turn toward that side;
  - motion at night → dim amber "night walk" glow;
  - tap → reading light on or off.
- **Demo:** it turns to look at you when you walk by.

### 7. Teach it to act (2 h, the fun one)
- Build a gesture library as keyframes, like animation: `nod`, `shake`, `droop`, `perk_up`, `look_at_door`, `curious_tilt`.
- Each gesture is a short list of `(pan, tilt, speed, light)` steps. You tune the timing until it feels alive.
- **Demo:** a little performance on demand.

### 8. Wi-Fi commands (1–2 h)
- The lamp joins your Wi-Fi and serves a small web API:
  - `/light?kelvin=3000&brightness=40`
  - `/pose?pan=30&tilt=-20`
  - `/gesture?name=nod`
  - `/rule` for adding a scheduled behaviour
- It gets the time from the internet so the sunrise runs on schedule.
- **Demo:** control it from your laptop's browser.

### 9. Claude joins (2–3 h)
- A Python "brain" on your laptop, later the Pi.
- Claude gets the lamp's abilities as **tools**, plus a short **personality prompt** that you write.
- Start in the terminal: type *"partner's asleep, I want to read 20 more minutes"*. Claude calls `set_light` and `move_head`, schedules the fade-out, and answers with a `gesture`.
- Then add a **Telegram bot**, so you text the lamp from your phone.
- **Demo:** you text it, and it answers with a nod and a message.

### 10. It knows your tomorrow, and the battery (2–3 h)
- **Calendar:** each evening the brain reads tomorrow's first event and asks Claude to plan the sunrise and wind-down. Claude explains the plan in one line on Telegram.
- **Sleep:** an iPhone Shortcut sends last night's sleep total from the Health app to the brain each morning.
- **Battery:** move power to the Voltaic V25 in the base. Turn off motor power between moves and dim the board's Wi-Fi activity. Measure how many days a charge lasts.
- **Demo:** "Your first meeting moved to 10:00. Sunrise at 7:45." The lamp acts on it with no cable.

---

## Honest notes

- **The motors are slow.** The 28BYJ-48 tops out around 10–15 RPM. A full pan takes 2–3 seconds, so gestures will be graceful rather than snappy. For a bedside lamp that's a feature, but it's also the first thing to upgrade if you want more life.
- **"Quiet" is relative.** The sellers claim under 40 dB. Expect a soft hum while moving, and silence when still, because the power is cut between moves.
- **Light quality** is good tunable white plus colour, not museum-grade high-CRI white. It's fine for bedside reading.
- **Data line:** the ESP32 sends 3.3 V signals to a 5 V strip. With a short wire this almost always works. If the LEDs flicker, we add a cheap level-shifter, which may need soldering.
- **Claude takes 1–3 seconds** to answer, so it handles messages and planning. Anything that must be instant stays on the lamp.
- **Privacy:** only text goes to Claude: your messages, event times and sleep totals. There's no camera and no microphone.
