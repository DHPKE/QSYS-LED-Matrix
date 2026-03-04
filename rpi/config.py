"""
config.py — Central configuration for RPi Zero 2 W + PoE Hat LED Matrix controller.

Version: 7.0.0 (Curtain Mode)

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
MATRIX_GPIO_SLOWDOWN    = 3           # 0–4; Controls LED refresh rate
                                      # RPi 4: Use 1-2 for best balance
                                      # RPi Zero 2 W: Use 3 for maximum stability
                                      # 0 = Fastest (~1000Hz+) - may cause glitches
                                      # 1 = Fast (~500Hz) - high refresh, may glitch
                                      # 2 = Balanced (~250-300Hz) - BEST for RPi 4
                                      # 3 = Slower (~200Hz) - maximum stability, no flicker
MATRIX_BRIGHTNESS       = 50          # 0–100 percent (library uses percent, not 0-255)
MATRIX_PWM_BITS        = 8           # 1-11; PWM bits for color depth (11=2048 levels, default)
                                      # Lower values = faster refresh but less color accuracy
                                      # 11 = Best color (slower refresh)
                                      # 8 = Excellent compromise (256 levels, stable)
                                      # 7-9 = Good compromise (128 levels per channel)
                                      # 6 = Faster, less glitches, still good colors
                                      # 5 = Fast refresh, minimal glitches, acceptable colors
                                      # 1-4 = Very fast but visibly reduced colors
MATRIX_SCAN_MODE        = 0           # 0 = progressive (default), 1 = interlaced
                                      # 0 = progressive for crisp, stable display
MATRIX_ROW_ADDRESS_TYPE = 0           # 0-4; Different panels use different addressing
                                      # 0 = default (direct), 1 = AB-address panels
                                      # 2 = direct row select, 3 = ABC-addressed
                                      # Try different values if flickering persists
MATRIX_MULTIPLEXING     = 0           # 0-18; Panel multiplexing mode
                                      # 0 = default (best for most panels)
                                      # Try 1,2,3,4 if lines flicker
                                      # Different panels use different modes
MATRIX_PWM_DITHER_BITS  = 0           # 0 = off; 1-2 = dithering for smoother color
                                      # 0 = no dithering for stable, flicker-free display
MATRIX_LED_RGB_SEQUENCE = "RGB"      # Color order: RGB, RBG, GRB, GBR, BRG, BGR
                                      # Try if colors look wrong
MATRIX_REFRESH_LIMIT    = 200         # Hz; 0 = no limit, 200 = stable refresh
                                      # Limiting refresh reduces flicker and power usage

# ──────────────────────────────────────────────────────────────────────────────
# Display Orientation & Rotation
# ──────────────────────────────────────────────────────────────────────────────
ORIENTATION = "landscape"  # "landscape" (64×32) or "portrait" (32×64)
                            # Portrait mode rotates the display 90° clockwise
                            # Can be changed via WebUI, QSYS plugin, or UDP command
                            # NOTE: When orientation changes, Layout 1 (fullscreen) is
                            # automatically applied to ensure segment dimensions match
                            # the new canvas size. Use layout presets or send new
                            # segment configs after changing orientation.

ROTATION = 0                # Display rotation in degrees: 0, 90, 180, 270
                            # Rotates the entire display output
                            # 0   = Normal (no rotation)
                            # 90  = Rotate 90° clockwise
                            # 180 = Upside down
                            # 270 = Rotate 90° counter-clockwise (270° CW)
                            # Can be changed dynamically via UDP or WebUI
                            # No restart required (uses PIL Image.rotate)

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
# Curtain Mode (v7.0+)
# ──────────────────────────────────────────────────────────────────────────────
# Curtain mode creates a 2-pixel frame around the entire display.
#
# When curtain mode is active for a group:
# - 2-pixel frame rendered on all four edges (top, right, bottom, left)
# - Inner display area: (2,2) to (61,29) on 64x32 panel
# - Frame is rendered on top of all segments (highest z-index)
#
# Segments 1-4 positions are NOT automatically remapped - they render as configured,
# with the frame appearing as an overlay on top.
#
# Usage: Configure curtain for specific groups (e.g., groups 1,4,6)
# When panel switches to those groups, curtain frame automatically shows.

CURTAIN_WIDTH = 2          # Width of frame border in pixels (used for compatibility)
CURTAIN_DEFAULT_COLOR = (255, 255, 255)  # Default color (white) RGB tuple

# Curtain remap mode: deprecated (segments no longer auto-remapped)
CURTAIN_AUTO_REMAP = False  # False = segments stay as-is, frame overlays

# Curtain state per group (1-8) - persisted to storage
# Format: {group_id: {"enabled": bool, "color": (R,G,B)}}
CURTAIN_STATE = {}

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
FALLBACK_IP      = "10.20.30.40"
FALLBACK_NETMASK = "255.255.255.0"
FALLBACK_GATEWAY = "10.20.30.1"
FALLBACK_IFACE   = "eth0"      # Network interface to configure
DHCP_TIMEOUT_S   = 15          # Seconds to wait for DHCP before applying fallback

# ──────────────────────────────────────────────────────────────────────────────
# Text rendering
# ──────────────────────────────────────────────────────────────────────────────
MAX_TEXT_LENGTH    = 128
DEFAULT_SCROLL_SPEED = 50   # pixels per second

# Font mapping (Q-SYS plugin font names -> font file paths)
# Two fonts available: "arial" (sans-serif) and "digital" (Mono Regular - clean monospace)
# Backward compatibility aliases: digital12 -> digital, verdana -> arial
FONT_PATHS = {
    "arial":     "/usr/share/fonts/truetype/nimbus/NimbusSanL-Bold.otf",
    "digital":   "/usr/share/fonts/truetype/sevensegment/MonoRegular.ttf",
    "digital12": "/usr/share/fonts/truetype/sevensegment/MonoRegular.ttf",  # Alias for digital
    "verdana":   "/usr/share/fonts/truetype/nimbus/NimbusSanL-Bold.otf",  # Alias for arial
}

# Fallback font if requested font not found
FONT_FALLBACK = "/usr/share/fonts/truetype/nimbus/NimbusSanL-Bold.otf"

# Font stroke width for bold rendering (0 = no stroke, 1-2 = bolder)
FONT_STROKE_WIDTH = {
    "arial":     0,  # No stroke - Nimbus Sans L Bold is already bold enough
    "digital":   0,  # No stroke for seven-segment (crisp rendering)
    "digital12": 0,
    "verdana":   0,
}
FONT_STROKE_WIDTH_DEFAULT = 0  # Default: no stroke

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
#  Layout 1  — Fullscreen         [seg0: full 64×32]
#  Layout 2  — Top/Bottom halves  [seg0: top, seg1: bottom]
#  Layout 3  — Left/Right halves  [seg0: left, seg1: right]
#  Layout 4  — Triple left        [seg0: left half, seg1: right-top, seg2: right-bottom]
#  Layout 5  — Triple right       [seg0: left-top, seg1: left-bottom, seg2: right half]
#  Layout 6  — Thirds vertical    [seg0 | seg1 | seg2]
#  Layout 7  — Quad view          [seg0: TL, seg1: TR, seg2: BL, seg3: BR]
#  Layout 11 — Segment 0 only     [seg0 fullscreen, others hidden]
#  Layout 12 — Segment 1 only     [seg1 fullscreen, others hidden]
#  Layout 13 — Segment 2 only     [seg2 fullscreen, others hidden]
#  Layout 14 — Segment 3 only     [seg3 fullscreen, others hidden]
#  Layout 15 — VO-left            [seg1: 5/6 width left (53×32), seg3: quarter BR (32×16)]
#  Layout 16 — VO-right           [seg2: 1/2 width, 1/3 height top-right (32×11), seg3: quarter BR (32×16)]
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
    # Voice-over layouts for video production
    15: [(0,        0,         1,     1    ),                            # segment 0 hidden (1x1)
         (0,        0,         (5*W)//6, H  ),                           # segment 1: 5/6 width left (53×32)
         (0,        0,         1,     1    ),                            # segment 2 hidden (1x1)
         (W//2,     H//2,      W//2,  H//2 )],                           # segment 3: quarter BR (32×16)
    16: [(0,        0,         1,     1    ),                            # segment 0 hidden (1x1)
         (0,        0,         1,     1    ),                            # segment 1 hidden (1x1)
         (W//2,     0,         W//2,  H//3 ),                            # segment 2: 1/2 width, 1/3 height top-right (32×11)
         (W//2,     H//2,      W//2,  H//2 )],                           # segment 3: quarter BR (32×16)
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
    # Voice-over layouts for video production (portrait mode)
    15: [(0,         0,         1,      1     ),                         # segment 0 hidden (1x1)
         (0,         0,         PW,     (5*PH)//6),                      # segment 1: 5/6 height top (32×53)
         (0,         0,         1,      1     ),                         # segment 2 hidden (1x1)
         (PW//2,     PH//2,     PW//2,  PH//2 )],                        # segment 3: quarter BR (16×32)
    16: [(0,         0,         1,      1     ),                         # segment 0 hidden (1x1)
         (0,         0,         1,      1     ),                         # segment 1 hidden (1x1)
         (0,         0,         PW//2,  PH//3 ),                         # segment 2: 1/2 width, 1/3 height top-left (16×21)
         (PW//2,     PH//2,     PW//2,  PH//2 )],                        # segment 3: quarter BR (16×32)
}

# ──────────────────────────────────────────────────────────────────────────────
# Logging
# ──────────────────────────────────────────────────────────────────────────────
LOG_LEVEL = "INFO"   # DEBUG | INFO | WARNING | ERROR
                        # Use WARNING or ERROR to reduce CPU overhead from logging

# ──────────────────────────────────────────────────────────────────────────────
# Display refresh rate
# ──────────────────────────────────────────────────────────────────────────────
EFFECT_INTERVAL = 0.05  # seconds (20 fps for smooth updates, matches main loop)
                        # Increase to reduce CPU usage, decrease for smoother animations
                        # Recommended: 0.05 (20fps) to 0.15 (7fps)
                        # Lower = less flicker from CPU interruptions
