#!/usr/bin/env python3
"""
main.py — Entry point for the RADXA Rock Pi S LED Matrix controller.

Port of src/main.cpp (ESP32 Arduino firmware) to Python / Linux.

Key differences from the RPi Zero 2 W version (rpi/main.py):
  - MATRIX_DISABLE_HW_PULSING = True  (RK3308 has no BCM-compatible PWM
    hardware peripheral; we bit-bang OE- instead — fully functional for text)
  - MATRIX_GPIO_SLOWDOWN = 2  (Cortex-A35 @ 1.3 GHz — start at 2, try 3
    if the panel shows colour garbage)
  - Built-in 100 Mbps Ethernet (no PoE HAT needed)

Architecture:
  ┌─────────────────┐
  │  main thread    │  render loop: update effects → render segments → swap canvas
  ├─────────────────┤
  │  udp-listener   │  daemon thread: recvfrom() → dispatch() → SegmentManager
  ├─────────────────┤
  │  web-server     │  daemon thread: GET/POST HTTP → dispatch() / snapshot()
  └─────────────────┘

Requirements (install on the Rock Pi S):
  sudo apt install python3-pillow fonts-dejavu-core
  # rpi-rgb-led-matrix Python bindings — build from source:
  # https://github.com/hzeller/rpi-rgb-led-matrix
  # The install.sh script handles this automatically.

Run:
  sudo python3 main.py
  (sudo / root required for /dev/mem GPIO access)
"""

import logging
import os
import signal
import sys
import time

# CRITICAL: Force NO_DISPLAY mode if environment variable is set
# This prevents the library from trying to detect Pi model and crashing
# with bus error on non-Pi hardware
NO_DISPLAY = os.environ.get('LED_MATRIX_NO_DISPLAY', '').lower() in ('1', 'true', 'yes')

from config import (
    MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_CHAIN, MATRIX_PARALLEL,
    MATRIX_HARDWARE_MAPPING, MATRIX_GPIO_SLOWDOWN, MATRIX_BRIGHTNESS,
    MATRIX_DISABLE_HW_PULSING,
    LOG_LEVEL, UDP_PORT, WEB_PORT,
    GPIO_R1, GPIO_G1, GPIO_B1, GPIO_R2, GPIO_G2, GPIO_B2,
    GPIO_A, GPIO_B, GPIO_C, GPIO_D, GPIO_CLK, GPIO_LAT, GPIO_OE,
)
from segment_manager import SegmentManager
from udp_handler import UDPHandler
from web_server import WebServer

# ─── Logging ────────────────────────────────────────────────────────────────
logging.basicConfig(
    level=getattr(logging, LOG_LEVEL, logging.INFO),
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S",
)
logger = logging.getLogger(__name__)

# ─── Effect / render interval ───────────────────────────────────────────────
EFFECT_INTERVAL = 0.05   # seconds (≈ 20 fps)


def _brightness_255_to_pct(value_255: int) -> int:
    """Convert 0-255 protocol brightness to 0-100 library percent."""
    return max(0, min(100, int(value_255 * 100 / 255)))


