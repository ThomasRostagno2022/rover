# 0. Concepts for absolute beginners

Read this once before you buy or build anything. It's about 20 minutes, and it'll make everything later make sense. Don't worry about memorizing it; come back when a word confuses you.

---

## Part 1: Electricity, just enough

Think of electricity like **water in pipes**:

| Electricity word | Water analogy | Unit | In our rover |
|---|---|---|---|
| **Voltage** | Water *pressure* | Volts (V) | Batteries give 6 V; the Pi's pins use 3.3 V |
| **Current** | How much water *flows* | Amps (A), milliamps (mA) | A motor draws ~0.2–1 A; a GPIO pin can give only ~0.016 A |
| **Power** | Pressure × flow | Watts (W) | |
| **Ground (GND)** | The drain, the "0 V" reference | | Every part's GND must be connected together |

Three rules that will save you hours of debugging:

1. **Electricity needs a full loop.** Current flows out of the battery's **+**, through the device, and back into its **−** (GND). If the loop is broken anywhere, nothing works.
2. **All grounds must be connected ("common ground").** Your rover has two batteries: one for the computer and one for the motors. Their GNDs must be wired together, or the computer's signals mean nothing to the motor driver. That's like two people arguing about height without agreeing where "zero" is.
3. **Never connect + directly to − (a "short circuit").** A huge current flows, wires get hot, and batteries can be damaged. Always double-check before switching on.

### Why can't the computer just power the motors directly?

A Raspberry Pi GPIO pin can supply about **16 mA**. A small motor wants **200–1000 mA**. Connecting a motor directly to a pin would be like trying to fill a swimming pool through a drinking straw, and it would probably destroy the pin. Motors also create electrical "noise" and voltage spikes that can crash or damage a computer.

So we use a **motor driver**: a small board that takes *weak signals* from the computer and switches *big current* from a separate battery into the motors. It's a remote-controlled valve.

---

## Part 2: The parts of a robot

Every robot, from a toy to a Mars rover, has the same main parts:

| Part | Human analogy | In our rover |
|---|---|---|
| **Sensors** | Eyes, ears | Camera, ultrasonic distance sensor |
| **Computer (controller)** | Spinal cord | Raspberry Pi Zero 2 W, or ESP32-CAM |
| **Brain (planner)** | Thinking brain | **Claude**, running in the cloud |
| **Actuators** | Muscles | DC motors + wheels |
| **Motor driver** | Nerves to the muscles | TB6612FNG board |
| **Power** | Food | Batteries |
| **Chassis** | Skeleton | Plastic/acrylic frame from a kit |

### Raspberry Pi vs. ESP32: what's the difference?

- **Raspberry Pi Zero 2 W** is a real, tiny **computer** with Linux, like a mini laptop with no screen. It runs Python, connects to WiFi, and can talk to Claude directly. You use it over WiFi from your laptop with **SSH** (a text window into another computer).
- **ESP32-CAM** is a **microcontroller**: a much simpler chip with no operating system. It runs one program in a loop, written in C++ with the Arduino software. It's cheap and has a camera built in, but it's too limited to run the Claude brain comfortably, so in Option B **your laptop is the brain** and the ESP32 is the "body".

---

## Part 3: How the computer controls things

### GPIO pins

**GPIO = General Purpose Input/Output.** These are the metal pins on the side of the Pi. Your code can set each pin:

- **HIGH** (3.3 V, "on", `1`) or **LOW** (0 V, "off", `0`) as an **output**, to send a signal
- or *read* whether something is HIGH or LOW as an **input**, for example the echo of the distance sensor

Pins have two numbering systems, which confuses everyone at first:
- **Physical pin number**: its position on the header, 1 to 40, counting in a zig-zag.
- **GPIO/BCM number**: the chip's internal name, like "GPIO17".

Our code uses **BCM numbers**. The wiring tables in the build guides list **both**. Run `pinout` on the Pi to print a map.

### PWM: how to control speed with an on/off switch

A pin can only be on or off. So how do we make a motor go *half* speed? We switch it on and off very fast, hundreds of times a second:

```
100% speed: ████████████████████████
 50% speed: ██░░██░░██░░██░░██░░██░░     (on half the time)
 25% speed: █░░░█░░░█░░░█░░░█░░░█░░░     (on a quarter of the time)
```

The motor is too slow and heavy to notice the flicker. It just feels the *average* power. This is **PWM (Pulse Width Modulation)**, and it's used everywhere, including LED dimmers, drones and electric cars. In our code, `DRIVE_SPEED = 0.6` in `config.py` means "on 60% of the time".

### The H-bridge: how to reverse a motor

