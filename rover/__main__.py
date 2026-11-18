"""
Command line for the rover. Run from the project folder:

    python -m rover check     --hardware pi           # test every part, one by one
    python -m rover teleop    --hardware pi           # drive it yourself with the keyboard
    python -m rover calibrate --hardware pi           # measure speed + turning rate
    python -m rover mission   --hardware pi "Find the red cup and stop next to it"

--hardware can be:  pi  (Option A / C),  esp32  (Option B),  sim  (no hardware)
"""

from __future__ import annotations

import argparse
import datetime as dt
import html
import json
import os
import pathlib
import sys
import time

from . import config
from .hardware import make_rover

PROJECT_DIR = pathlib.Path(__file__).resolve().parents[1]

# Rough prices in US$ per million tokens (input, output), for the cost estimate
# printed at the end of a mission. Check https://claude.com/pricing for current ones.
PRICES = {
    "claude-opus-5-5": (4.00, 20.00),
    "claude-sonnet-5-5": (2.00, 10.00),
    "claude-haiku-5-5": (0.10, 0.50),
}


def add_hw_args(p: argparse.ArgumentParser) -> None:
    p.add_argument("--hardware", choices=["pi", "esp32", "sim"], default="pi",
                   help="which rover body to use (default: pi)")
    p.add_argument("--esp32-url", help="e.g. http://192.168.1.50 (Option B only)")
    p.add_argument("--sim-images", help="folder of .jpg photos (sim only)")


def open_rover(args, use_camera: bool = True):
    return make_rover(args.hardware, esp32_url=args.esp32_url,
                      sim_images=args.sim_images, use_camera=use_camera)


def need_api_key() -> None:
    if not os.environ.get("ANTHROPIC_API_KEY"):
        sys.exit(
            "ANTHROPIC_API_KEY is not set.\n"
            "Get a key at https://console.anthropic.com -> API Keys, then run:\n"
            "    export ANTHROPIC_API_KEY='sk-ant-...'\n"
            "(see docs/05-claude-brain.md)"
        )


# ---------------------------------------------------------------------------
# check: test every part on its own
# ---------------------------------------------------------------------------
def cmd_check(args) -> None:
    print("=== Rover self-check ===\n")
    with open_rover(args) as rover:
        print("1) Camera: taking a photo...")
        jpeg = rover.capture_jpeg()
        out = PROJECT_DIR / "test_photo.jpg"
        out.write_bytes(jpeg)
        print(f"   OK - saved {len(jpeg) // 1024} KB to {out}")
        print("   Open it and check it is the right way up (see CAMERA_UPSIDE_DOWN).\n")

        print("2) Distance sensor: 5 readings (move your hand in front of it)")
        for _ in range(5):
            d = rover.distance_cm()
            print("   no sensor fitted" if d is None else f"   {d:.0f} cm")
            if d is None:
                break
            time.sleep(0.5)
        print()

        if args.hardware != "esp32":
            print("3) Motors. LIFT THE ROVER so the wheels are off the table!")
            input("   Press Enter when the wheels can spin freely...")
            for label, l, r in [("LEFT wheels forward", 0.6, 0),
                                ("RIGHT wheels forward", 0, 0.6),
                                ("BOTH wheels forward", 0.6, 0.6)]:
                print(f"   -> {label} for 1 second")
                rover.run_for(l, r, 1.0)
                time.sleep(0.5)
        else:
            print("3) Motors: use the buttons on the rover's web page, or `teleop`.")
        print("   If a side spun the wrong way, set LEFT/RIGHT_MOTOR_REVERSED in config.py.\n")

        print("4) Claude: asking Claude to describe the photo...")
        need_api_key()
        import anthropic
        client = anthropic.Anthropic()
        import base64
        resp = client.messages.create(
            model=config.CLAUDE_MODEL,
            max_tokens=16000,
            output_config={"effort": "low"},
            messages=[{"role": "user", "content": [
                {"type": "image", "source": {
                    "type": "base64", "media_type": "image/jpeg",
                    "data": base64.standard_b64encode(jpeg).decode()}},
                {"type": "text", "text": "In one sentence: what do you see in this photo?"},
            ]}],
        )
        text = next((b.text for b in resp.content if b.type == "text"), "(no text)")
        print(f"   Claude says: {text}\n")
    print("All checks done!")


