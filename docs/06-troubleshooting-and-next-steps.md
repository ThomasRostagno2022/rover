# 6. Troubleshooting & what to build next

## The debugging mindset

When something doesn't work, **don't change five things at once**. Robots are chains: power → computer → signal → driver → motor. Find the *first* broken link:

1. **Is it powered?** LEDs on? Batteries fresh? Switch on? A multimeter set to "DC V" across the battery wires should read about 5–6 V.
2. **Are the grounds connected?** This is the cause of half of all beginner problems.
3. **Is each wire where the table says?** Check them one at a time, using the labels on the board.
4. **Test the smallest piece on its own**: e.g. `python -m rover check` tests each part separately.
5. **Read the error message.** The last line of a Python error usually says exactly what's wrong. Paste it into Claude and ask what it means. That's a great use of AI while learning.

---

## Common problems

### Can't connect to the Pi (`ssh: Could not resolve hostname rover.local`)
- Wait 5 minutes after the very first boot.
- Is your laptop on the **same WiFi** network? Guest networks often block devices from seeing each other.
- Find the Pi's IP address in your router's device list and use `ssh rover@<that-ip>`.
- Re-check the WiFi name, password and country in Raspberry Pi Imager and re-write the card. Remember the Pi Zero 2 W only supports **2.4 GHz** WiFi.
- On Windows, `.local` names sometimes don't work. Use the IP address.

### `REMOTE HOST IDENTIFICATION HAS CHANGED` when SSH-ing
This happens after you re-flash the SD card. Clear the old key from your laptop, then connect again:
```bash
ssh-keygen -R rover.local
```

### Camera: `No cameras available` / `Camera init failed`
- Power off and **re-seat the ribbon cable** at both ends. The contacts must face the right way (toward the board on the Pi Zero) and the latch must be fully closed.
- Pi Zero: make sure you're using the narrow **Zero** cable.
- ESP32-CAM: the tiny camera connector latch flips up. Make sure the ribbon is fully in before closing it.

### Motors don't move
- Is the **battery switch on**, with fresh batteries in the right way round?
- Is **STBY** connected to 3.3 V? If not, the driver sleeps.
- Is **PWMA/PWMB** connected? Without them the channel is off.
- Is **battery − connected to GND** on the driver, and is the Pi's GND also connected?
- Does a motor work when touched directly to the battery wires? That tests the motor itself.
- Try lower `DRIVE_SPEED` values. At very low PWM (< 0.3) TT motors often just hum and don't turn.

### Motors twitch, or the Pi reboots/freezes when motors start
The motors are disturbing the Pi's power supply.
- Make sure the Pi runs from its **own power bank**, not from the motor batteries.
- Keep motor wires away from the Pi and twist each motor's two wires together.
- Optional fix: solder a small **0.1 µF ceramic capacitor** across each motor's two terminals. This is the classic noise fix.

### One side is weaker, so the rover curves when driving "straight"
That's normal with cheap motors. It doesn't matter much, because Claude corrects course from the photos. To improve it, add a small trim: in `rover/hardware/pi.py`, `set_motors()`, multiply the stronger side by e.g. `0.9`.

### Headlights don't light
- Is the LED the right way round? The **long leg** goes toward + (the resistor from 5 V or the GPIO pin). A backwards LED just stays dark.
- Version 2: check the transistor's legs. With the flat side facing you, a PN2222A is **E, B, C** left to right. Swapped E and C is the classic mistake.
- Test the LED alone: connect 5 V → 100 Ω → LED → GND directly. If it lights, the LED is fine and the problem is the switching part.
- Is `USE_HEADLIGHTS = True` in `config.py`, and `HEADLIGHT_PIN = 26` matching pin 37?
- Version 1 is *meant* to be dim. For real light, build version 2.
- ESP32-CAM: after updating the firmware, press 💡 on the rover's web page. If the page has no 💡 button, the new firmware isn't on the board yet.

### Claude never switches the headlights on
- Check the start of the mission output says `Headlights: fitted (Claude-controlled)`. If it doesn't, set `USE_HEADLIGHTS = True` in `config.py` (Pi).
- Look at the `Brightness:` values in `report.html`. Cameras brighten dim scenes automatically, so a room that looks dark to you may look "fine" to the camera, and Claude reasonably leaves the lights off.
- To nudge it, make the tip in `LIGHTS_TIP` (in `rover/brain.py`) stronger, e.g. "Switch the headlights on whenever brightness is below 80."
- Or turn on the safety net: `AUTO_LIGHTS = True` in `config.py`.

### Headlights never switch on automatically (only with `AUTO_LIGHTS = True`)
- `AUTO_LIGHTS` must be `True` in `config.py`. It's off by default, because Claude controls the lights.
- Pillow must be installed (`python3 -c "import PIL"` should print nothing). On the Pi, run `sudo apt install -y python3-pil`; on a laptop, `pip install pillow`.
- Your room may not count as "dark" for the camera, because cameras brighten dim scenes automatically. Raise `DARK_THRESHOLD` (e.g. to 80), and check the `Brightness:` values in `report.html` to choose a good number.