A DC motor spins one way when current flows through it one way, and the opposite way when you reverse the current. Swapping wires by hand isn't practical. An **H-bridge** is four electronic switches arranged in an "H" shape that can send current either direction:

```
        +Battery                     IN1  IN2   Motor does
        │      │                      1    0    forward
       S1      S3                     0    1    backward
        ├──M───┤                      0    0    coast / stop
       S2      S4                     1    1    brake
        │      │
        GND   GND
```

Our **TB6612FNG** board has *two* H-bridges (channels A and B), one for the left wheels and one for the right. Each channel has three control pins: `IN1`, `IN2` for direction, and `PWM` for speed.

> **Why TB6612FNG and not the popular L298N?** The L298N is older and wastes about 2 V as heat. With a 6 V battery pack, your 6 V motors would only get about 4 V and be weak. The TB6612 wastes almost nothing, is smaller, and works directly with 3.3 V signals. It costs the same.

### How the rover turns (skid steering)

There's no steering wheel. Like a tank:
- both sides forward → **go straight**
- left side backward + right side forward → **spin left in place**
- left side forward + right side backward → **spin right in place**

### The ultrasonic distance sensor

It works like a bat. It sends out a "click" of sound too high for us to hear, then times how long the echo takes to come back. Sound travels about 343 m/s, so `distance = time × 343 / 2` (÷2 because the sound goes there *and* back). It's good from about 3 cm to 2 m. It struggles with soft surfaces like curtains and with objects hit at steep angles.

---

## Part 4: How Claude drives

**Claude** is an AI model made by Anthropic. It can look at images and reason about them. Your code reaches it over the internet through the **Claude API**: your program sends a request and gets a reply. Every request costs a tiny amount of money (fractions of a cent to a couple of cents), paid from credit you buy in advance.

Each step of a mission works like this:

```
 Rover (Python)                                   Claude (cloud)
 ─────────────                                    ─────────────
 1. take photo, read distance sensor
 2. send: mission + photo + distance  ─────────▶  looks at the photo,
          + "your last 8 moves were…"             considers the mission
                                       ◀─────────  3. replies with JSON:
                                                     {"action": "turn_left",
                                                      "amount": 30,
                                                      "why": "door is to the left", …}
 4. check the move is safe & limited,
    then run the motors
 5. repeat
```

Key ideas:

- **Structured output.** We tell the API exactly what shape the answer must have, a *JSON schema*, and the API guarantees Claude replies in that shape. Our code never has to guess what Claude meant.
- **Short moves and frequent looks.** Claude only sees a still photo every few seconds. A long blind drive is risky, so we keep moves small and look again after each one.
- **Memory.** Each request is independent: Claude doesn't remember the last one by itself. So we include a short text log of the last few moves, plus a `note_to_self` that Claude writes for its future self.
- **Layered control (reflex vs. brain).** Claude takes a few seconds to answer. That's fine for planning, but too slow to avoid a wall you're about to hit. So the rover also has a **reflex**: while driving forward, the code checks the distance sensor 20 times a second and brakes instantly if something is within 20 cm. Real robots and real animals use this same split: fast, simple reflexes plus slower, smarter thinking.
- **Dead reckoning.** The rover doesn't know where it is. It *estimates* movement from time: "I drove at 30 cm/s for 1 s, so about 30 cm." This drifts, because wheels slip, carpet is grippier than wood, and batteries fade. That's why you'll **calibrate**, and why Claude re-checks the camera after every move.

---

## Part 5: Tools and words you'll meet

| Word | Meaning |
|---|---|
| **Terminal / command line** | A text window where you type commands. Mac: "Terminal" app. Windows: "PowerShell" or "Windows Terminal". |
| **SSH** | Opening a terminal *on another computer* (the Pi) over WiFi: `ssh rover@rover.local` |
| **Python** | The programming language of this project. Easy to read. |
| **venv (virtual environment)** | A private box of Python libraries for one project, so projects don't conflict. |
| **pip** | The tool that installs Python libraries. |
| **git** | Tracks versions of your code; `git clone` downloads a project. |
| **Breadboard** | A plastic board with holes that are connected in rows, for making connections without soldering. |
| **Jumper wires / Dupont wires** | Wires with little plugs on the ends. **M** = male (a pin), **F** = female (a socket). |
| **Header** | A row of pins on a board. "Pre-soldered header" means you don't need to solder it on. |
| **Firmware** | The program that lives on a microcontroller such as the ESP32. |
| **API key** | A secret password that lets your program use Claude and charges your account. Never share it or put it on GitHub. |
| **Token** | The unit Claude's usage is measured and billed in. About ¾ of a word of text; a 640×480 photo is about 400 tokens. |

Ready? → [1. Shopping lists](01-shopping-lists.md)