# ---------------------------------------------------------------------------
# teleop: drive by hand
# ---------------------------------------------------------------------------
def cmd_teleop(args) -> None:
    print("Keyboard driving. Type a letter then press Enter:")
    print("  w = forward 20 cm   s = back 20 cm")
    print("  a = left 45 deg     d = right 45 deg")
    print("  p = take photo      q = quit\n")
    with open_rover(args) as rover:
        n = 0
        while True:
            key = input("> ").strip().lower()
            if key == "q":
                break
            elif key == "w":
                print(rover.forward(20))
            elif key == "s":
                print(rover.backward(20))
            elif key == "a":
                print(rover.turn_left(45))
            elif key == "d":
                print(rover.turn_right(45))
            elif key == "p":
                n += 1
                out = PROJECT_DIR / f"teleop_photo_{n}.jpg"
                out.write_bytes(rover.capture_jpeg())
                print(f"saved {out}")
            d = rover.distance_cm()
            if d is not None:
                print(f"(distance ahead: {d:.0f} cm)")


# ---------------------------------------------------------------------------
# calibrate: measure how far / how much the rover moves per second
# ---------------------------------------------------------------------------
def ask_number(prompt: str) -> float:
    while True:
        try:
            return float(input(prompt))
        except ValueError:
            print("Please type just a number, e.g. 55")


def cmd_calibrate(args) -> None:
    seconds = 2.0
    with open_rover(args, use_camera=False) as rover:
        print("CALIBRATION - put the rover on the floor with ~1.5 m of space ahead.")
        print("Mark where the FRONT of the rover is with a piece of tape.")
        input("Press Enter to drive forward for 2 seconds...")
        rover.run_for(config.DRIVE_SPEED, config.DRIVE_SPEED, seconds)
        cm = ask_number("How many cm did it travel? ")

        print("\nNow put a piece of tape on the floor pointing the way the rover faces.")
        input("Press Enter to spin right for 2 seconds...")
        rover.run_for(config.TURN_SPEED, -config.TURN_SPEED, seconds)
        print("How many degrees did it turn? (90 = quarter turn, 180 = half,")
        deg = ask_number("360 = full circle, 450 = one and a quarter...) ")

    print("\nPut these lines in rover/config.py (replace the old values):\n")
    print(f"CM_PER_SECOND = {cm / seconds:.1f}")
    print(f"DEGREES_PER_SECOND = {deg / seconds:.1f}")
    print("\nTip: run calibrate 2-3 times and average. Re-do it if you change "
          "batteries, speed settings or the floor (carpet vs. wood).")


# ---------------------------------------------------------------------------
# mission: Claude drives!
# ---------------------------------------------------------------------------
def write_report(folder: pathlib.Path, mission: str, steps: list[dict]) -> pathlib.Path:
    """A simple web page with every photo + Claude's decision, for review."""
    rows = []
    for s in steps:
        rows.append(
            f"<div class='step'><img src='{s['photo']}'><div>"
            f"<h3>Step {s['step']}: {html.escape(s['action'])} {s['amount']}</h3>"
            f"<p><b>Saw:</b> {html.escape(s['what_i_see'])}</p>"
            f"<p><b>Why:</b> {html.escape(s['why'])}</p>"
            f"<p><b>Note:</b> {html.escape(s['note_to_self'])}</p>"
            f"<p><b>Distance:</b> {s['distance_cm']} &nbsp; <b>Result:</b> "
            f"{html.escape(s['result'])}</p></div></div>"
        )
    page = f"""<!doctype html><meta charset="utf-8">
<title>Rover mission log</title>
<style>
body{{font-family:system-ui,sans-serif;max-width:900px;margin:2em auto;padding:0 1em}}
.step{{display:flex;gap:1em;border-bottom:1px solid #ccc;padding:1em 0}}
.step img{{width:320px;height:auto;border-radius:6px}} h3{{margin:0 0 .5em}}
@media(max-width:600px){{.step{{flex-direction:column}}.step img{{width:100%}}}}
</style>
<h1>Mission log</h1><p><b>Mission:</b> {html.escape(mission)}</p>
{''.join(rows)}"""
    out = folder / "report.html"
    out.write_text(page, encoding="utf-8")
    return out


