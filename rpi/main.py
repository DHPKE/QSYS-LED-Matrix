#!/usr/bin/env python3
"""
main.py — Entry point for the RPi Zero 2 W LED Matrix controller.

Version: 7.0.18 (VO Maximized 1px Gap)

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
import socket
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
from curtain_manager import CurtainManager
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


def _get_first_up_ip() -> str:
    """
    Return the first non-loopback IPv4 address from an UP interface.
    Prefers eth interfaces, then wlan, then any other.
    Only returns IPs from interfaces that are actually UP (not DOWN or NO-CARRIER).
    """
    try:
        # Get list of all interfaces with their state
        out = subprocess.check_output(
            ["ip", "-4", "addr", "show"],
            stderr=subprocess.DEVNULL, text=True)
        
        interfaces = {}
        current_iface = None
        is_up = False
        
        logger.debug("[NET] Parsing interface list:")
        for line in out.splitlines():
            line_stripped = line.strip()
            
            # Line like: "2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 ... state UP ..."
            if not line.startswith(" ") and ":" in line:
                parts = line.split(":")
                if len(parts) >= 2:
                    current_iface = parts[1].strip()
                    # Check if interface is actually UP:
                    # Must have LOWER_UP (link is up) OR state UP
                    # Must NOT have NO-CARRIER or state DOWN
                    has_lower_up = "LOWER_UP" in line
                    has_state_up = "state UP" in line
                    has_no_carrier = "NO-CARRIER" in line
                    has_state_down = "state DOWN" in line
                    
                    is_up = (has_lower_up or has_state_up) and not has_no_carrier and not has_state_down
                    logger.debug(f"[NET]   {current_iface}: LOWER_UP={has_lower_up}, state_UP={has_state_up}, NO_CARRIER={has_no_carrier}, state_DOWN={has_state_down} → is_up={is_up}")
            
            # Line like: "    inet 10.1.1.25/24 ..."
            elif line_stripped.startswith("inet ") and current_iface:
                ip = line_stripped.split()[1].split("/")[0]
                logger.debug(f"[NET]   {current_iface} has IP {ip}, is_up={is_up}, loopback={ip.startswith('127.')}")
                if not ip.startswith("127.") and is_up:
                    interfaces[current_iface] = ip
                    logger.debug(f"[NET]   → Added {current_iface}: {ip}")
        
        logger.info(f"[NET] Found UP interfaces: {interfaces}")
        
        # Priority: eth* > wlan* > others
        for prefix in ["eth", "wlan", ""]:
            for iface in sorted(interfaces.keys()):
                if iface.startswith(prefix):
                    logger.info(f"[NET] Selected IP {interfaces[iface]} from {iface} (UP)")
                    return interfaces[iface]
        
        logger.warning("[NET] No UP interface with IP found")
        return ""
        
    except Exception as e:
        logger.error(f"[NET] Error scanning interfaces: {e}")
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
    Scan for first UP interface with IP address.
    If none found, wait for DHCP on FALLBACK_IFACE.
    Returns the IP address string to display on-screen.
    """
    # First, try to get IP from any UP interface
    ip = _get_first_up_ip()
    if ip:
        return ip
    
    # No UP interfaces yet, wait for DHCP on fallback interface
    logger.info(f"[NET] No UP interface found, waiting up to {DHCP_TIMEOUT_S}s for DHCP on {FALLBACK_IFACE}…")
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


