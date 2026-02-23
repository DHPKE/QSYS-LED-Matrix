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
MATRIX_GPIO_SLOWDOWN    = 2           # 0–4; Controls LED refresh rate
                                      # RPi 4: Use 1-2 for best balance
                                      # RPi Zero 2 W: Use 2 or 3 (lower power)
                                      # 0 = Fastest (~1000Hz+) - may cause glitches
                                      # 1 = Fast (~500Hz) - high refresh, may glitch
                                      # 2 = Balanced (~250-300Hz) - BEST for RPi 4
                                      # 3 = Slower (~200Hz) - maximum stability
MATRIX_BRIGHTNESS       = 50          # 0–100 percent (library uses percent, not 0-255)
MATRIX_PWM_BITS        = 7           # 1-11; PWM bits for color depth (11=2048 levels, default)
                                      # Lower values = faster refresh but less color accuracy
                                      # 11 = Best color (slower refresh)
                                      # 7-9 = Good compromise (128 levels per channel)
                                      # 6 = Faster, less glitches, still good colors
                                      # 5 = Fast refresh, minimal glitches, acceptable colors
                                      # 1-4 = Very fast but visibly reduced colors
MATRIX_SCAN_MODE        = 0           # 0 = progressive (default), 1 = interlaced
                                      # Try 1 if you see line flickering
MATRIX_ROW_ADDRESS_TYPE = 0           # 0-4; Different panels use different addressing
                                      # 0 = default (direct), 1 = AB-address panels
                                      # 2 = direct row select, 3 = ABC-addressed
                                      # Try different values if flickering persists
MATRIX_MULTIPLEXING     = 0           # 0-18; Panel multiplexing mode
                                      # 0 = default (best for most panels)
                                      # Try 1,2,3,4 if lines flicker
                                      # Different panels use different modes
MATRIX_PWM_DITHER_BITS  = 0           # 0 = off; 1-2 = dithering for smoother color
                                      # Can reduce color banding with lower PWM bits
MATRIX_LED_RGB_SEQUENCE = "RGB"      # Color order: RGB, RBG, GRB, GBR, BRG, BGR
                                      # Try if colors look wrong
MATRIX_REFRESH_LIMIT    = 0           # Hz; 0 = no limit, 120-200 = limit refresh rate
                                      # Use if display looks oversaturated or unstable

# ──────────────────────────────────────────────────────────────────────────────
# Display Orientation
# ──────────────────────────────────────────────────────────────────────────────
ORIENTATION = "landscape"  # "landscape" (64×32) or "portrait" (32×64)
                            # Portrait mode rotates the display 90° clockwise
                            # Can be changed via WebUI, QSYS plugin, or UDP command
                            # NOTE: When orientation changes, Layout 1 (fullscreen) is
                            # automatically applied to ensure segment dimensions match
                            # the new canvas size. Use layout presets or send new
                            # segment configs after changing orientation.

# ──────────────────────────────────────────────────────────────────────────────
# Group Configuration
# ──────────────────────────────────────────────────────────────────────────────
# Allows grouping multiple LED panels for centralized control via QSYS Plugin
# Each panel can be assigned to a group (1-8) and will only respond to commands
# sent to that group. The group indicator is displayed as a small square in the
# bottom-left corner of the display (4×4 pixels).
#
# Group colors:
#   1 = White    5 = Magenta
#   2 = Yellow   6 = Blue
#   3 = Orange   7 = Cyan
#   4 = Red      8 = Green
#
# Set to 0 to disable grouping (panel responds to all commands)
# Set to 1-8 to assign panel to a specific group
GROUP_ID = 0  # 0 = no grouping, 1-8 = assigned group

GROUP_COLORS = {
    0: (0, 0, 0),          # No group (black/invisible)
    1: (255, 255, 255),    # White
    2: (255, 255, 0),      # Yellow
    3: (255, 165, 0),      # Orange
    4: (255, 0, 0),        # Red
    5: (255, 0, 255),      # Magenta
    6: (0, 0, 255),        # Blue
    7: (0, 255, 255),      # Cyan
    8: (0, 255, 0),        # Green
}

# Group indicator configuration
GROUP_INDICATOR_SIZE = 2   # Size of the group indicator square (2×2 pixels)
GROUP_INDICATOR_X = 0      # X position (bottom-left corner)
GROUP_INDICATOR_Y = -1     # Y position (-1 = auto-calculate bottom position)

# ──────────────────────────────────────────────────────────────────────────────
# Segments
# ──────────────────────────────────────────────────────────────────────────────
MAX_SEGMENTS = 4

