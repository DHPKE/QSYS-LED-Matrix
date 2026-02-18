"""
config.py — Central configuration for RPi Zero 2 W + PoE Hat LED Matrix controller.

Hardware:
  - Raspberry Pi Zero 2 W (RP3A0, quad-core Cortex-A53 @ 1GHz, 512MB RAM)
  - PoE HAT (802.3af) providing power + wired Ethernet via USB or HAT
  - 64x32 HUB75 LED Matrix panel

HUB75 GPIO Wiring (BCM numbering) — rpi-rgb-led-matrix defaults:
  The rpi-rgb-led-matrix library has well-tested default pin mappings.
  Using the 'regular' (non-adafruit-hat) mapping:

    R1  = GPIO 5   (physical pin 29)
    G1  = GPIO 13  (physical pin 33)
    B1  = GPIO 6   (physical pin 31)
    R2  = GPIO 12  (physical pin 32)
    G2  = GPIO 16  (physical pin 36)
    B2  = GPIO 23  (physical pin 16)
    A   = GPIO 22  (physical pin 15)
    B   = GPIO 26  (physical pin 37)
    C   = GPIO 27  (physical pin 13)
    D   = GPIO 20  (physical pin 38)
    E   = GPIO 24  (physical pin 18)  — only needed for 1/64 scan (64px tall panels)
    CLK = GPIO 17  (physical pin 11)
    LAT = GPIO 4   (physical pin 7)
    OE  = GPIO 18  (physical pin 12)

  All 40 GPIO pins remain available — wired Ethernet is via the PoE HAT
  USB/SPI path, NOT via silicon RMII, so there is NO GPIO conflict.

PoE HAT notes:
  - Most PoE HATs for Pi Zero use a small fan on GPIO 4 & 14 for cooling.
  - If your HAT uses GPIO 4 for the fan, change LAT to a different free pin
    and update both this file AND your physical wiring.
  - Check your HAT datasheet. The Waveshare PoE HAT (B) uses GPIO 26/35 for
    the fan — both of which ARE used above. Adjust as needed.
"""

# ──────────────────────────────────────────────────────────────────────────────
# Matrix hardware
# ──────────────────────────────────────────────────────────────────────────────
MATRIX_WIDTH   = 64
MATRIX_HEIGHT  = 32
MATRIX_CHAIN   = 1       # Number of panels chained
MATRIX_PARALLEL = 1      # Number of parallel chains
# Scan rate: 32px tall panel = 1/16 scan (set automatically by library)
MATRIX_HARDWARE_MAPPING = "regular"   # or "adafruit-hat", "adafruit-hat-pwm"
MATRIX_GPIO_SLOWDOWN    = 2           # 0–4; increase if display flickers (Zero 2 = 2)
MATRIX_BRIGHTNESS       = 50          # 0–100 percent (library uses percent, not 0-255)

# ──────────────────────────────────────────────────────────────────────────────
# Segments
# ──────────────────────────────────────────────────────────────────────────────
MAX_SEGMENTS = 4

# Default segment layout: side-by-side split (1|2)
DEFAULT_SEGMENTS = [
    {"id": 0, "x": 0,  "y": 0, "w": MATRIX_WIDTH // 2, "h": MATRIX_HEIGHT,
     "text": "", "color": "#FFFFFF", "bgcolor": "#000000",
     "align": "C", "effect": "none", "active": True},
    {"id": 1, "x": MATRIX_WIDTH // 2, "y": 0, "w": MATRIX_WIDTH // 2, "h": MATRIX_HEIGHT,
     "text": "", "color": "#FFFFFF", "bgcolor": "#000000",
     "align": "C", "effect": "none", "active": False},
    {"id": 2, "x": 0,  "y": MATRIX_HEIGHT // 2, "w": MATRIX_WIDTH // 2, "h": MATRIX_HEIGHT // 2,
     "text": "", "color": "#FFFFFF", "bgcolor": "#000000",
     "align": "C", "effect": "none", "active": False},
    {"id": 3, "x": MATRIX_WIDTH // 2, "y": MATRIX_HEIGHT // 2, "w": MATRIX_WIDTH // 2, "h": MATRIX_HEIGHT // 2,
     "text": "", "color": "#FFFFFF", "bgcolor": "#000000",
     "align": "C", "effect": "none", "active": False},
]

# ──────────────────────────────────────────────────────────────────────────────
# Network
# ──────────────────────────────────────────────────────────────────────────────
UDP_PORT       = 21324
UDP_BIND_ADDR  = "0.0.0.0"
WEB_PORT       = 80        # Change to 8080 if running without sudo

# ──────────────────────────────────────────────────────────────────────────────
# Text rendering
# ──────────────────────────────────────────────────────────────────────────────
MAX_TEXT_LENGTH    = 128
DEFAULT_SCROLL_SPEED = 50   # pixels per second
FONT_PATH          = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FONT_PATH_FALLBACK = "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf"

# ──────────────────────────────────────────────────────────────────────────────
# Persistence
# ──────────────────────────────────────────────────────────────────────────────
CONFIG_FILE  = "/var/lib/led-matrix/config.json"
SEGMENT_FILE = "/var/lib/led-matrix/segments.json"

# ──────────────────────────────────────────────────────────────────────────────
# Logging
# ──────────────────────────────────────────────────────────────────────────────
LOG_LEVEL = "INFO"   # DEBUG | INFO | WARNING | ERROR