def _network_monitor(sm: SegmentManager, udp: 'UDPHandler', ip_splash_callback):
    """
    Background thread: monitor network IP and update splash screen if it changes.
    Scans all UP interfaces for IP changes.
    Runs until the first UDP command is received, then exits.
    """
    import threading
    last_ip = _get_first_up_ip()
    logger.info(f"[NET-MON] Starting network monitor (current IP: {last_ip})")
    
    while True:
        time.sleep(5.0)  # Check every 5 seconds
        
        # Stop monitoring after first command
        if udp.has_received_command():
            logger.info("[NET-MON] First command received, stopping network monitor")
            break
            
        current_ip = _get_first_up_ip()
        
        # IP changed (and not empty)
        if current_ip and current_ip != last_ip:
            logger.info(f"[NET-MON] IP changed: {last_ip} → {current_ip}")
            last_ip = current_ip
            
            # Update splash screen with new IP
            sm.update_text(0, current_ip, color="FFFFFF", bgcolor="000000", align="C")
            sm.set_frame(0, enabled=True, color="FFFFFF", width=1)
            sm.mark_dirty(0)  # Trigger re-render
            logger.info(f"[NET-MON] Updated splash screen with new IP: {current_ip}")
            
            # Notify callback to update device_ip variable
            ip_splash_callback(current_ip)



