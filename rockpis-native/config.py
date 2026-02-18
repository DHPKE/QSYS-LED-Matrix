"""
config.py — Configuration for RADXA Rock Pi S LED Matrix controller (Native Driver)

Hardware:
  - Waveshare RGB-Matrix-P4-64x32 (64×32 pixels, 4mm pitch, 1/16 scan)
  - RADXA Rock Pi S v1.3 (RK3308, Cortex-A35 @ 1.3 GHz, 512MB RAM)
  - Direct GPIO access via libgpiod (no rpi-rgb-led-matrix library)
"""

# ─── Matrix Configuration ─────────────────────────────────────────────────
MATRIX_WIDTH = 64
MATRIX_HEIGHT = 32
MATRIX_BRIGHTNESS = 200  # 0-255 (start at ~78%)

# ─── Network Configuration ────────────────────────────────────────────────
UDP_PORT = 21324         # UDP JSON protocol
UDP_BIND_ADDR = "0.0.0.0"  # Bind to all interfaces
WEB_PORT = 80            # HTTP web interface

# ─── Logging ──────────────────────────────────────────────────────────────
LOG_LEVEL = "INFO"  # DEBUG, INFO, WARNING, ERROR

# ─── Segment Configuration ────────────────────────────────────────────────
MAX_SEGMENTS = 4
MAX_TEXT_LENGTH = 256
DEFAULT_SCROLL_SPEED = 30  # pixels per second
DEFAULT_SEGMENTS = {
    1: {"x": 0, "y": 0, "w": 64, "h": 8},
    2: {"x": 0, "y": 8, "w": 64, "h": 8},
    3: {"x": 0, "y": 16, "w": 64, "h": 8},
    4: {"x": 0, "y": 24, "w": 64, "h": 8},
}

# ─── GPIO Pin Assignments ─────────────────────────────────────────────────
# These are defined in gpio_config.py and used by hub75_driver.py
# All pins on Rock Pi S Header 1 for clean wiring
#
# ⚠️  IMPORTANT: Pins 8 (GPIO 65) and 10 (GPIO 64) are UART0 TX/RX
# The serial console MUST be disabled:
#   sudo systemctl disable --now serial-getty@ttyS0.service
#   Edit /boot/armbianEnv.txt: remove console=ttyS0
#   Reboot

# For reference (actual definitions in gpio_config.py):
# R1=GPIO16, G1=GPIO17, B1=GPIO15
# R2=GPIO68, G2=GPIO69, B2=GPIO74
# A=GPIO11, B=GPIO12, C=GPIO65, D=GPIO64
# CLK=GPIO71, LAT=GPIO55, OE=GPIO54
