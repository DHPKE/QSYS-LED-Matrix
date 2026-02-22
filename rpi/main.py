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
import subprocess
import sys
import time

from config import (
    MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_CHAIN, MATRIX_PARALLEL,
    MATRIX_HARDWARE_MAPPING, MATRIX_GPIO_SLOWDOWN, MATRIX_BRIGHTNESS,
    MATRIX_PWM_BITS, MATRIX_SCAN_MODE, MATRIX_ROW_ADDRESS_TYPE,
    MATRIX_MULTIPLEXING, MATRIX_PWM_DITHER_BITS, MATRIX_LED_RGB_SEQUENCE,
    MATRIX_REFRESH_LIMIT, LOG_LEVEL, UDP_PORT, WEB_PORT, EFFECT_INTERVAL,
    FALLBACK_IP, FALLBACK_NETMASK, FALLBACK_GATEWAY, FALLBACK_IFACE,
    DHCP_TIMEOUT_S,
)
from segment_manager import SegmentManager
from udp_handler import UDPHandler, _load_config
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


# ─── Network helpers ─────────────────────────────────────────────────────────

def _get_ip(iface: str = "eth0") -> str:
    """Return the current IPv4 address of *iface*, or '' if none / not found."""
    try:
        import socket as _socket
        import fcntl
        import struct
        SIOCGIFADDR = 0x8915
        s = _socket.socket(_socket.AF_INET, _socket.SOCK_DGRAM)
        try:
            packed = fcntl.ioctl(s.fileno(), SIOCGIFADDR,
                                 struct.pack("256s", iface[:15].encode()))
            return _socket.inet_ntoa(packed[20:24])
        finally:
            s.close()
    except Exception:
        pass
    # Fallback: parse `ip addr show`
    try:
        out = subprocess.check_output(
            ["ip", "-4", "addr", "show", iface],
            stderr=subprocess.DEVNULL, text=True)
        for line in out.splitlines():
            line = line.strip()
            if line.startswith("inet "):
                return line.split()[1].split("/")[0]
    except Exception:
        pass
    return ""


def _apply_fallback_ip(ip: str, netmask: str, gateway: str, iface: str):
    """Configure a static IP address via `ip` commands (requires root)."""
    try:
        import socket as _socket
        import struct
        prefix = bin(struct.unpack(">I",
                     _socket.inet_aton(netmask))[0]).count("1")
    except Exception:
        prefix = 24
    try:
        subprocess.run(["ip", "addr", "flush", "dev", iface],
                       check=True, stderr=subprocess.DEVNULL)
        subprocess.run(["ip", "addr", "add", f"{ip}/{prefix}", "dev", iface],
                       check=True, stderr=subprocess.DEVNULL)
        subprocess.run(["ip", "link", "set", iface, "up"],
                       check=True, stderr=subprocess.DEVNULL)
        subprocess.run(["ip", "route", "add", "default", "via", gateway],
                       check=False, stderr=subprocess.DEVNULL)
        logger.info(f"✓ Fallback IP applied: {ip}/{prefix} gw {gateway} on {iface}")
    except subprocess.CalledProcessError as e:
        logger.error(f"⚠  Could not apply fallback IP: {e}")