def main():
    logger.info("=" * 50)
    logger.info("RPi Zero 2 W — LED Matrix Controller")
    logger.info("=" * 50)
    logger.info(f"Matrix: {MATRIX_WIDTH}×{MATRIX_HEIGHT}, chain={MATRIX_CHAIN}")
    logger.info(f"UDP port: {UDP_PORT},  Web port: {WEB_PORT}")

    # ── 1. Network: wait for DHCP / apply fallback ────────────────────────
    device_ip = _ensure_network()
    
    # Variable to track current IP (will be updated by monitor)
    current_ip_ref = [device_ip]  # Use list to allow updates from thread
    
    def update_device_ip(new_ip):
        """Callback to update device IP from network monitor"""
        current_ip_ref[0] = new_ip

    # ── 2. Shared state ──────────────────────────────────────────────────
    sm = SegmentManager()
    cm = CurtainManager()  # Curtain mode manager (v7.0+)

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
        options.pwm_lsb_nanoseconds = 200  # Original stable timing
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
        renderer = TextRenderer(matrix, canvas, sm, cm)  # Pass curtain manager
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

    def on_display_change(enabled: bool):
        nonlocal display_enabled, canvas, matrix
        logger.info(f"[MAIN] Display → {'ENABLED' if enabled else 'DISABLED'}")
        
        if not enabled:
            # Clear display when disabled
            display_enabled = False
            if canvas and matrix:
                canvas.Clear()
                canvas = matrix.SwapOnVSync(canvas)  # Swap cleared buffer to display
                logger.info("[MAIN] Display blanked (buffer swapped)")
            elif matrix:
                matrix.Clear()
                logger.info("[MAIN] Matrix cleared (no canvas)")
        else:
            # Re-enable display and force immediate re-render
            display_enabled = True
            sm.mark_all_dirty()
            logger.info("[MAIN] Marked all segments dirty for re-render")
            # Force immediate render (don't wait for next interval)
            if renderer:
                try:
                    renderer.render_all()
                    logger.info("[MAIN] Forced immediate re-render")
                except Exception as exc:
                    logger.error(f"[MAIN] Re-render exception: {exc}")
                except Exception as exc:
                    logger.error(f"[MAIN] Re-render exception: {exc}")

    # ── 5. Load saved configuration ───────────────────────────────────────
    from udp_handler import _load_config as load_udp_config
    load_udp_config()

    # ── 6. Start UDP listener ────────────────────────────────────────────
    def on_curtain_trigger(group: int, state: bool):
        """Callback for curtain boolean trigger"""
        cm.set_visibility(group, state)
        sm.mark_all_dirty()  # Force re-render
    
    def on_curtain_config(group: int, enabled: bool, color: str):
        """Callback for curtain configuration"""
        cm.configure(group, enabled, color)
        sm.mark_all_dirty()  # Force re-render
    
    udp = UDPHandler(sm, 
                     brightness_callback=on_brightness_change,
                     orientation_callback=on_orientation_change,
                     display_callback=on_display_change,
                     curtain_callback=on_curtain_trigger,
                     curtain_config_callback=on_curtain_config)
    udp.start()
    
    # Set global handler reference for cross-module access
    import udp_handler
    udp_handler._handler = udp
    
    # Apply the correct layout based on loaded orientation
    # This ensures segments match the canvas dimensions on startup
    from udp_handler import get_orientation
    initial_orientation = get_orientation()
    logger.info(f"[MAIN] Initial orientation: {initial_orientation}")

    # ── 7. Start web server ──────────────────────────────────────────────
    web = WebServer(sm, udp)
    web.start()

    # ── 8. IP address splash screen ──────────────────────────────────────
    # Display the device IP on segment 0 (full-screen, white on black) until
    # the first UDP command arrives — mirrors the ESP32 firmware behaviour.
    # Force rotation to 0° for the IP splash (ignore configured rotation).
    ip_splash_active = True
    
    # Save current rotation and temporarily set to 0° for layout application
    from udp_handler import get_rotation, set_rotation
    saved_rotation = get_rotation()
    set_rotation(0)  # Temporarily force 0° so layout uses landscape presets
    
    # Set renderer rotation override to 0°
    if renderer:
        renderer.set_rotation_override(0)
        logger.info("[SPLASH] Rotation override set to 0° (saved rotation: {}°)".format(saved_rotation))
    
    # Apply layout at 0° rotation (landscape presets)
    udp._apply_layout(1)  # Apply Layout 1 (fullscreen) for IP splash at 0°
    
    sm.update_text(0, current_ip_ref[0], color="FFFFFF", bgcolor="000000", align="C")
    # Frame removed for cleaner splash screen (v7.0.7)
    sm.mark_all_dirty()  # Mark for rendering
    
    # Force immediate render of IP splash at 0° rotation
    if renderer:
        try:
            renderer.render_all()
            logger.info(f"[SPLASH] Rendered IP splash at 0° rotation: {current_ip_ref[0]}")
        except Exception as exc:
            logger.error(f"[SPLASH] Render exception: {exc}")
    
    logger.info(f"[SPLASH] Showing IP address: {current_ip_ref[0]} (rotation locked to 0°)")
    
    # Start network monitor thread
    import threading
    net_monitor_thread = threading.Thread(
        target=_network_monitor,
        args=(sm, udp, update_device_ip),
        daemon=True
    )
    net_monitor_thread.start()

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
    test_mode_active = False
    test_mode_was_active = False
    test_bar_offset = 0
    test_cycle_state = 0
    last_cycle_switch = time.monotonic()
    last_hostname_fetch = time.monotonic()
    last_ip_fetch = time.monotonic()
    hostname = socket.gethostname()
    test_device_ip = current_ip_ref[0]
    frame_counter = 0
    
    # Store state before entering test mode for restoration
    saved_layout_preset = 1
    saved_segments_state = []
    
    # Display enable/disable state
    display_enabled = True
    
    # Watchdog: Auto-blank if no UDP commands received for 30 seconds
    last_udp_command_time = time.monotonic()
    watchdog_blanked = False
    WATCHDOG_TIMEOUT = 30.0  # seconds

    while True:
        now = time.monotonic()
        
        # ── Check for test mode ──────────────────────────────────────────
        try:
            with open("/tmp/led-matrix-testmode", "r") as f:
                test_mode_active = (f.read().strip() == "1")
        except FileNotFoundError:
            test_mode_active = False
        
        # Clear display and force 0° rotation when entering test mode
        if test_mode_active and not test_mode_was_active:
            logger.info("[TEST] Entering test mode - saving state, clearing display, forcing 0° rotation")
            
            # Save current layout preset (from UDP handler) and segment states
            saved_layout_preset = udp.get_current_layout() if udp else 1
            saved_segments_state = sm.get_all_segments_state()
            logger.info(f"[TEST] Saved layout preset: {saved_layout_preset}, segments: {len(saved_segments_state)}")
            
            # Clear all segment text
            sm.clear_all()
            
            # Deactivate ALL segments first (to prevent rendering over segment 0)
            for seg_id in range(4):
                sm.activate(seg_id, False)
            logger.info("[TEST] Deactivated all segments")
            
            if matrix:
                matrix.Clear()
            
            # Configure segment 0 for full-screen test mode display
            sm.configure(0, 0, 0, MATRIX_WIDTH, MATRIX_HEIGHT)
            sm.activate(0, True)
            sm.set_frame(0, enabled=False)
            logger.info("[TEST] Configured segment 0 for full-screen (64x32)")
            
            if renderer:
                renderer.set_rotation_override(0)  # Force 0° rotation for test mode
            test_bar_offset = 0
            test_cycle_state = 0
            last_cycle_switch = now
            frame_counter = 0
            # Force immediate refresh of hostname and IP
            hostname = socket.gethostname()
            test_device_ip = _get_first_up_ip() or "No IP"
            last_hostname_fetch = now
            last_ip_fetch = now
            logger.info(f"[TEST] Hostname: {hostname}, IP: {test_device_ip}")
        
        # Restore rotation, layout, and segments when exiting test mode
        if not test_mode_active and test_mode_was_active:
            logger.info("[TEST] Exiting test mode - restoring state")
            if matrix:
                matrix.Clear()  # Clear test mode artifacts
            if renderer:
                renderer.set_rotation_override(None)  # Restore configured rotation
            
            # Reapply saved layout preset
            logger.info(f"[TEST] Reapplying layout preset: {saved_layout_preset}")
            udp._apply_layout(saved_layout_preset)
            
            # Restore segment states
            if saved_segments_state:
                sm.restore_segments_state(saved_segments_state)
                logger.info(f"[TEST] Restored {len(saved_segments_state)} segments")
            
            # Mark all segments dirty to force re-render
            sm.mark_all_dirty()
            logger.info("[TEST] State restored, ready for normal operation")
        
        test_mode_was_active = test_mode_active
        
        # ── Test mode rendering ──────────────────────────────────────────
        if test_mode_active:
            # Refresh hostname every 10 seconds
            if now - last_hostname_fetch >= 10:
                hostname = socket.gethostname()
                last_hostname_fetch = now
            
            # Refresh IP every 10 seconds
            if now - last_ip_fetch >= 10:
                test_device_ip = _get_first_up_ip() or "No IP"
                last_ip_fetch = now
            
            # 4-state cycle: 0=hostname, 1=blank, 2=IP, 3=blank
            if now - last_cycle_switch >= 1.0:
                test_cycle_state = (test_cycle_state + 1) % 4
                last_cycle_switch = now
                
                # Update segment 0 text based on cycle state
                # Segment 0 is already configured as full-screen on entry to test mode
                if test_cycle_state == 0:
                    # Hostname
                    sm.activate(0, True)
                    sm.update_text(0, hostname, color="FFFFFF", bgcolor="010101", align="C")
                    logger.info(f"[TEST] Displaying hostname: '{hostname}'")
                elif test_cycle_state == 2:
                    # IP
                    sm.activate(0, True)
                    sm.update_text(0, test_device_ip, color="FFFFFF", bgcolor="010101", align="C")
                    logger.info(f"[TEST] Displaying IP: '{test_device_ip}'")
                else:
                    # Blank (deactivate segment, just show bars)
                    sm.activate(0, False)
                    logger.info(f"[TEST] Blank cycle (state {test_cycle_state})")
                # States 1 and 3 are blank (no text, just bars)
            
            # RENDER TEST MODE PATTERN
            # Import PIL here for test mode rendering
            from PIL import Image, ImageDraw
            
            # Create image for this frame
            test_img = Image.new("RGB", (MATRIX_WIDTH, MATRIX_HEIGHT), (0, 0, 0))
            
            # Draw 5 vertical color bars (full height)
            bar_width = MATRIX_WIDTH // 5  # 64/5 = ~13px per bar
            colors = [
                (255, 0, 0),    # Red
                (0, 255, 0),    # Green
                (0, 0, 255),    # Blue
                (255, 255, 0),  # Yellow
                (255, 255, 255) # White
            ]
            
            # Move bars every 2 frames for smooth animation
            frame_counter += 1
            if frame_counter % 2 == 0:
                test_bar_offset = (test_bar_offset + 1) % (bar_width * len(colors))
            
            # STEP 1: Draw color bars to image
            pixels = test_img.load()
            for x in range(MATRIX_WIDTH):
                bar_index = ((x + test_bar_offset) // bar_width) % len(colors)
                r, g, b = colors[bar_index]
                for y in range(MATRIX_HEIGHT):
                    pixels[x, y] = (r, g, b)
            
            # STEP 2: Render text on top of bars (only if text is active)
            if renderer and (test_cycle_state == 0 or test_cycle_state == 2):
                try:
                    # Get segment snapshots
                    segment_snapshots, any_dirty = sm.get_render_snapshot()
                    
                    # Temporarily use test_img as the renderer's image
                    old_image = renderer._image
                    old_draw = renderer._draw
                    renderer._image = test_img
                    renderer._draw = ImageDraw.Draw(test_img)
                    
                    # Render text segments with transparent background
                    for snap in segment_snapshots:
                        if snap['is_active'] and snap['width'] > 1 and snap['height'] > 1:
                            renderer._render_segment_from_snapshot(snap)
                    
                    # Restore renderer's original image
                    renderer._image = old_image
                    renderer._draw = old_draw
                    
                    # Clear dirty flags
                    sm.clear_dirty_flags()
                except Exception as exc:
                    logger.error(f"[TEST] Render exception: {exc}")
            
            # STEP 3: Send combined image to matrix
            if matrix and canvas:
                canvas.SetImage(test_img)
                canvas = matrix.SwapOnVSync(canvas)
            
            time.sleep(0.016)  # ~60fps for smooth bars
            continue

        # ── Normal mode rendering ────────────────────────────────────────

        # Watchdog: Auto-blank if no UDP commands for 30 seconds (Q-SYS plugin disabled)
        time_since_last_command = now - udp.get_last_command_time()
        if time_since_last_command > WATCHDOG_TIMEOUT and not watchdog_blanked and not ip_splash_active:
            logger.warning(f"[WATCHDOG] No UDP commands for {WATCHDOG_TIMEOUT}s - blanking display")
            if canvas and matrix:
                canvas.Clear()
                canvas = matrix.SwapOnVSync(canvas)
            elif matrix:
                matrix.Clear()
            watchdog_blanked = True
        elif time_since_last_command <= WATCHDOG_TIMEOUT and watchdog_blanked:
            # Commands resumed - restore display
            logger.info("[WATCHDOG] Commands resumed - restoring display")
            watchdog_blanked = False
            sm.mark_all_dirty()

        # Skip rendering if display is disabled or watchdog blanked
        if not display_enabled or watchdog_blanked:
            time.sleep(EFFECT_INTERVAL)
            continue

        # Dismiss IP splash on first received command.
        # Do NOT call sm.clear_all() here — the command has already written its
        # content into the target segment inside dispatch().  Wiping here would
        # race and blank the display until the next command arrives.
        if ip_splash_active and udp.has_received_command():
            ip_splash_active = False
            
            # Restore saved rotation to global config
            from udp_handler import set_rotation
            set_rotation(saved_rotation)
            logger.info(f"[SPLASH] Restored rotation to {saved_rotation}°")
            
            if renderer:
                renderer.set_rotation_override(None)  # Clear renderer override
                # Re-apply layout for the actual rotation (might have changed from 0° to 90°/270°)
                udp._apply_layout(udp._current_layout if hasattr(udp, '_current_layout') else 1)
            logger.info("[SPLASH] First command received — IP splash dismissed, rotation restored")

        # Update effects and render at fixed interval
        if now - last_effect >= EFFECT_INTERVAL:
            sm.update_effects()
            last_effect = now

            if renderer:
                try:
                    renderer.render_all()
                except Exception as exc:
                    logger.error(f"[RENDER] Exception: {exc}")
        
        # Sleep to yield CPU and allow matrix library clean refresh cycles
        # Aligned with EFFECT_INTERVAL for consistent timing
        time.sleep(EFFECT_INTERVAL)


if __name__ == "__main__":
    main()
