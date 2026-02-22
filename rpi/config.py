"""
config.py — Central configuration for RPi Zero 2 W + PoE Hat LED Matrix controller.

Hardware:
  - Raspberry Pi Zero 2 W (RP3A0, quad-core Cortex-A53 @ 1GHz, 512MB RAM)
  - PoE HAT (802.3af) providing power + wired Ethernet via USB or HAT
  - 64x32 HUB75 LED Matrix panel

HUB75 GPIO Wiring (BCM numbering) — Adafruit RGB Matrix Bonnet/HAT pinout:
  Using the 'adafruit-hat' hardware mapping for compatibility with
  Adafruit RGB Matrix Bonnet/HAT pinout:

    Control Pins:
      OE  = GPIO 18  (physical pin 12)  — Output Enable
      CLK = GPIO 17  (physical pin 11)  — Clock
      LAT = GPIO 4   (physical pin 7)   — Latch

    Address Pins (Row Select):
      A   = GPIO 22  (physical pin 15)  — Address A (1->32, 1->16, or 1->8 mux)
      B   = GPIO 23  (physical pin 16)  — Address B (1->32, 1->16, or 1->8 mux)
      C   = GPIO 24  (physical pin 18)  — Address C (1->32, 1->16, or 1->8 mux)
      D   = GPIO 25  (physical pin 22)  — Address D (1->32 or 1->16 mux only)
      E   = GPIO 15  (physical pin 10, RxD)  — Address E (1->64 mux, 64px tall only)

    Port 1 Color Pins:
      R1  = GPIO 11  (physical pin 23)  — Red top half
      G1  = GPIO 27  (physical pin 13)  — Green top half
      B1  = GPIO 7   (physical pin 26)  — Blue top half
      R2  = GPIO 8   (physical pin 24)  — Red bottom half
      G2  = GPIO 9   (physical pin 21)  — Green bottom half
      B2  = GPIO 10  (physical pin 19)  — Blue bottom half

  This pinout is optimized for Adafruit RGB Matrix Bonnet/HAT and is
  confirmed working with this hardware configuration.
"""

# ──────────────────────────────────────────────────────────────────────────────
# Matrix hardware
# ──────────────────────────────────────────────────────────────────────────────
MATRIX_WIDTH   = 64
MATRIX_HEIGHT  = 32
MATRIX_CHAIN   = 1       # Number of panels chained
MATRIX_PARALLEL = 1      # Number of parallel chains
# Scan rate: 32px tall panel = 1/16 scan (set automatically by library)
MATRIX_HARDWARE_MAPPING = "adafruit-hat"   # regular, adafruit-hat, adafruit-hat-pwm
MATRIX_GPIO_SLOWDOWN    = 2           # 0–4; Controls LED refresh rate
                                      # 0 = Fastest (~1000Hz+) - BEST for reducing flicker
                                      # 1 = Fast (~500Hz) - Good balance for Pi Zero 2 W
                                      # 2 = Good balance (~250-300Hz)
                                      # 3 = Slower (~200Hz) - very stable
                                      # 4 = Slowest (~150Hz) - most stable but may appear dim
MATRIX_BRIGHTNESS       = 50          # 0–100 percent (library uses percent, not 0-255)
MATRIX_PWM_BITS        = 7           # 1-11; PWM bits for color depth (11=2048 levels, default)
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
}

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
