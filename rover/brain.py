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

ACTIONS = ["forward", "backward", "turn_left", "turn_right", "mission_complete"]

SYSTEM_PROMPT = f"""\
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

Keep "what_i_see" and "why" to one short sentence each. Use "note_to_self" \
for anything you want to remember for later steps (it is shown back to you)."""

# The shape Claude's answer must have.
DECISION_SCHEMA = {
    "type": "object",
    "properties": {
        "what_i_see": {"type": "string"},
        "why": {"type": "string"},
        "action": {"type": "string", "enum": ACTIONS},
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
                 effort: str | None = None):
        # Reads your key from the ANTHROPIC_API_KEY environment variable.
        self.client = anthropic.Anthropic()
        self.mission = mission
        self.model = model or config.CLAUDE_MODEL
        self.effort = effort or config.CLAUDE_EFFORT

    def decide(self, jpeg: bytes, distance_cm: float | None,
               history: list[str], step: int) -> Decision:
        if distance_cm is None:
            distance_text = "no distance sensor fitted"
        else:
            distance_text = f"{distance_cm:.0f} cm to the nearest object straight ahead"

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
                    f"Distance sensor: {distance_text}.\n\n"
                    f"Your recent moves (oldest first):\n{recent}\n\n"
                    "The photo above is what the camera sees right now. "
                    "Choose your next move."
                ),
            },
        ]

        response = self.client.beta.messages.create(
            model=self.model,
            max_tokens=16000,
            system=SYSTEM_PROMPT,
            messages=[{"role": "user", "content": user_content}],
            output_config={
                "effort": self.effort,
                "format": {"type": "json_schema", "schema": DECISION_SCHEMA},
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