def main():
    logger.info("=" * 50)
    logger.info("RADXA Rock Pi S — LED Matrix Controller")
    logger.info("=" * 50)
    logger.info(f"Matrix: {MATRIX_WIDTH}×{MATRIX_HEIGHT}, chain={MATRIX_CHAIN}")
    logger.info(f"UDP port: {UDP_PORT},  Web port: {WEB_PORT}")
    logger.info(f"HW pulsing disabled: {MATRIX_DISABLE_HW_PULSING}  "
                f"(expected True on RK3308)")

    # ── 1. Shared state ──────────────────────────────────────────────────
    sm = SegmentManager()

    # ── 2. Try to initialise the real LED matrix ──────────────────────────
    matrix   = None
    canvas   = None
    renderer = None

    if NO_DISPLAY:
        logger.warning("LED_MATRIX_NO_DISPLAY set — running in NO-DISPLAY test mode")
        logger.warning("  ↳ UDP and web server functional, but NO physical panel output")
    else:
        try:
            from rgbmatrix import RGBMatrix, RGBMatrixOptions
            from text_renderer import TextRenderer

            options = RGBMatrixOptions()
            options.rows                = MATRIX_HEIGHT
            options.cols                = MATRIX_WIDTH
            options.chain_length        = MATRIX_CHAIN
            options.parallel            = MATRIX_PARALLEL
            options.hardware_mapping    = MATRIX_HARDWARE_MAPPING
            options.gpio_slowdown       = MATRIX_GPIO_SLOWDOWN
            options.brightness          = MATRIX_BRIGHTNESS
            
            # ── Rock Pi S specific ──────────────────────────────────────────
            # The RK3308 does not have BCM-compatible PWM hardware that the
            # rpi-rgb-led-matrix library uses for the OE- signal on RPi.
            # Disabling hardware pulsing forces bit-banged OE-, which works
            # correctly and introduces only a very slight increase in CPU load.
            options.disable_hardware_pulsing = MATRIX_DISABLE_HW_PULSING
            
            # CRITICAL for non-Raspberry Pi boards:
            # The library tries to detect Pi model and crashes on non-Pi hardware.
            # drop_privileges=False prevents the Pi detection code from running.
            options.drop_privileges = False
            
            # Try to set explicit GPIO pins if the library version supports it
            # (Some builds don't expose gpio_* attributes in Python bindings)
            try:
                options.gpio_r1  = GPIO_R1
                options.gpio_g1  = GPIO_G1
                options.gpio_b1  = GPIO_B1
                options.gpio_r2  = GPIO_R2
                options.gpio_g2  = GPIO_G2
                options.gpio_b2  = GPIO_B2
                options.gpio_a   = GPIO_A
                options.gpio_b   = GPIO_B
                options.gpio_c   = GPIO_C
                options.gpio_d   = GPIO_D
                options.gpio_clk = GPIO_CLK
                options.gpio_lat = GPIO_LAT
                options.gpio_oe  = GPIO_OE
                logger.info("  ↳ Explicit GPIO pins set via options")
            except AttributeError:
                # GPIO attributes not available in this build - library will use
                # hardware_mapping defaults or custom hardware mapping file
                logger.info("  ↳ GPIO pin attributes not available in this library build")
                logger.info("  ↳ Using hardware_mapping='%s' (library defaults)", 
                           MATRIX_HARDWARE_MAPPING)
            
            options.show_refresh_rate   = False

            matrix = RGBMatrix(options=options)
            canvas = matrix.CreateFrameCanvas()
            renderer = TextRenderer(matrix, canvas, sm)
            logger.info("✓ LED matrix initialised")
            logger.info("  ↳ Hardware pulsing disabled (bit-bang OE- mode for RK3308)")
            logger.info("  ↳ Privilege drop disabled (non-Pi hardware mode)")

        except ImportError:
        logger.warning(
            "⚠  rgbmatrix module not found — running in NO_DISPLAY (virtual) mode.\n"
            "   Install rpi-rgb-led-matrix Python bindings to drive the physical panel.\n"
            "   The web UI and UDP handler are fully functional."
        )
    except Exception as exc:
        logger.error(f"⚠  LED matrix init failed: {exc} — falling back to NO_DISPLAY mode")

    # ── 3. Brightness callback ────────────────────────────────────────────
    def on_brightness_change(value_255: int):
        if matrix:
            matrix.brightness = _brightness_255_to_pct(value_255)
            logger.info(f"[MAIN] Brightness → {matrix.brightness}%")

    # ── 4. Start UDP listener ────────────────────────────────────────────
    udp = UDPHandler(sm, brightness_callback=on_brightness_change)
    udp.start()

    # ── 5. Start web server ──────────────────────────────────────────────
    web = WebServer(sm, udp)
    web.start()

    # ── 6. Display "READY" on segment 0 ─────────────────────────────────
    sm.update_text(0, "READY", color="00FF00", align="C")

    logger.info("=" * 50)
    logger.info("System ready — press Ctrl+C to stop")
    logger.info("=" * 50)

    # ── 7. Main render loop ──────────────────────────────────────────────
    last_effect = time.monotonic()

    def _shutdown(sig, frame):
        logger.info("\nShutting down…")
        udp.stop()
        web.stop()
        if matrix:
            matrix.Clear()
        sys.exit(0)

    signal.signal(signal.SIGINT,  _shutdown)
    signal.signal(signal.SIGTERM, _shutdown)

    while True:
        now = time.monotonic()

        if now - last_effect >= EFFECT_INTERVAL:
            sm.update_effects()
            last_effect = now

            if renderer:
                try:
                    renderer.render_all()
                    # canvas reference is updated inside render_all() after SwapOnVSync
                except Exception as exc:
                    logger.error(f"[RENDER] Exception: {exc}")

        # Sleep for the remainder of the interval
        elapsed = time.monotonic() - now
        sleep_for = max(0.001, EFFECT_INTERVAL - elapsed)
        time.sleep(sleep_for)


if __name__ == "__main__":
    main()
