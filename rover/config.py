"""
All the settings for your rover live here.

You WILL edit this file: after you calibrate (step "Calibrate" in the guide),
copy the numbers that `python -m rover calibrate` prints into the CALIBRATION
section below.
"""

# ---------------------------------------------------------------------------
# Claude
# ---------------------------------------------------------------------------
# Which Claude model drives the rover. claude-opus-5-5 is the smartest and
# the default. If you want cheaper / faster steps, you can try
# "claude-sonnet-5-5" or "claude-haiku-5-5" -- see docs/05-claude-brain.md.
CLAUDE_MODEL = "claude-opus-5-5"

# How hard Claude thinks before each move: "low", "medium" or "high".
# "low" = faster and cheaper; good enough for most driving.
CLAUDE_EFFORT = "low"

# Safety net: the mission stops after this many moves no matter what.
MAX_STEPS = 40

# How many previous moves Claude is reminded of on each step (its "memory").
MEMORY_STEPS = 8

# ---------------------------------------------------------------------------
# Camera
# ---------------------------------------------------------------------------
# Small images = fewer tokens = cheaper + faster. 640x480 is plenty.
IMAGE_WIDTH = 640
IMAGE_HEIGHT = 480
# Set to True if your photos come out upside down (camera mounted flipped).
CAMERA_UPSIDE_DOWN = False
# Which kind of camera (Option A / C):
#   "pi"  = a ribbon-cable Raspberry Pi camera (Camera Module 3, OV5647, ...)
#   "usb" = an ordinary USB webcam plugged into the Pi
#           (needs: sudo apt install -y python3-opencv)
CAMERA_TYPE = "pi"
USB_CAMERA_INDEX = 0   # 0 = the first webcam plugged in
# Told to Claude so it can judge distances. Measure yours!
CAMERA_HEIGHT_CM = 10        # Option C with a camera mast: maybe 20-25
CAMERA_FOV_DEGREES = 60      # ~60 for standard cameras, ~120 for wide-angle
ROVER_LENGTH_CM = 25

# ---------------------------------------------------------------------------
# Motors (Raspberry Pi wiring, BCM GPIO numbers -- see docs/02-build-option-a-pi.md)
# ---------------------------------------------------------------------------
LEFT_MOTOR_IN1 = 17   # TB6612 AIN1  -> physical pin 11
LEFT_MOTOR_IN2 = 27   # TB6612 AIN2  -> physical pin 13
LEFT_MOTOR_PWM = 22   # TB6612 PWMA  -> physical pin 15
RIGHT_MOTOR_IN1 = 23  # TB6612 BIN1  -> physical pin 16
RIGHT_MOTOR_IN2 = 24  # TB6612 BIN2  -> physical pin 18
RIGHT_MOTOR_PWM = 25  # TB6612 PWMB  -> physical pin 22

# If a side spins backwards when it should go forward, flip its flag
# (or swap the two motor wires on the driver board -- same effect).
LEFT_MOTOR_REVERSED = False
RIGHT_MOTOR_REVERSED = False

# 0.0 - 1.0. Slower is easier to control and gives better photos.
DRIVE_SPEED = 0.6
TURN_SPEED = 0.7

# ---------------------------------------------------------------------------
# Distance sensor (ultrasonic) -- the rover's "reflex"
# ---------------------------------------------------------------------------
USE_DISTANCE_SENSOR = True
ULTRASONIC_TRIGGER = 5   # physical pin 29
ULTRASONIC_ECHO = 6      # physical pin 31
# If something is closer than this while driving forward, stop immediately,
# no matter what Claude said.
SAFETY_STOP_CM = 20

# ---------------------------------------------------------------------------
# Headlights
# ---------------------------------------------------------------------------
# Option A / C: white LEDs on a GPIO pin (see docs/02-build-option-a-pi.md,
# Phase 8). Option B: the ESP32-CAM's built-in flash LED (always available).
USE_HEADLIGHTS = True
HEADLIGHT_PIN = 26        # physical pin 37 (GND is right next to it, pin 39)

# During missions CLAUDE decides when to switch the headlights on and off.
# It is told whether they are on and how bright each photo is
# (0 = black ... 255 = white).
#
# Optional safety net: set AUTO_LIGHTS = True to also switch them on
# automatically whenever a photo is darker than DARK_THRESHOLD.
AUTO_LIGHTS = False
DARK_THRESHOLD = 60

# ---------------------------------------------------------------------------
# CALIBRATION -- run `python -m rover calibrate` and paste the results here
# ---------------------------------------------------------------------------
# How many centimetres the rover moves in 1 second at DRIVE_SPEED.
CM_PER_SECOND = 30.0
# How many degrees the rover spins in 1 second at TURN_SPEED.
DEGREES_PER_SECOND = 180.0

# Limits on a single move, so one bad decision can't send the rover far.
MAX_MOVE_CM = 60
MAX_TURN_DEGREES = 180

# ---------------------------------------------------------------------------
# Option B (ESP32-CAM): the rover's address on your WiFi.
# The ESP32 prints it on the Serial Monitor when it boots. You can also pass
# --esp32-url on the command line instead of editing this.
# ---------------------------------------------------------------------------
ESP32_URL = "http://192.168.1.50"