# Default segment layout: fullscreen on segment 0, others inactive
# NOTE: These defaults are for LANDSCAPE mode (64×32).
# On startup, if orientation is portrait, Layout 1 will be auto-applied.
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
    # Single segment fullscreen layouts (for QSYS plugin presets 11-14)
    # Other segments set to 1x1 to hide them (segment index matches tuple position)
    11: [(0,        0,         W,     H    )],                           # segment 0 fullscreen only
    12: [(0,        0,         1,     1    ),                            # segment 0 hidden (1x1)
         (0,        0,         W,     H    )],                           # segment 1 fullscreen
    13: [(0,        0,         1,     1    ),                            # segment 0 hidden (1x1)
         (0,        0,         1,     1    ),                            # segment 1 hidden (1x1)
         (0,        0,         W,     H    )],                           # segment 2 fullscreen
    14: [(0,        0,         1,     1    ),                            # segment 0 hidden (1x1)
         (0,        0,         1,     1    ),                            # segment 1 hidden (1x1)
         (0,        0,         1,     1    ),                            # segment 2 hidden (1x1)
         (0,        0,         W,     H    )],                           # segment 3 fullscreen
}

# Portrait layout presets (32×64 virtual canvas)
# Coordinates: (x, y, w, h) for 32 wide × 64 tall display
#  Layout 1 — Fullscreen         [seg0: full 32×64]
#  Layout 2 — Top/Bottom halves  [seg0: top, seg1: bottom]
#  Layout 3 — Left/Right halves  [seg0: left, seg1: right]
#  Layout 4 — Triple top         [seg0: top half, seg1: bottom-left, seg2: bottom-right]
#  Layout 5 — Triple bottom      [seg0: top-left, seg1: top-right, seg2: bottom half]
#  Layout 6 — Thirds horizontal  [seg0 — seg1 — seg2]
#  Layout 7 — Quad view          [seg0: TL, seg1: TR, seg2: BL, seg3: BR]
PW = MATRIX_HEIGHT  # Portrait width = 32
PH = MATRIX_WIDTH   # Portrait height = 64

LAYOUT_PRESETS_PORTRAIT = {
    1: [(0,          0,         PW,     PH    )],                        # fullscreen
    2: [(0,          0,         PW,     PH//2 ),                         # top half
        (0,          PH//2,     PW,     PH//2 )],                        # bottom half
    3: [(0,          0,         PW//2,  PH    ),                         # left half
        (PW//2,      0,         PW//2,  PH    )],                        # right half
    4: [(0,          0,         PW,     PH//2 ),                         # top half
        (0,          PH//2,     PW//2,  PH//2 ),                         # bottom-left quarter
        (PW//2,      PH//2,     PW//2,  PH//2 )],                        # bottom-right quarter
    5: [(0,          0,         PW//2,  PH//2 ),                         # top-left quarter
        (PW//2,      0,         PW//2,  PH//2 ),                         # top-right quarter
        (0,          PH//2,     PW,     PH//2 )],                        # bottom half
    6: [(0,          0,         PW,     PH//3      ),                    # top third  (21px)
        (0,          PH//3,     PW,     PH//3      ),                    # middle third (21px)
        (0,          2*(PH//3), PW,     PH-2*(PH//3))],                  # bottom third  (22px)
    7: [(0,          0,         PW//2,  PH//2 ),                         # top-left
        (PW//2,      0,         PW//2,  PH//2 ),                         # top-right
        (0,          PH//2,     PW//2,  PH//2 ),                         # bottom-left
        (PW//2,      PH//2,     PW//2,  PH//2 )],                        # bottom-right
    # Single segment fullscreen layouts (for QSYS plugin presets 11-14)
    # Other segments set to 1x1 to hide them (segment index matches tuple position)
    11: [(0,         0,         PW,     PH    )],                        # segment 0 fullscreen only
    12: [(0,         0,         1,      1     ),                         # segment 0 hidden (1x1)
         (0,         0,         PW,     PH    )],                        # segment 1 fullscreen
    13: [(0,         0,         1,      1     ),                         # segment 0 hidden (1x1)
         (0,         0,         1,      1     ),                         # segment 1 hidden (1x1)
         (0,         0,         PW,     PH    )],                        # segment 2 fullscreen
    14: [(0,         0,         1,      1     ),                         # segment 0 hidden (1x1)
         (0,         0,         1,      1     ),                         # segment 1 hidden (1x1)
         (0,         0,         1,      1     ),                         # segment 2 hidden (1x1)
         (0,         0,         PW,     PH    )],                        # segment 3 fullscreen
}

# ──────────────────────────────────────────────────────────────────────────────
# Logging
# ──────────────────────────────────────────────────────────────────────────────
LOG_LEVEL = "WARNING"   # DEBUG | INFO | WARNING | ERROR
                        # Use WARNING or ERROR to reduce CPU overhead from logging

# ──────────────────────────────────────────────────────────────────────────────
# Display refresh rate
# ──────────────────────────────────────────────────────────────────────────────
EFFECT_INTERVAL = 0.1   # seconds (10 fps for reduced CPU load)
                        # Increase to reduce CPU usage, decrease for smoother animations
                        # Recommended: 0.05 (20fps) to 0.15 (7fps)
                        # Lower = less flicker from CPU interruptions