def _wait_for_dhcp(iface: str, timeout_s: float) -> str:
    """Poll for a non-loopback IPv4 address on *iface* for up to *timeout_s* seconds."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        ip = _get_ip(iface)
        if ip and not ip.startswith("127."):
            return ip
        time.sleep(1.0)
    return ""


def _ensure_network() -> str:
    """
    Wait for DHCP on FALLBACK_IFACE; apply static fallback if it times out.
    Returns the IP address string to display on-screen.
    """
    logger.info(f"[NET] Waiting up to {DHCP_TIMEOUT_S}s for DHCP on {FALLBACK_IFACE}…")
    ip = _wait_for_dhcp(FALLBACK_IFACE, DHCP_TIMEOUT_S)
    if ip:
        logger.info(f"[NET] ✓ DHCP address: {ip}")
        return ip

    logger.warning(f"[NET] No DHCP lease after {DHCP_TIMEOUT_S}s")
    if FALLBACK_IP:
        logger.info(f"[NET] Applying fallback static IP: {FALLBACK_IP}")
        _apply_fallback_ip(FALLBACK_IP, FALLBACK_NETMASK, FALLBACK_GATEWAY,
                           FALLBACK_IFACE)
        return FALLBACK_IP

    logger.warning("[NET] FALLBACK_IP is None — device may be unreachable")
    return "no IP"


def main():
    logger.info("=" * 50)
    logger.info("RPi Zero 2 W — LED Matrix Controller")
    logger.info("=" * 50)
    logger.info(f"Matrix: {MATRIX_WIDTH}×{MATRIX_HEIGHT}, chain={MATRIX_CHAIN}")
    logger.info(f"UDP port: {UDP_PORT},  Web port: {WEB_PORT}")

    # ── 1. Network: wait for DHCP / apply fallback ────────────────────────
    device_ip = _ensure_network()

    # ── 2. Shared state ──────────────────────────────────────────────────
    sm = SegmentManager()

    # ── 3. Try to initialise the real LED matrix ──────────────────────────
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
        options.pwm_lsb_nanoseconds = 200  # Balanced timing for RPi 4 (130 too fast, caused glitches)
        options.scan_mode           = MATRIX_SCAN_MODE
        options.row_address_type    = MATRIX_ROW_ADDRESS_TYPE
        options.multiplexing        = MATRIX_MULTIPLEXING
        options.pwm_dither_bits     = MATRIX_PWM_DITHER_BITS
        options.led_rgb_sequence    = MATRIX_LED_RGB_SEQUENCE
        options.limit_refresh_rate_hz = MATRIX_REFRESH_LIMIT
        # Software PWM — avoids conflict with snd_bcm2835 BCM PWM peripheral.
        # To use hardware pulsing: blacklist snd_bcm2835 in /etc/modprobe.d/
        options.disable_hardware_pulsing = True
        options.show_refresh_rate   = False
        # Additional stability options
        options.inverse_colors      = False
        options.daemon              = 0   # 0 = don't drop privileges

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

    # ── 4. Brightness and orientation callbacks ──────────────────────────
    def on_brightness_change(value_255: int):
        if matrix:
            matrix.brightness = _brightness_255_to_pct(value_255)
            logger.info(f"[MAIN] Brightness → {matrix.brightness}%")
    
    def on_orientation_change(orientation: str):
        logger.info(f"[MAIN] Orientation → {orientation}")
        # Reapply the same layout preset for the new orientation
        # This ensures segment dimensions match the new canvas size while preserving layout choice
        if udp:
            current_layout = udp.get_current_layout()
            udp._apply_layout(current_layout)
            logger.info(f"[MAIN] Reapplied Layout {current_layout} for {orientation} mode")
        else:
            # During startup, just mark dirty
            sm.mark_all_dirty()

    # ── 5. Load saved configuration ───────────────────────────────────────
    _load_config()

    # ── 6. Start UDP listener ────────────────────────────────────────────
    udp = UDPHandler(sm, 
                     brightness_callback=on_brightness_change,
                     orientation_callback=on_orientation_change)
    udp.start()
    
    # Apply the correct layout based on loaded orientation
    # This ensures segments match the canvas dimensions on startup
    from udp_handler import get_orientation
    initial_orientation = get_orientation()
    logger.info(f"[MAIN] Initial orientation: {initial_orientation}")
    udp._apply_layout(1)  # Apply Layout 1 (fullscreen) for current orientation

    # ── 7. Start web server ──────────────────────────────────────────────
    web = WebServer(sm, udp)
    web.start()

    # ── 8. IP address splash screen ──────────────────────────────────────
    # Display the device IP on segment 0 (full-screen, white on black) until
    # the first UDP command arrives — mirrors the ESP32 firmware behaviour.
    ip_splash_active = True
    sm.update_text(0, device_ip, color="FFFFFF", bgcolor="000000", align="C")
    logger.info(f"[SPLASH] Showing IP address: {device_ip}")

    logger.info("=" * 50)
    logger.info("System ready — press Ctrl+C to stop")
    logger.info("=" * 50)

    # ── 9. Shutdown handler ──────────────────────────────────────────────
    def _shutdown(sig, frame):
        logger.info("\nShutting down…")
        udp.stop()
        web.stop()
        if matrix:
            matrix.Clear()
        sys.exit(0)

    signal.signal(signal.SIGINT,  _shutdown)
    signal.signal(signal.SIGTERM, _shutdown)

    # ── 10. Main render loop ──────────────────────────────────────────────
    last_effect = time.monotonic()

    while True:
        now = time.monotonic()

        # Dismiss IP splash on first received command.
        # Do NOT call sm.clear_all() here — the command has already written its
        # content into the target segment inside dispatch().  Wiping here would
        # race and blank the display until the next command arrives.
        if ip_splash_active and udp.has_received_command():
            ip_splash_active = False
            logger.info("[SPLASH] First command received — IP splash dismissed")

        # Update effects and render at fixed interval
        if now - last_effect >= EFFECT_INTERVAL:
            sm.update_effects()
            last_effect = now

            if renderer:
                try:
                    renderer.render_all()
                except Exception as exc:
                    logger.error(f"[RENDER] Exception: {exc}")

        # Longer sleep to reduce CPU load and give matrix library uninterrupted time
        time.sleep(0.05)  # 50ms sleep reduces CPU load significantly


if __name__ == "__main__":
    main()
