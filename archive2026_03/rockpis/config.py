"""
config.py — Central configuration for RADXA Rock Pi S LED Matrix controller.

Hardware:
  - RADXA Rock Pi S v1.3 (RK3308, quad-core Cortex-A35 @ 1.3 GHz, 512 MB DDR3)
  - Built-in 100 Mbps Ethernet (IP101G PHY, RGMII — does NOT consume any GPIO header pins)
  - PoE power via a passive/active PoE splitter on the Ethernet cable (~5V/2A)
  - 64×32 HUB75 LED Matrix panel

────────────────────────────────────────────────────────────────────────────────
RK3308 GPIO numbering:
  The RK3308 names GPIOs as GPIOx_Py where:
    x  = bank number  (0–4)
    y  = pin  in bank (A0..D7, i.e. 0..31 within the bank)
  Linux sysfs / libgpiod number = x*32 + offset_within_bank
  Offset encoding:  A=0-7, B=8-15, C=16-23, D=24-31

  Rock Pi S v1.3 — 26-pin GPIO header (1-indexed physical pins):
  ┌────┬───────────────┬──────────────────────┬──────┐
  │ Phy│ Name          │ RK3308 GPIO          │ Linux│
  ├────┼───────────────┼──────────────────────┼──────┤
  │  1 │ 3V3           │  —                   │  —   │
  │  2 │ 5V            │  —                   │  —   │
  │  3 │ I2C0_SDA      │ GPIO0_B3             │   11 │
  │  4 │ 5V            │  —                   │  —   │
  │  5 │ I2C0_SCL      │ GPIO0_B4             │   12 │
  │  6 │ GND           │  —                   │  —   │
  │  7 │ GPIO0_B6      │ GPIO0_B6             │   14 │
  │  8 │ UART0_TX      │ GPIO0_B5 / UART0_TX  │   13 │
  │  9 │ GND           │  —                   │  —   │
  │ 10 │ UART0_RX      │ GPIO0_B6 / UART0_RX  │   14 │
  │ 11 │ GPIO0_C0      │ GPIO0_C0             │   16 │
  │ 12 │ GPIO0_C1      │ GPIO0_C1             │   17 │
  │ 13 │ GPIO0_C2      │ GPIO0_C2             │   18 │
  │ 14 │ GND           │  —                   │  —   │
  │ 15 │ GPIO0_C3      │ GPIO0_C3             │   19 │
  │ 16 │ GPIO0_C4      │ GPIO0_C4             │   20 │
  │ 17 │ 3V3           │  —                   │  —   │
  │ 18 │ GPIO0_C5      │ GPIO0_C5             │   21 │
  │ 19 │ SPI0_MOSI     │ GPIO0_C7 / SPI0_MOSI │   23 │
  │ 20 │ GND           │  —                   │  —   │
  │ 21 │ SPI0_MISO     │ GPIO0_D0 / SPI0_MISO │   24 │
  │ 22 │ GPIO0_C6      │ GPIO0_C6             │   22 │
  │ 23 │ SPI0_CLK      │ GPIO0_D1 / SPI0_CLK  │   25 │
  │ 24 │ SPI0_CS0      │ GPIO0_D2 / SPI0_CS0  │   26 │
  │ 25 │ GND           │  —                   │  —   │
  │ 26 │ SPI0_CS1      │ GPIO0_D3 / SPI0_CS1  │   27 │
  └────┴───────────────┴──────────────────────┴──────┘

  Linux GPIO numbers used for HUB75 (Header 1 ONLY - all pins available):
  ┌──────────┬──────────────┬──────────────┬──────────────────────────────────┐
  │ HUB75    │ RK3308 GPIO  │ Linux number │ Physical header pin               │
  ├──────────┼──────────────┼──────────────┼──────────────────────────────────┤
  │ R1       │ GPIO0_C0     │  16          │ 13                                │
  │ G1       │ GPIO0_C1     │  17          │ 15                                │
  │ B1       │ GPIO0_B7     │  15          │ 11                                │
  │ R2       │ GPIO2_A4     │  68          │  7                                │
  │ G2       │ GPIO2_A5     │  69          │ 12                                │
  │ B2       │ GPIO2_B2     │  74          │ 16                                │
  │ A (addr) │ GPIO0_B3     │  11          │  3                                │
  │ B (addr) │ GPIO0_B4     │  12          │  5                                │
  │ C (addr) │ GPIO2_A1     │  65          │  8  (UART0_TX — disable console!) │
  │ D (addr) │ GPIO2_A0     │  64          │ 10  (UART0_RX — disable console!) │
  │ CLK      │ GPIO2_A7     │  71          │ 22                                │
  │ LAT/STR  │ GPIO1_C7     │  55          │ 19                                │
  │ OE       │ GPIO1_C6     │  54          │ 21                                │
  └──────────┴──────────────┴──────────────┴──────────────────────────────────┘

  ⚠  NOTE ON UART0 CONFLICT (pins 8/10):
     GPIO0_B5 (Linux 13) and GPIO0_B6 (Linux 14) double as UART0_TX/RX which
     is the default Armbian serial console.  You MUST disable the serial
     console before using these pins for HUB75:

       sudo systemctl disable --now serial-getty@ttyS0
       In /boot/armbianEnv.txt, remove or comment out: console=ttyS0,1500000

     Alternatively, use MATRIX_HARDWARE_MAPPING = "pinout" and define a
     custom hardware mapping (see rpi-rgb-led-matrix lib/hardware-mapping.c).

  ⚠  NOTE ON rpi-rgb-led-matrix SUPPORT:
     rpi-rgb-led-matrix targets RPi hardware natively. On Armbian/RK3308 you
     MUST build with:
         make HARDWARE_DESC=armbian-gpio  (if your Armbian build supports it)
     OR pass --led-no-hardware-pulse at runtime (disables hardware PWM assist,
     slight increase in flicker/CPU but fully functional for text display).
     The Python bindings build and run fine on any ARM Cortex-A Linux with
     /dev/gpiomem or /dev/mem access.

     Command line: sudo python3 main.py --led-no-hardware-pulse
                   (already set in main.py via options.disable_hardware_pulsing)

  ⚠  NOTE ON GPIO SLOWDOWN:
     The RK3308 Cortex-A35 @ 1.3 GHz is faster than an RPi Zero 2 W.
     Start with MATRIX_GPIO_SLOWDOWN = 2.  If display shows garbage, try 3 or 4.

────────────────────────────────────────────────────────────────────────────────
"""

