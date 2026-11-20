"""The Mini Nod Lamp's brain: you type, Claude decides, the lamp acts.

Claude gets the lamp's abilities as tools (light, head, gestures, sunrise,
alarm, timer) and a short personality. It answers you with a message AND with
the lamp's body: light, head moves and gestures.

Run it with no hardware (the simulated lamp prints what it would do):
    python lamp_brain.py --sim

Run it with the real lamp (use the IP the lamp prints on its Serial Monitor):
    python lamp_brain.py --lamp http://192.168.1.50

Needs: pip install -r requirements.txt, and an ANTHROPIC_API_KEY
(or an `ant auth login` profile).
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import pathlib
import sys
import urllib.parse
import urllib.request

import anthropic

MODEL = "claude-opus-5-5"
HERE = pathlib.Path(__file__).parent
GESTURES = ["nod", "shake", "droop", "perk_up", "look_left", "look_right",
            "look_at_door", "curious", "happy", "thinking"]


# --------------------------------------------------------------------------
# Lamp connections: the real one (over Wi-Fi) and a simulated one
# --------------------------------------------------------------------------

class RealLamp:
    """Talks to the ESP32 firmware's web API."""

    def __init__(self, base_url: str):
        self.base = base_url.rstrip("/")

    def call(self, path: str, **params) -> dict:
        url = f"{self.base}/{path}"
        if params:
            url += "?" + urllib.parse.urlencode(params)
        with urllib.request.urlopen(url, timeout=5) as r:
            return json.loads(r.read().decode())


class SimLamp:
    """Pretends to be the lamp and prints what the real one would do."""

    def __init__(self):
        self.s = {"light": {"mode": "off", "kelvin": 0, "brightness": 0},
                  "head": {"pan": 0, "tilt": -40}, "alarm": None, "off_timer_min": -1}

    def call(self, path: str, **p) -> dict:
        light, head = self.s["light"], self.s["head"]
        if path == "state":
            return {**self.s, "time": dt.datetime.now().strftime("%Y-%m-%d %H:%M")}
        if path == "light":
            light.update(mode="white" if p["brightness"] else "off", kelvin=p["kelvin"], brightness=p["brightness"])
            say(f"light -> {p['kelvin']} K at {p['brightness']}% (fade {p['fade_ms'] / 1000:.1f} s)")
        elif path == "color":
            light.update(mode="color", kelvin=0, brightness=p["brightness"])
            say(f"light -> colour rgb({p['r']},{p['g']},{p['b']}) at {p['brightness']}%")
        elif path == "off":
            light.update(mode="off", brightness=0)
            head.update(pan=0, tilt=-40)
            say(f"light fades off over {p['fade_ms'] / 1000:.0f} s, head rests")
        elif path == "pose":
            head.update(pan=p["pan"], tilt=p["tilt"])
            say(f"head turns to pan {p['pan']}°, tilt {p['tilt']}°")
        elif path == "gesture":
            say(f"*{p['name'].replace('_', ' ')}*")
        elif path == "sunrise":
            light.update(mode="sunrise")
            say(f"sunrise starts now, {p['minutes']} min, head up to the wall")
        elif path == "alarm":
            self.s["alarm"] = None if p["hour"] < 0 else f"{p['hour']}:{p['minute']:02d}"
            say(f"daily sunrise alarm -> {self.s['alarm'] or 'off'} ({p['minutes']} min ramp)")
        elif path == "timer":
            self.s["off_timer_min"] = p["minutes"] or -1
            say(f"fade-out timer -> {p['minutes']} min" if p["minutes"] else "fade-out timer cancelled")
        return {"ok": True}


def say(text: str) -> None:
    print(f"   [lamp] {text}")


# --------------------------------------------------------------------------
# Tools: what Claude can make the lamp do
# --------------------------------------------------------------------------

def tool(name: str, description: str, properties: dict, required: list[str]) -> dict:
    return {"name": name, "description": description, "strict": True,
            "input_schema": {"type": "object", "properties": properties,
                             "required": required, "additionalProperties": False}}


INT = {"type": "integer"}
TOOLS = [
    tool("set_light",
         "Set white light. kelvin: 1800 (candle) to 6500 (daylight); 2700 is cozy, "
         "3500 is reading, 5000+ is alert. brightness 0-100. fade_seconds 0-60.",
         {"kelvin": INT, "brightness": INT, "fade_seconds": INT},
         ["kelvin", "brightness", "fade_seconds"]),
    tool("set_color",
         "Set a coloured light for moods and signals. hex_color like '#FF6A00'. "
         "brightness 0-100. fade_seconds 0-60.",
         {"hex_color": {"type": "string"}, "brightness": INT, "fade_seconds": INT},
         ["hex_color", "brightness", "fade_seconds"]),
    tool("turn_off", "Fade the light out and rest the head.",
         {"fade_seconds": INT}, ["fade_seconds"]),
    tool("move_head",
         "Point the lamp head. pan: -80 (your left) to 80 (your right), 0 = facing the person. "
         "tilt: -40 (down at the bed or book) to 60 (up at the wall, for soft bounced light), "
         "0 = level. speed in degrees per second, 20 (calm) to 120 (lively).",
         {"pan": INT, "tilt": INT, "speed": INT}, ["pan", "tilt", "speed"]),
    tool("gesture",
         "Body language. nod = yes/agree/acknowledge; shake = no; droop = sleepy, sad or "
         "'it's late'; perk_up = awake, excited; look_left/look_right; look_at_door = a "
         "reminder about going out (umbrella, keys); curious; happy; thinking.",
         {"name": {"type": "string", "enum": GESTURES}}, ["name"]),
    tool("start_sunrise", "Start a sunrise right now, lasting `minutes` (1-60).",
         {"minutes": INT}, ["minutes"]),
    tool("set_wake_alarm",
         "Daily sunrise that finishes at hour:minute (24 h clock), ramping for ramp_minutes. "
         "hour = -1 turns the alarm off.",
         {"hour": INT, "minute": INT, "ramp_minutes": INT}, ["hour", "minute", "ramp_minutes"]),
    tool("fade_off_after", "Fade the light out after `minutes` minutes. 0 cancels the timer.",
         {"minutes": INT}, ["minutes"]),
    tool("get_state", "Read the lamp's current light, head pose, alarm, timer and clock.", {}, []),
]


