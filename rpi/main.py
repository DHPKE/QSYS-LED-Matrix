#!/usr/bin/env python3
"""
main.py — Entry point for the RPi Zero 2 W LED Matrix controller.

Port of src/main.cpp (ESP32 Arduino firmware) to Python / Linux.

Architecture:
  ┌─────────────────┐
  │  main thread    │  render loop: update effects → render segments → swap canvas
  ├─────────────────┤
  │  udp-listener   │  daemon thread: recvfrom() → dispatch() → SegmentManager
  ├─────────────────┤
  │  web-server     │  daemon thread: GET/POST HTTP → dispatch() / snapshot()
  └─────────────────┘

Requirements (install on the Pi):
  sudo apt install python3-pillow fonts-dejavu-core
  # rpi-rgb-led-matrix Python bindings — build from source:
  # https://github.com/hzeller/rpi-rgb-led-matrix#building
  # After building:  sudo make install-python  (or pip install ./ in bindings/python)

Run:
  sudo python3 main.py
  (sudo required for low-level GPIO DMA; or use --led-no-hardware-pulse for testing)
"""

import logging
import signal
import sys
import time

from config import (
    MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_CHAIN, MATRIX_PARALLEL,
    MATRIX_HARDWARE_MAPPING, MATRIX_GPIO_SLOWDOWN, MATRIX_BRIGHTNESS,
    MATRIX_PWM_BITS, LOG_LEVEL, UDP_PORT, WEB_PORT, EFFECT_INTERVAL,
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


def _brightness_255_to_pct(value_255: int) -> int:
    """Convert 0-255 protocol brightness to 0-100 library percent."""
    return max(0, min(100, int(value_255 * 100 / 255)))


def main():
    logger.info("=" * 50)
    logger.info("RPi Zero 2 W — LED Matrix Controller")
    logger.info("=" * 50)
    logger.info(f"Matrix: {MATRIX_WIDTH}×{MATRIX_HEIGHT}, chain={MATRIX_CHAIN}")
    logger.info(f"UDP port: {UDP_PORT},  Web port: {WEB_PORT}")

    # ── 1. Shared state ──────────────────────────────────────────────────
    sm = SegmentManager()

    # ── 2. Try to initialise the real LED matrix ──────────────────────────
    matrix   = None
    canvas   = None
    renderer = None

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
        options.pwm_bits            = MATRIX_PWM_BITS
        options.pwm_lsb_nanoseconds = 200  # Higher = smoother but slower (default 130)
        # The rpi-rgb-led-matrix hardware pulse uses the same BCM PWM peripheral
        # as the on-board sound (snd_bcm2835). If that module is loaded the
        # library hard-exits with "fix the above first or use --led-no-hardware-pulse".
        # Setting this True uses bit-banged OE- instead — fully functional for
        # text display, no flicker visible at normal viewing distances.
        # To get hardware pulsing back: blacklist snd_bcm2835 (see install.sh).
        options.disable_hardware_pulsing = True  # Software PWM - more stable for Pi Zero 2 W
        options.show_refresh_rate   = False
        options.limit_refresh_rate_hz = 0  # No limit - as fast as possible

        matrix = RGBMatrix(options=options)
        canvas = matrix.CreateFrameCanvas()
        renderer = TextRenderer(matrix, canvas, sm)
        logger.info("✓ LED matrix initialised")

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
    render_count = 0

    def _shutdown(sig, frame):
        logger.info("\nShutting down…")
        udp.stop()
        web.stop()
        if matrix:
            matrix.Clear()
        sys.exit(0)

    signal.signal(signal.SIGINT,  _shutdown)
    signal.signal(signal.SIGTERM, _shutdown)

    # Optimized render loop with minimal overhead
    while True:
        now = time.monotonic()

        # Update effects at fixed interval
        if now - last_effect >= EFFECT_INTERVAL:
            sm.update_effects()
            last_effect = now

            if renderer:
                try:
                    renderer.render_all()
                except Exception as exc:
                    logger.error(f"[RENDER] Exception: {exc}")

        # Adaptive sleep to maintain framerate without busy-waiting
        elapsed = time.monotonic() - now
        sleep_for = max(0.001, EFFECT_INTERVAL - elapsed)
        time.sleep(sleep_for)


if __name__ == "__main__":
    main()