# ──────────────────────────────────────────────────────────────────────────────
# Matrix hardware
# ──────────────────────────────────────────────────────────────────────────────
MATRIX_WIDTH    = 64
MATRIX_HEIGHT   = 32
MATRIX_CHAIN    = 1       # Number of panels chained
MATRIX_PARALLEL = 1       # Number of parallel chains
# Scan rate: 32px tall panel = 1/16 scan (set automatically by library)

# rpi-rgb-led-matrix hardware mapping.
# IMPORTANT: After running build-with-hardware-mapping.sh, change this to "rockpis"
# The custom hardware mapping must be compiled into the library first.
# Default "regular" will cause bus errors on Rock Pi S.
MATRIX_HARDWARE_MAPPING = "rockpis"  # Custom Rock Pi S GPIO mapping (after library rebuild)

# GPIO slowdown — increase if the panel shows garbage / flickering.
# Rock Pi S (Cortex-A35 @ 1.3 GHz) is faster than RPi Zero; start at 2.
MATRIX_GPIO_SLOWDOWN = 2

# Initial brightness 0–100 (library percent).
MATRIX_BRIGHTNESS = 50

# Disable hardware PWM pulse (required on non-RPi SBCs; avoids /dev/mem BCM
# PWM peripheral access which doesn't exist on RK3308).
# This means the OE- signal is bit-banged, which works fine for text display.
MATRIX_DISABLE_HW_PULSING = True

# ──────────────────────────────────────────────────────────────────────────────
# HUB75 GPIO assignments (Linux sysfs / libgpiod numbers)
# Updated to use ONLY Header 1 pins - all 13 signals available on Header 1!
# Linux GPIO# = bank×32 + offset (A=0-7, B=8-15, C=16-23, D=24-31)
# ──────────────────────────────────────────────────────────────────────────────
GPIO_R1  = 16   # GPIO0_C0   physical pin 13   (upper half red)
GPIO_G1  = 17   # GPIO0_C1   physical pin 15   (upper half green)
GPIO_B1  = 15   # GPIO0_B7   physical pin 11   (upper half blue)
GPIO_R2  = 68   # GPIO2_A4   physical pin  7   (lower half red)
GPIO_G2  = 69   # GPIO2_A5   physical pin 12   (lower half green)
GPIO_B2  = 74   # GPIO2_B2   physical pin 16   (lower half blue)
GPIO_A   = 11   # GPIO0_B3   physical pin  3   (row address A)
GPIO_B   = 12   # GPIO0_B4   physical pin  5   (row address B)
GPIO_C   = 65   # GPIO2_A1   physical pin  8   (row address C) ⚠ UART0_TX!
GPIO_D   = 64   # GPIO2_A0   physical pin 10   (row address D) ⚠ UART0_RX!
GPIO_CLK = 71   # GPIO2_A7   physical pin 22   (shift clock)
GPIO_LAT = 55   # GPIO1_C7   physical pin 19   (latch/strobe)
GPIO_OE  = 54   # GPIO1_C6   physical pin 21   (output enable)

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
MAX_TEXT_LENGTH      = 128
DEFAULT_SCROLL_SPEED = 50    # pixels per second
FONT_PATH            = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FONT_PATH_FALLBACK   = "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf"

# ──────────────────────────────────────────────────────────────────────────────
# Persistence
# ──────────────────────────────────────────────────────────────────────────────
CONFIG_FILE  = "/var/lib/led-matrix/config.json"
SEGMENT_FILE = "/var/lib/led-matrix/segments.json"

# ──────────────────────────────────────────────────────────────────────────────
# Logging
# ──────────────────────────────────────────────────────────────────────────────
LOG_LEVEL = "INFO"   # DEBUG | INFO | WARNING | ERROR
