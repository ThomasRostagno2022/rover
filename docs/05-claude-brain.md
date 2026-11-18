# 5. The Claude brain: let Claude drive

You've built a rover that you can drive yourself. Now let's give it a mind of its own.

---

## Step 1: Get a Claude API key

1. Go to **https://console.anthropic.com** and create an account.
2. Go to **Billing** and add **$5–10** of prepaid credit.
3. **Set a spending limit** under **Limits**, for example $10/month. That way a bug can never cost you more than you're comfortable with.
4. Go to **API Keys → Create Key** and name it `rover`. **Copy the key now** (it starts with `sk-ant-`). You can only see it once.

> 🔐 **Treat the key like a password.** Anyone with it can spend your credit. Never paste it into code you commit, never put it on GitHub, and never post it in a screenshot. If it leaks, delete it in the console and make a new one.

## Step 2: Give the key to the rover

The code reads the key from an **environment variable** called `ANTHROPIC_API_KEY`, a named value your terminal hands to programs. That keeps it out of the code.

**On the Pi (Options A and C)**, SSH in and run this, pasting your real key:

```bash
echo "export ANTHROPIC_API_KEY='sk-ant-PASTE-YOURS-HERE'" >> ~/.bashrc
```
```bash
source ~/.bashrc
```

`~/.bashrc` runs every time you log in, so you only do this once.

**On your laptop (Option B or the simulator):**
- **Mac/Linux:** the same two commands as above. On a Mac, use `~/.zshrc` in place of `~/.bashrc`.
- **Windows (PowerShell):** run `setx ANTHROPIC_API_KEY "sk-ant-PASTE-YOURS-HERE"`, then **close and reopen** the terminal.

Check that it worked by running the self-check again. Step 4 should now print Claude's description of a photo:

```bash
python -m rover check --hardware pi
```

## Step 3: Your first mission 🚀

Put the rover on the **floor** in an open area. Then run:

```bash
cd ~/rover && source .venv/bin/activate
python -m rover mission --hardware pi "Drive around and describe what you find. Stop after looking in 4 directions."
```

(For Option B, use `--hardware esp32`.)

You'll see something like:

```
MISSION: Drive around and describe what you find. Stop after looking in 4 directions.
Brain: claude-opus-5-5 (effort low). Logging to /home/rover/rover/missions/2026-11-18_14-02-11
Press Ctrl+C at any time to stop the rover.

Step 1: thinking...
   sees: A wooden floor with a sofa about 1.5 m ahead and a lamp to the right.
   why:  Start by looking left to survey the room.
   move: turn_left 90   (3.2s to decide)
Step 2: thinking...
   ...
```

**Press `Ctrl+C` at any time to stop.** The motors stop immediately.

### Good first missions

Start easy and get harder:

1. `"Turn around slowly in a full circle and tell me what's in the room."`
2. `"Drive toward the biggest open space without hitting anything. Complete after 10 moves."`
3. `"Find the red cup and stop about 20 cm in front of it."` (put a cup somewhere visible)
4. `"Find the door and drive through it into the next room."`
5. `"Explore the room and find my black backpack. It might be behind the sofa."`
6. `"There are three colored objects on the floor. Visit each one: red first, then blue, then yellow."`
7. **Moon mission:** build a "lunar surface" with pillows and books, put a "lander" (any box) somewhere, and use `"You are a lunar rover. Navigate the rough terrain to reach the lander (a cardboard box) without getting stuck."`

### Keep it running when your laptop sleeps (Pi only)

If your SSH connection drops, a mission running in it stops. Use **tmux** to keep it alive. Install it once:

```bash
sudo apt install -y tmux
```

Start a tmux session, then run the mission inside it as usual:

```bash
tmux new -s rover
```

To leave it running in the background, press `Ctrl+B` then `D`. To come back to it later, run:

```bash
tmux attach -t rover
```

## Step 4: Review what Claude saw

Every mission saves a folder in `missions/` containing:
- `step_001.jpg`, `step_002.jpg`, … : what the camera saw at each step
- `log.jsonl` : every decision, one per line
- `report.html` : **a web page with every photo next to Claude's reasoning**

To view the report from a Pi, serve the folder from the Pi:

```bash
cd ~/rover/missions && python3 -m http.server 8000
```

Then open `http://rover.local:8000` in your laptop's browser, click the newest folder, then `report.html`. Press `Ctrl+C` to stop the web server when you're done.

