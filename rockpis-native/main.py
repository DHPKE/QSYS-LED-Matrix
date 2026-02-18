#!/usr/bin/env python3
"""
main.py — Entry point for the RADXA Rock Pi S LED Matrix controller.
NATIVE DRIVER VERSION - Uses direct GPIO access via gpiod instead of rpi-rgb-led-matrix

Waveshare RGB-Matrix-P4-64x32 Specifications:
  - 64×32 pixels (2048 RGB LEDs)
  - 4mm pitch, 256×128mm dimensions
  - 1/16 scan mode (HUB75 protocol)
  - 5V 4A power supply, ≤20W consumption
  - Viewing angle ≥160°

Architecture:
  ┌─────────────────┐
  │  main thread    │  render loop: update effects → render segments → swap buffers
  ├─────────────────┤
  │  refresh thread │  HUB75 driver: continuous row scanning at ~600 Hz
  ├─────────────────┤
  │  udp-listener   │  daemon thread: recvfrom() → dispatch() → SegmentManager
  ├─────────────────┤
  │  web-server     │  daemon thread: GET/POST HTTP → snapshot()
  └─────────────────┘

Requirements (install on the Rock Pi S):
  sudo apt install python3-gpiod python3-pillow fonts-dejavu-core

Run:
  sudo python3 main.py
  (sudo / root required for GPIO access)
"""

import logging
import signal
import sys
import time

from config import (
    MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_BRIGHTNESS,
    LOG_LEVEL, UDP_PORT, WEB_PORT,
)
from hub75_driver import HUB75Driver
from segment_manager import SegmentManager
from text_renderer import TextRenderer
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
    """Convert 0-255 protocol brightness to 0-255 native driver value."""
    return max(0, min(255, value_255))


def main():
    logger.info("=" * 60)
    logger.info("RADXA Rock Pi S — LED Matrix Controller (Native Driver)")
    logger.info("Waveshare RGB-Matrix-P4-64x32 (1/16 scan, HUB75)")
    logger.info("=" * 60)
    logger.info(f"Matrix: {MATRIX_WIDTH}×{MATRIX_HEIGHT}")
    logger.info(f"UDP port: {UDP_PORT},  Web port: {WEB_PORT}")

    # ── 1. Shared state ──────────────────────────────────────────────────
    sm = SegmentManager()

    # ── 2. Initialise the native HUB75 driver ────────────────────────────
    driver = HUB75Driver(width=MATRIX_WIDTH, height=MATRIX_HEIGHT)
    
    if not driver.start():
        logger.error("Failed to initialize display driver")
        return 1
    
    logger.info(f"✓ Native HUB75 driver started")
    
    # Set initial brightness
    driver.set_brightness(_brightness_255_to_pct(MATRIX_BRIGHTNESS))
    
    # ── 3. Text renderer (uses Pillow, writes to driver's back buffer) ────
    renderer = TextRenderer(driver, sm)
    logger.info("✓ Text renderer initialised")

    # ── 4. UDP listener ──────────────────────────────────────────────────
    udp_handler = UDPHandler(sm, UDP_PORT)
    udp_handler.start()
    logger.info(f"✓ UDP handler listening on port {UDP_PORT}")

    # ── 5. Web server ────────────────────────────────────────────────────
    web_server = WebServer(sm, driver, WEB_PORT)
    web_server.start()
    logger.info(f"✓ Web server started on port {WEB_PORT}")

    # ── Graceful shutdown handler ─────────────────────────────────────────
    shutdown_requested = False

    def signal_handler(sig, frame):
        nonlocal shutdown_requested
        logger.info(f"\n⚠  Signal {sig} received — shutting down...")
        shutdown_requested = True

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    # ── Main render loop ──────────────────────────────────────────────────
    logger.info("=" * 60)
    logger.info("Main loop running. Press Ctrl+C to exit.")
    logger.info("=" * 60)
    
    last_effect_time = time.time()
    frame_count = 0
    last_fps_time = time.time()

    try:
        while not shutdown_requested:
            now = time.time()

            # ── Apply any pending effects ────────────────────────────────
            if now - last_effect_time >= EFFECT_INTERVAL:
                sm.update_effects()
                last_effect_time = now

            # ── Render all segments to back buffer ───────────────────────
            renderer.render()
            
            # ── Swap buffers (display what we just rendered) ─────────────
            driver.swap_buffers()

            # ── Calculate render FPS (different from refresh rate) ───────
            frame_count += 1
            if now - last_fps_time >= 5.0:
                fps = frame_count / (now - last_fps_time)
                status = driver.get_status()
                logger.info(f"Status: Render {fps:.1f} FPS, "
                           f"Display {status['refresh_rate']} Hz, "
                           f"Brightness {status['brightness']}/255")
                frame_count = 0
                last_fps_time = now

            # ── Small delay to target ~20 FPS rendering ──────────────────
            time.sleep(max(0.001, EFFECT_INTERVAL - (time.time() - now)))

    except KeyboardInterrupt:
        logger.info("\n⚠  Keyboard interrupt received")
    except Exception as exc:
        logger.exception(f"⚠  Unhandled exception in main loop: {exc}")
    finally:
        logger.info("Cleaning up...")
        
        # Stop all services
        udp_handler.stop()
        web_server.stop()
        driver.stop()
        
        logger.info("✓ Shutdown complete")
        return 0


if __name__ == "__main__":
    sys.exit(main())
