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
MATRIX_HARDWARE_MAPPING = "regular"   # regular, adafruit-hat, adafruit-hat-pwm
MATRIX_GPIO_SLOWDOWN    = 1           # 0–4; Controls LED refresh rate
                                      # 0 = Fastest (~1000Hz+) - BEST for reducing flicker
                                      # 1 = Fast (~500Hz) - Good balance for Pi Zero 2 W
                                      # 2 = Good balance (~250-300Hz)
                                      # 3 = Slower (~200Hz) - very stable
                                      # 4 = Slowest (~150Hz) - most stable but may appear dim
MATRIX_BRIGHTNESS       = 50          # 0–100 percent (library uses percent, not 0-255)
MATRIX_PWM_BITS        = 8           # 1-11; PWM bits for color depth (11=2048 levels, default)
                                      # Lower values = faster refresh but less color accuracy
                                      # 11 = Best color (slower refresh)
                                      # 7-9 = Good compromise
                                      # 1-6 = Faster but reduced colors

# ──────────────────────────────────────────────────────────────────────────────
# Segments
# ──────────────────────────────────────────────────────────────────────────────
MAX_SEGMENTS = 4

# Default segment layout: fullscreen on segment 0, others inactive
DEFAULT_SEGMENTS = [
    {"id": 0, "x": 0,  "y": 0, "w": MATRIX_WIDTH, "h": MATRIX_HEIGHT,
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
WEB_PORT       = 8080      # Non-privileged port (no sudo required)

# Fallback static IP applied when DHCP gives no address within DHCP_TIMEOUT_S.
# Set FALLBACK_IP = None to disable (device stays unreachable instead).
FALLBACK_IP      = "192.168.1.200"
FALLBACK_NETMASK = "255.255.255.0"
FALLBACK_GATEWAY = "192.168.1.1"
FALLBACK_IFACE   = "eth0"      # Network interface to configure
DHCP_TIMEOUT_S   = 15          # Seconds to wait for DHCP before applying fallback

# ──────────────────────────────────────────────────────────────────────────────
# Text rendering
# ──────────────────────────────────────────────────────────────────────────────
MAX_TEXT_LENGTH    = 128
DEFAULT_SCROLL_SPEED = 50   # pixels per second
FONT_PATH          = "/usr/share/fonts/truetype/msttcorefonts/Arial_Bold.ttf"
FONT_PATH_FALLBACK = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

# ──────────────────────────────────────────────────────────────────────────────
# Persistence
# ──────────────────────────────────────────────────────────────────────────────
CONFIG_FILE  = "/var/lib/led-matrix/config.json"
SEGMENT_FILE = "/var/lib/led-matrix/segments.json"

# ──────────────────────────────────────────────────────────────────────────────
# Layout presets  (mirrors ESP32 udp_handler.h applyLayoutPreset)
#
# Each entry is a list of (x, y, w, h) tuples, one per active segment.
# Segments beyond the list length are deactivated (w=0).
#
#  Layout 1 — Fullscreen         [seg0: full 64×32]
#  Layout 2 — Top/Bottom halves  [seg0: top, seg1: bottom]
#  Layout 3 — Left/Right halves  [seg0: left, seg1: right]
#  Layout 4 — Triple left        [seg0: left half, seg1: right-top, seg2: right-bottom]
#  Layout 5 — Triple right       [seg0: left-top, seg1: left-bottom, seg2: right half]
#  Layout 6 — Thirds vertical    [seg0 | seg1 | seg2]
#  Layout 7 — Quad view          [seg0: TL, seg1: TR, seg2: BL, seg3: BR]
# ──────────────────────────────────────────────────────────────────────────────
W = MATRIX_WIDTH
H = MATRIX_HEIGHT

LAYOUT_PRESETS = {
    1: [(0,         0,         W,     H    )],                           # fullscreen
    2: [(0,         0,         W,     H//2 ),                            # top half
        (0,         H//2,      W,     H//2 )],                           # bottom half
    3: [(0,         0,         W//2,  H    ),                            # left half
        (W//2,      0,         W//2,  H    )],                           # right half
    4: [(0,         0,         W//2,  H    ),                            # left half
        (W//2,      0,         W//2,  H//2 ),                            # right-top quarter
        (W//2,      H//2,      W//2,  H//2 )],                           # right-bottom quarter
    5: [(0,         0,         W//2,  H//2 ),                            # left-top quarter
        (0,         H//2,      W//2,  H//2 ),                            # left-bottom quarter
        (W//2,      0,         W//2,  H    )],                           # right half
    6: [(0,         0,         W//3,      H    ),                        # left third  (21px)
        (W//3,      0,         W//3,      H    ),                        # middle third (21px)
        (2*(W//3),  0,         W-2*(W//3),H    )],                       # right third  (22px)
    7: [(0,         0,         W//2,  H//2 ),                            # top-left
        (W//2,      0,         W//2,  H//2 ),                            # top-right
        (0,         H//2,      W//2,  H//2 ),                            # bottom-left
        (W//2,      H//2,      W//2,  H//2 )],                           # bottom-right
    # 11-14: single-segment fullscreen — only the named segment is active
    11: [(0, 0, W, H)],                                                  # seg 0 fullscreen
    12: [(0, 0, W, H)],                                                  # seg 1 fullscreen
    13: [(0, 0, W, H)],                                                  # seg 2 fullscreen
    14: [(0, 0, W, H)],                                                  # seg 3 fullscreen
}

# Map preset numbers 11-14 to the segment index that should be active
LAYOUT_SINGLE_SEG = {11: 0, 12: 1, 13: 2, 14: 3}

# ──────────────────────────────────────────────────────────────────────────────
# Logging
# ──────────────────────────────────────────────────────────────────────────────
LOG_LEVEL = "INFO"   # DEBUG | INFO | WARNING | ERROR

# ──────────────────────────────────────────────────────────────────────────────
# Display refresh rate
# ──────────────────────────────────────────────────────────────────────────────
EFFECT_INTERVAL = 0.05   # seconds (≈ 20 fps; matrix library itself runs at ~120Hz)
                        # Increase to reduce CPU usage, decrease for smoother animations
                        # Recommended: 0.05 (20fps) to 0.1 (10fps)