Reading these reports is the **best way to learn**. You'll see exactly where Claude misjudged a distance, missed an object, or got confused. Then you can improve the prompt, the camera angle or the calibration.

---

## How the code works (a guided tour)

Open these files and read along. They're commented for beginners.

### `rover/__main__.py` → `cmd_mission()`: the main loop

```python
for step in range(1, max_steps + 1):
    jpeg = rover.capture_jpeg()                    # 1. SENSE
    distance = rover.distance_cm()
    d = brain.decide(jpeg, distance, history, step)    # 2. THINK  (Claude)
    if d.action == "forward":                          # 3. ACT
        result = rover.forward(d.amount)
    ...
    history.append(f"step {step}: {d.action} {d.amount} -> {result} ...")  # 4. REMEMBER
```

### `rover/brain.py`: talking to Claude

- **`SYSTEM_PROMPT`** is the rover's "job description": what it is, what the camera sees, what moves exist, and driving tips. **This is the main thing to experiment with.** Changing it changes how the rover behaves.
- **`DECISION_SCHEMA`** is the exact JSON shape Claude must reply with. Because we pass it as `output_config.format`, the API *guarantees* the reply matches it, including that `action` is one of the five allowed words.
- **`decide()`** builds one request with the **photo** (as base64 text) plus the mission, distance and history, then reads back the JSON.
- `effort: "low"` tells Claude not to overthink each move. That makes it faster and cheaper. Try `--effort medium` for harder missions.
- `fallbacks="default"`: in the rare case a request is declined by a safety filter, the API automatically retries it on a recommended backup model.

### `rover/hardware/base.py`: moves + the safety reflex

- `forward(cm)` converts centimetres into seconds using your calibration: `seconds = cm / CM_PER_SECOND`.
- **Every move is clamped** to `MAX_MOVE_CM` and `MAX_TURN_DEGREES`. **Never trust any single decision blindly**, whether it comes from an AI or a human.
- `run_for(..., watch_obstacles=True)` checks the distance sensor 20× per second while driving forward and stops instantly if something is within `SAFETY_STOP_CM`. That's the **reflex** layer, and it works even if Claude is wrong or slow.

---

## What does it cost?

Each step sends one 640×480 photo (about 400 tokens), the instructions and history (about 1,000 tokens), and gets back a short answer with a bit of thinking (a few hundred tokens). With the default **Claude Opus 5.5** at low effort, that's roughly **1–2 cents per step**:

| | Per step | 20-step mission | $10 credit gets you |
|---|---|---|---|
| Claude Opus 5.5 (default, smartest) | ~$0.01–0.02 | ~$0.20–0.40 | ~25–50 missions |
| Claude Sonnet 5.5 | ~half of Opus | ~$0.10–0.20 | ~50–100 missions |
| Claude Haiku 5.5 (fastest, cheapest) | well under 1 cent | ~$0.01–0.02 | many hundreds |

These are estimates. The program prints the **actual tokens used and an estimated cost** at the end of every mission, and the console shows your real usage. To try a different model:

```bash
python -m rover mission --hardware pi --model claude-haiku-5-5 "Find the red cup"
```

Or set `CLAUDE_MODEL` in `config.py`. **Experiment!** Compare how well each model drives on the *same* mission. Smaller models are faster and cheaper but may misjudge more. That's a real engineering trade-off, and the mission reports let you measure it.

---

## Experiments to try (this is where the learning happens)

1. **Prompt engineering.** Add a sentence to `SYSTEM_PROMPT`, such as *"Prefer to keep obstacles on your left"*, and see whether behaviour changes. What happens if you remove the "Driving tips" entirely?
2. **Camera angle.** Tilt the camera up or down and compare missions. What helps Claude judge distance best?
3. **Calibration.** Deliberately set `CM_PER_SECOND` wrong by double. How does Claude cope? It gets feedback from each new photo, so it often adapts, which is called *closed-loop control*.
4. **Memory.** Set `MEMORY_STEPS = 0`. Does the rover get stuck in loops more often? Why?
5. **Effort.** Run the same mission at `--effort low` and `--effort high`. Compare speed, cost and success.
6. **New actions.** Add a `"look_around"` action that does 4 × 90° turns and sends all four photos in one request (a mini panorama). You'll need to edit `ACTIONS`, the schema, `decide()` to accept several images, and the loop in `__main__.py`.

**Next → [6. Troubleshooting & what to build next](06-troubleshooting-and-next-steps.md)**