### Glare / washed-out photos with headlights on
Move the LEDs further from the lens, or point them more downward. Light bouncing off the chassis straight into the camera ruins photos.

### Distance sensor always reads 200 cm, or jumps around
- Trig and Echo swapped? Check pins 29 and 31.
- Soft things like curtains, sofas and clothes absorb sound, and angled surfaces bounce it away. That's a real limitation of ultrasonic sensors.
- The sensor must not "see" the chassis or wheels. Mount it at the very front.
- gpiozero may print `no echo received` warnings now and then. They're harmless.

### `pip install` fails or takes forever on the Pi Zero
- Make sure you're on **64-bit** Raspberry Pi OS: `uname -m` should print `aarch64`.
- Make sure you've activated the venv (your prompt shows `(.venv)`).
- Give it time: a Pi Zero is slow, so 10 minutes is normal.

### `ANTHROPIC_API_KEY is not set` / `Your API key was rejected`
- Run `echo $ANTHROPIC_API_KEY`. It should print your key.
- Did you `source ~/.bashrc` (or open a new terminal) after adding it?
- Check the key still exists at console.anthropic.com and that you have credit left.

### Claude drives badly
Open the mission's `report.html` and look at what it *saw*:
- **Photos dark or blurry?** Use better lighting and slower moves (lower `DRIVE_SPEED`), and check the camera focus.
- **Camera pointing at the ceiling or only the floor?** Fix the tilt.
- **Moves are much longer or shorter than Claude asked for?** Re-run `calibrate`.
- **Keeps bumping into things?** Lower `MAX_MOVE_CM` to 30, and add a line to the prompt like "Never drive forward more than 20 cm unless the path is clearly open".
- **Gets lost or loops?** Raise `MEMORY_STEPS`, or try `--effort medium`.

### ESP32-CAM specific
- `Brownout detector was triggered`: the power supply isn't strong enough. Use a good power bank and a short, thick cable.
- Upload fails: hold **IO0** on the MB board, tap **RST**, release IO0, and upload again.
- Won't boot when motors are wired: unplug the **IO12** wire during power-on (see the guide).
- `Could not reach the rover`: check the address on the Serial Monitor, and that the laptop and rover are on the same 2.4 GHz WiFi.

---

## Upgrades

Once your rover drives itself, here are ideas, from easy to hard:

### Easy
- **Mission from your phone.** Start missions from a phone SSH app such as Termius.
- **Speak.** Add a small USB or I²S speaker and use `espeak` so the rover reads out what it sees.

### Medium
- **Pan camera.** Mount the camera on an **SG90 micro servo** ($3) so the rover can "look left/right" without turning its whole body, like a real rover's mast. Add `look_left` and `look_right` actions.
- **Cliff sensor.** A downward-facing infrared sensor (search `TCRT5000 module`, about $1) at the front edge. If the floor suddenly "disappears", stop, before the rover drives off a step. This is the biggest safety upgrade; see [guide 7](07-what-it-can-really-do.md#3-what-can-stop-it-terrain-and-obstacles).
- **Bumper switches.** Two micro-switches at the front corners give a reflex for things the ultrasonic sensor misses.
- **Distance sensor for Option B.** Swap the ESP32-CAM for an **ESP32-S3** camera board (e.g. "XIAO ESP32S3 Sense"), which has more free pins.
- **Dimmable headlights.** Use PWM (`gpiozero.PWMLED`) so Claude can choose a brightness, not just on/off. Less glare, more battery.
- **Battery monitor.** Have the rover tell Claude when its batteries are low, so it can "return to base".
- **Wheel encoders.** Small disks + sensors that count wheel turns, so distances become *measured*, not *guessed*. This is a big step toward real robotics.

### Advanced
- **Mapping.** Keep a simple 2D grid of where the rover has been (using dead reckoning) and show it to Claude as text or as a drawn image, so it can plan "unexplored areas".
- **Tool-using agent.** Instead of one move per request, give Claude **tools** (`drive`, `turn`, `take_photo`, `read_sensor`) and let it decide when to look and when to move, using the Claude API's tool use. The `claude-api` docs cover how.
- **Two brains.** Run a fast, cheap model every step for driving, and call a smarter model only when stuck or when the mission needs real reasoning.
- **ROS 2.** The Robot Operating System is what professional robots use. Re-building this rover on ROS 2 is a great "next level" project, though it needs a Pi 4/5.
- **Real rocker-bogie.** Look up the open-source **NASA JPL Open Source Rover** for a much bigger (and pricier) build.

Have fun, and post your mission reports! 🚀🌕