def cmd_mission(args) -> None:
    need_api_key()
    import anthropic
    from .brain import ClaudeBrain

    brain = ClaudeBrain(args.mission, model=args.model, effort=args.effort)
    stamp = dt.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    folder = PROJECT_DIR / "missions" / stamp
    folder.mkdir(parents=True, exist_ok=True)
    max_steps = args.max_steps or config.MAX_STEPS

    print(f"MISSION: {args.mission}")
    print(f"Brain: {brain.model} (effort {brain.effort}). Logging to {folder}")
    print("Press Ctrl+C at any time to stop the rover.\n")

    history: list[str] = []
    steps: list[dict] = []
    tokens_in = tokens_out = 0

    with open_rover(args) as rover:
        try:
            for step in range(1, max_steps + 1):
                jpeg = rover.capture_jpeg()
                photo = f"step_{step:03d}.jpg"
                (folder / photo).write_bytes(jpeg)
                distance = rover.distance_cm()

                print(f"Step {step}: thinking...", flush=True)
                t0 = time.monotonic()
                try:
                    d = brain.decide(jpeg, distance, history, step)
                except anthropic.AuthenticationError:
                    sys.exit("Your API key was rejected. Check ANTHROPIC_API_KEY.")
                except anthropic.APIConnectionError:
                    print("   No internet connection - waiting 5 s and trying again.")
                    time.sleep(5)
                    continue
                except anthropic.RateLimitError:
                    print("   Rate limited - waiting 20 s.")
                    time.sleep(20)
                    continue
                think_s = time.monotonic() - t0
                tokens_in += d.input_tokens
                tokens_out += d.output_tokens

                print(f"   sees: {d.what_i_see}")
                print(f"   why:  {d.why}")
                print(f"   move: {d.action} {d.amount}   ({think_s:.1f}s to decide)")

                if d.action == "mission_complete":
                    result = "mission complete"
                elif d.action == "forward":
                    result = rover.forward(d.amount)
                elif d.action == "backward":
                    result = rover.backward(d.amount)
                elif d.action == "turn_left":
                    result = rover.turn_left(d.amount)
                elif d.action == "turn_right":
                    result = rover.turn_right(d.amount)
                else:
                    result = "unknown action - ignored"
                if result != "ok":
                    print(f"   result: {result}")

                history.append(
                    f"step {step}: {d.action} {d.amount} -> {result}. "
                    f"Saw: {d.what_i_see} Note: {d.note_to_self}"
                )
                record = {"step": step, "photo": photo,
                          "distance_cm": None if distance is None else round(distance),
                          "result": result, **{k: getattr(d, k) for k in (
                              "what_i_see", "why", "action", "amount", "note_to_self")}}
                steps.append(record)
                with open(folder / "log.jsonl", "a", encoding="utf-8") as f:
                    f.write(json.dumps(record) + "\n")

                if d.action == "mission_complete":
                    print("\nClaude says the mission is complete!")
                    break
                time.sleep(0.3)  # let the rover stop wobbling before the next photo
            else:
                print(f"\nReached the limit of {max_steps} steps - stopping.")
        except KeyboardInterrupt:
            print("\nStopped by you (Ctrl+C).")
        finally:
            rover.stop()

    report = write_report(folder, args.mission, steps)
    price = PRICES.get(brain.model)
    cost = ""
    if price:
        usd = tokens_in / 1e6 * price[0] + tokens_out / 1e6 * price[1]
        cost = f" (about ${usd:.2f})"
    print(f"\nTokens used: {tokens_in} in / {tokens_out} out{cost}")
    print(f"Mission report: {report}")


def main() -> None:
    parser = argparse.ArgumentParser(prog="python -m rover",
                                     description="A Claude-driven rover")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("check", help="test camera, sensor, motors and Claude")
    add_hw_args(p)
    p.set_defaults(func=cmd_check)

    p = sub.add_parser("teleop", help="drive the rover yourself")
    add_hw_args(p)
    p.set_defaults(func=cmd_teleop)

    p = sub.add_parser("calibrate", help="measure speed and turn rate")
    add_hw_args(p)
    p.set_defaults(func=cmd_calibrate)

    p = sub.add_parser("mission", help="let Claude drive")
    add_hw_args(p)
    p.add_argument("mission", help='what to do, e.g. "explore the room and find a shoe"')
    p.add_argument("--max-steps", type=int, help=f"default {config.MAX_STEPS}")
    p.add_argument("--model", help=f"default {config.CLAUDE_MODEL}")
    p.add_argument("--effort", choices=["low", "medium", "high"],
                   help=f"default {config.CLAUDE_EFFORT}")
    p.set_defaults(func=cmd_mission)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
