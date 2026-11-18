"""
The rover's brain: send Claude a photo, get back ONE move.

How it works, every step:
  1. We send Claude: the mission, the newest camera photo, the distance
     sensor reading, and a short list of the last few moves (its "memory").
  2. Claude answers with a small JSON object, for example
        {"what_i_see": "a chair leg ahead on the left",
         "why": "steer around the chair to reach the open floor",
         "action": "turn_right", "amount": 30,
         "note_to_self": "red cup was behind the chair"}
  3. Our code (not Claude) checks the move is safe and executes it.

We use "structured outputs": we give the API a JSON schema and it guarantees
Claude's reply matches it, so we never have to guess how to read the answer.
"""

from __future__ import annotations

import base64
import json
from dataclasses import dataclass

import anthropic

from . import config

DRIVE_ACTIONS = ["forward", "backward", "turn_left", "turn_right", "mission_complete"]
LIGHT_ACTIONS = ["lights_on", "lights_off"]

LIGHTS_HELP = """\
- lights_on      : switch the headlights on (amount 0). Use it when the photo \
is too dark to see the floor and obstacles clearly.
- lights_off     : switch the headlights off (amount 0), e.g. when the room is \
bright again, to save battery.
"""

LIGHTS_TIP = """\
- If the photo is dark or murky and the headlights are off, switch them on \
before driving: never drive into darkness you can't see.
"""

SYSTEM_PROMPT_TEMPLATE = f"""\
You are the driver of a small wheeled rover (about {config.ROVER_LENGTH_CM} cm \
long), a toy-sized version of a planetary rover. You see the world through \
one forward-facing camera mounted roughly {config.CAMERA_HEIGHT_CM} cm above \
the ground. Its field of view is about {config.CAMERA_FOV_DEGREES} degrees \
wide, so things outside that cone are invisible to you until you turn.

Each step you receive the mission, a fresh photo, the reading from an \
ultrasonic distance sensor pointing straight ahead (if fitted), and a log of \
your recent moves. You choose exactly ONE move:
- forward N      : drive forward N centimetres (1-{config.MAX_MOVE_CM})
- backward N     : reverse N centimetres (1-{config.MAX_MOVE_CM})
- turn_left N    : spin in place N degrees to the left (1-{config.MAX_TURN_DEGREES})
- turn_right N   : spin in place N degrees to the right (1-{config.MAX_TURN_DEGREES})
- mission_complete: stop, because the mission is achieved (amount 0)
{{lights_help}}
Driving tips:
- Moves are approximate: wheels slip, so re-check the photo after every move.
- Prefer several short moves (10-30 cm, 15-45 degrees) over one long one.
- Objects that fill the bottom of the photo are very close.
- Never drive toward drops, stairs or the edge of a table. Avoid cables, \
liquids, pets and people's feet.
- The distance sensor stops the rover automatically if something is closer \
than {config.SAFETY_STOP_CM} cm; if your last move says SAFETY STOP, back up \
or turn instead of pushing forward.
- If you seem stuck (same view several times), try something different: \
back up and turn.
- To search for something, turn in steps of about 45 degrees to look around.
{{lights_tip}}
Keep "what_i_see" and "why" to one short sentence each. Use "note_to_self" \
for anything you want to remember for later steps (it is shown back to you)."""



def build_system_prompt(has_lights: bool) -> str:
    """The rover's job description. Light controls only appear if fitted."""
    return SYSTEM_PROMPT_TEMPLATE.format(
        lights_help=LIGHTS_HELP if has_lights else "",
        lights_tip=LIGHTS_TIP if has_lights else "",
    )


def build_schema(has_lights: bool) -> dict:
    """The shape Claude's answer must have."""
    actions = DRIVE_ACTIONS + (LIGHT_ACTIONS if has_lights else [])
    return {
        "type": "object",
        "properties": {
            "what_i_see": {"type": "string"},
            "why": {"type": "string"},
            "action": {"type": "string", "enum": actions},
            "amount": {"type": "integer"},
            "note_to_self": {"type": "string"},
        },
        "required": ["what_i_see", "why", "action", "amount", "note_to_self"],
        "additionalProperties": False,
    }


@dataclass
class Decision:
    what_i_see: str
    why: str
    action: str
    amount: int
    note_to_self: str
    input_tokens: int = 0
    output_tokens: int = 0


class ClaudeBrain:
    def __init__(self, mission: str, model: str | None = None,
                 effort: str | None = None, has_lights: bool = False):
        # Reads your key from the ANTHROPIC_API_KEY environment variable.
        self.client = anthropic.Anthropic()
        self.mission = mission
        self.model = model or config.CLAUDE_MODEL
        self.effort = effort or config.CLAUDE_EFFORT
        self.has_lights = has_lights
        self.system_prompt = build_system_prompt(has_lights)
        self.schema = build_schema(has_lights)

    def decide(self, jpeg: bytes, distance_cm: float | None,
               history: list[str], step: int, lights_on: bool = False,
               brightness: float | None = None) -> Decision:
        if distance_cm is None:
            distance_text = "no distance sensor fitted"
        else:
            distance_text = f"{distance_cm:.0f} cm to the nearest object straight ahead"

        light_text = ""
        if self.has_lights:
            light_text = f"Headlights: {'ON' if lights_on else 'off'}.\n"
        if brightness is not None:
            light_text += (f"Photo brightness: {brightness:.0f} out of 255 "
                           "(below about 60 is dark).\n")

        recent = "\n".join(history[-config.MEMORY_STEPS:]) or "(this is the first step)"

        user_content = [
            {
                "type": "image",
                "source": {
                    "type": "base64",
                    "media_type": "image/jpeg",
                    "data": base64.standard_b64encode(jpeg).decode("utf-8"),
                },
            },
            {
                "type": "text",
                "text": (
                    f"MISSION: {self.mission}\n\n"
                    f"Step {step} of at most {config.MAX_STEPS}.\n"
                    f"Distance sensor: {distance_text}.\n"
                    f"{light_text}\n"
                    f"Your recent moves (oldest first):\n{recent}\n\n"
                    "The photo above is what the camera sees right now. "
                    "Choose your next move."
                ),
            },
        ]

        response = self.client.beta.messages.create(
            model=self.model,
            max_tokens=16000,
            system=self.system_prompt,
            messages=[{"role": "user", "content": user_content}],
            output_config={
                "effort": self.effort,
                "format": {"type": "json_schema", "schema": self.schema},
            },
            # If a safety filter ever declines a request, let the API retry
            # automatically on a recommended fallback model.
            betas=["server-side-fallback-2026-07-01"],
            fallbacks="default",
        )

        if response.stop_reason == "refusal":
            raise RuntimeError("Claude declined this request; stopping the rover.")
        if response.stop_reason == "max_tokens":
            raise RuntimeError("Claude's answer was cut off; stopping the rover.")

        # The reply can contain a "thinking" block first; the JSON is in the
        # text block.
        text = next(b.text for b in response.content if b.type == "text")
        data = json.loads(text)
        return Decision(
            what_i_see=data["what_i_see"],
            why=data["why"],
            action=data["action"],
            amount=int(data["amount"]),
            note_to_self=data["note_to_self"],
            input_tokens=response.usage.input_tokens,
            output_tokens=response.usage.output_tokens,
        )