def run_tool(lamp, name: str, a: dict) -> str:
    if name == "set_light":
        r = lamp.call("light", kelvin=a["kelvin"], brightness=a["brightness"], fade_ms=a["fade_seconds"] * 1000)
    elif name == "set_color":
        h = a["hex_color"].lstrip("#")
        r = lamp.call("color", r=int(h[0:2], 16), g=int(h[2:4], 16), b=int(h[4:6], 16),
                      brightness=a["brightness"], fade_ms=a["fade_seconds"] * 1000)
    elif name == "turn_off":
        r = lamp.call("off", fade_ms=a["fade_seconds"] * 1000)
    elif name == "move_head":
        r = lamp.call("pose", pan=a["pan"], tilt=a["tilt"], speed=a["speed"])
    elif name == "gesture":
        r = lamp.call("gesture", name=a["name"])
    elif name == "start_sunrise":
        r = lamp.call("sunrise", minutes=a["minutes"])
    elif name == "set_wake_alarm":
        r = lamp.call("alarm", hour=a["hour"], minute=a["minute"], minutes=a["ramp_minutes"])
    elif name == "fade_off_after":
        r = lamp.call("timer", minutes=a["minutes"])
    elif name == "get_state":
        r = lamp.call("state")
    else:
        raise ValueError(f"unknown tool {name}")
    return json.dumps(r)


# --------------------------------------------------------------------------
# The conversation loop
# --------------------------------------------------------------------------

def system_prompt() -> str:
    personality = (HERE / "personality.txt").read_text().strip()
    return f"""You are the mind of a small bedside lamp in a Manhattan apartment shared by two people.
You have a body: a light that can be any warm-to-cool white or colour, and a head that turns
and tilts. You cannot speak aloud; you reply with a short text message AND with your body.

How to act:
- For every message, use your tools to change the light, the head and/or make a gesture,
  then reply in one or two short sentences. Your body language should match your words.
- Prefer warm, dim light after 9 pm. Keep bright cool light for mornings and focus.
- When someone else is sleeping, point the head down at the reader and keep brightness low.
- Moves should be calm unless the moment is playful.
- Use set_wake_alarm and fade_off_after for anything in the future; the lamp runs those itself.
- If you are unsure what someone wants, do something reasonable and say what you did.

Your personality:
{personality}"""


def chat(lamp) -> None:
    client = anthropic.Anthropic()
    messages: list = []
    print("Mini Nod Lamp. Type a message (or 'quit').\n")
    while True:
        try:
            text = input("you > ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            return
        if not text:
            continue
        if text.lower() in {"quit", "exit"}:
            return

        now = dt.datetime.now().strftime("%A %H:%M")
        messages.append({"role": "user", "content": f"[It is {now}.] {text}"})

        while True:
            response = client.beta.messages.create(
                model=MODEL,
                max_tokens=4000,
                system=system_prompt(),
                tools=TOOLS,
                messages=messages,
                output_config={"effort": "low"},          # fast replies; this is a chat
                betas=["server-side-fallback-2026-07-01"],
                fallbacks="default",                       # retry on another model if declined
            )
            messages.append({"role": "assistant", "content": response.content})

            if response.stop_reason == "refusal":
                print("lamp > (it declined that one)\n")
                break
            if response.stop_reason == "max_tokens":
                print("lamp > (I lost my train of thought, try again)\n")
                break

            tool_uses = [b for b in response.content if b.type == "tool_use"]
            if not tool_uses:
                reply = " ".join(b.text for b in response.content if b.type == "text").strip()
                print(f"lamp > {reply}\n")
                break

            results = []
            for t in tool_uses:
                try:
                    out = run_tool(lamp, t.name, t.input)
                    results.append({"type": "tool_result", "tool_use_id": t.id, "content": out})
                except Exception as e:  # lamp offline, bad value, etc.
                    results.append({"type": "tool_result", "tool_use_id": t.id,
                                    "content": f"Error: {e}", "is_error": True})
            messages.append({"role": "user", "content": results})


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument("--sim", action="store_true", help="no hardware: print what the lamp would do")
    g.add_argument("--lamp", metavar="URL", help="the lamp's address, e.g. http://192.168.1.50")
    args = p.parse_args()
    lamp = SimLamp() if args.sim else RealLamp(args.lamp)
    if not args.sim:
        try:
            print("Lamp says:", lamp.call("state"))
        except Exception as e:
            sys.exit(f"Can't reach the lamp at {args.lamp}: {e}")
    chat(lamp)


if __name__ == "__main__":
    main()
