"""
udp_handler.py — Port of udp_handler.h

Listens on UDP_PORT for JSON commands from Q-SYS (or any UDP sender).
Runs in its own daemon thread so it never blocks the render loop.

Supported JSON commands (identical protocol to the ESP32 firmware):
  {"cmd":"text",       "seg":0, "text":"Hello", "color":"FFFFFF",
   "bgcolor":"000000", "font":"arial", "size":"auto",
   "align":"C", "effect":"none", "intensity":255}
  {"cmd":"layout",     "preset":1}          # 1-7, see config.LAYOUT_PRESETS
  {"cmd":"clear",      "seg":0}
  {"cmd":"clear_all"}
  {"cmd":"brightness", "value":200}
  {"cmd":"config",     "seg":0, "x":0, "y":0, "w":64, "h":32}
"""

import socket
import json
import threading
import logging
import os

from config import UDP_PORT, UDP_BIND_ADDR, MAX_TEXT_LENGTH, LAYOUT_PRESETS, LAYOUT_PRESETS_PORTRAIT, MAX_SEGMENTS, CONFIG_FILE, ORIENTATION, GROUP_ID
from segment_manager import SegmentManager

logger = logging.getLogger(__name__)

# Brightness is 0-255 in the protocol (same as ESP32 firmware).
# We convert to 0-100 for the rpi-rgb-led-matrix library when applying it.
_brightness = 128  # default
_brightness_lock = threading.Lock()

# Orientation is "landscape" or "portrait"
_orientation = ORIENTATION
_orientation_lock = threading.Lock()

# Group ID (0 = no grouping, 1-8 = assigned group)
_group_id = GROUP_ID
_group_id_lock = threading.Lock()


def get_brightness() -> int:
    with _brightness_lock:
        return _brightness


def _set_brightness(value: int):
    global _brightness
    with _brightness_lock:
        _brightness = max(0, min(255, int(value)))


def get_orientation() -> str:
    """Get current orientation: 'landscape' or 'portrait'"""
    with _orientation_lock:
        return _orientation


def get_canvas_dimensions() -> tuple[int, int]:
    """Get current canvas width and height based on orientation.
    Returns (width, height) for the virtual canvas.
    """
    from config import MATRIX_WIDTH, MATRIX_HEIGHT
    orientation = get_orientation()
    if orientation == "portrait":
        # Portrait: 32 wide × 64 tall
        return (MATRIX_HEIGHT, MATRIX_WIDTH)
    else:
        # Landscape: 64 wide × 32 tall
        return (MATRIX_WIDTH, MATRIX_HEIGHT)


def set_orientation(value: str):
    """Set orientation and persist to config file"""
    global _orientation
    value = value.lower()
    if value not in ("landscape", "portrait"):
        logger.warning(f"[ORIENTATION] Invalid value: {value}")
        return False
    
    with _orientation_lock:
        _orientation = value
    
    # Persist to config file
    _save_config()
    logger.info(f"[ORIENTATION] Set to: {value}")
    return True


def get_group_id() -> int:
    """Get current group ID (0 = no grouping, 1-8 = assigned group)"""
    with _group_id_lock:
        return _group_id


def set_group_id(value: int):
    """Set group ID and persist to config file"""
    global _group_id
    if not (0 <= value <= 8):
        logger.warning(f"[GROUP] Invalid group ID: {value}")
        return False
    
    with _group_id_lock:
        _group_id = value
    
    # Persist to config file
    _save_config()
    logger.info(f"[GROUP] Set to: {value}")
    return True


def _save_config():
    """Save current orientation and group ID to JSON config file"""
    try:
        os.makedirs(os.path.dirname(CONFIG_FILE), exist_ok=True)
        config_data = {
            "orientation": get_orientation(),
            "group_id": get_group_id()
        }
        with open(CONFIG_FILE, "w") as f:
            json.dump(config_data, f)
        logger.debug(f"[CONFIG] Saved to {CONFIG_FILE}")
    except Exception as e:
        logger.error(f"[CONFIG] Failed to save: {e}")


def _load_config():
    """Load orientation and group ID from JSON config file"""
    global _orientation, _group_id
    try:
        if os.path.exists(CONFIG_FILE):
            with open(CONFIG_FILE, "r") as f:
                config_data = json.load(f)
                orientation = config_data.get("orientation", "landscape")
                group_id = config_data.get("group_id", 0)
                with _orientation_lock:
                    _orientation = orientation
                with _group_id_lock:
                    _group_id = group_id
                logger.info(f"[CONFIG] Loaded orientation: {orientation}, group_id: {group_id}")
    except Exception as e:
        logger.warning(f"[CONFIG] Failed to load: {e}, using defaults")


class UDPHandler:
    """
    Receives JSON UDP packets and updates the SegmentManager.

    Call start() once — it spawns a background daemon thread.
    Call dispatch(raw_json_str) directly (e.g. from the web handler).
    """

    def __init__(self, segment_manager: SegmentManager,
                 brightness_callback=None,
                 orientation_callback=None):
        """
        :param segment_manager: shared SegmentManager instance
        :param brightness_callback: optional callable(int 0-255) invoked when
               brightness changes so the display can be updated immediately.
        :param orientation_callback: optional callable(str) invoked when
               orientation changes ('landscape' or 'portrait').
        """
        self._sm   = segment_manager
        self._sock = None
        self._running = False
        self._thread  = None
        self._brightness_callback = brightness_callback
        self._orientation_callback = orientation_callback
        self._orientation = get_orientation()  # Track current orientation for layout presets
        self._current_layout = 1  # Track current layout preset number (applies to both orientations)
        # Set to True on the first successfully dispatched command so
        # main.py can dismiss the IP splash screen.
        self._first_command_received = False

    # ─── Public API ────────────────────────────────────────────────────────

    def start(self):
        """Bind the UDP socket and start the listener thread."""
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind((UDP_BIND_ADDR, UDP_PORT))
        self._sock.settimeout(1.0)   # unblocks the thread for clean shutdown
        self._running = True
        self._thread  = threading.Thread(target=self._run,
                                         name="udp-listener",
                                         daemon=True)
        self._thread.start()
        logger.info(f"[UDP] Listening on {UDP_BIND_ADDR}:{UDP_PORT}")

    def stop(self):
        self._running = False
        if self._sock:
            self._sock.close()

    def has_received_command(self) -> bool:
        """True once the first command has been dispatched (UDP or web)."""
        return self._first_command_received
    
    def get_current_layout(self) -> int:
        """Return the current layout preset number."""
        return self._current_layout

    def dispatch(self, raw: str):
        """Parse a raw JSON string and apply the command (thread-safe)."""
        try:
            doc = json.loads(raw)
        except json.JSONDecodeError as e:
            logger.error(f"[UDP] JSON parse error: {e}")
            return

        # Flag raised on the very first successfully parsed command —
        # used by main.py to dismiss the IP address splash screen.
        self._first_command_received = True

        # Check group filtering: if the command has a "group" field,
        # only execute if it matches this panel's group_id (or if group_id is 0)
        cmd_group = doc.get("group", 0)  # 0 = broadcast to all
        my_group = get_group_id()
        
        if cmd_group != 0 and my_group != 0 and cmd_group != my_group:
            # Command is for a different group, ignore it
            logger.debug(f"[UDP] Ignoring command for group {cmd_group} (this panel is group {my_group})")
            return

        cmd = doc.get("cmd", "")

        if cmd == "text":
            seg     = int(doc.get("seg", 0))
            text    = str(doc.get("text", ""))[:MAX_TEXT_LENGTH]
            color   = str(doc.get("color",   "FFFFFF"))
            bgcolor = str(doc.get("bgcolor", "000000"))
            align   = str(doc.get("align",   "C"))
            effect  = str(doc.get("effect",  "none"))
            intens  = int(doc.get("intensity", 255))
            self._sm.update_text(seg, text, color=color, bgcolor=bgcolor,
                                 align=align, effect=effect, intensity=intens)

        elif cmd == "layout":
            preset = int(doc.get("preset", 1))
            self._apply_layout(preset)

        elif cmd == "clear":
            seg = int(doc.get("seg", 0))
            self._sm.clear_segment(seg)

        elif cmd == "clear_all":
            self._sm.clear_all()

        elif cmd == "brightness":
            val = doc.get("value", -1)
            if 0 <= int(val) <= 255:
                _set_brightness(int(val))
                if self._brightness_callback:
                    self._brightness_callback(int(val))

        elif cmd == "orientation":
            value = str(doc.get("value", "landscape"))
            if set_orientation(value):
                self._orientation = value  # Update instance variable for layout presets
                if self._orientation_callback:
                    self._orientation_callback(value)

        elif cmd == "group":
            # Set this panel's group ID
            value = int(doc.get("value", 0))
            set_group_id(value)

        elif cmd == "config":
            seg = int(doc.get("seg", 0))
            x   = int(doc.get("x", 0))
            y   = int(doc.get("y", 0))
            w   = int(doc.get("w", 64))
            h   = int(doc.get("h", 32))
            self._sm.configure(seg, x, y, w, h)

        else:
            logger.warning(f"[UDP] Unknown cmd: {cmd}")

    # ─── Layout presets ────────────────────────────────────────────────────

    def _apply_layout(self, preset: int):
        """Apply a numbered layout preset from config.LAYOUT_PRESETS or LAYOUT_PRESETS_PORTRAIT."""
        # Use portrait layouts if in portrait mode
        if self._orientation == "portrait":
            zones = LAYOUT_PRESETS_PORTRAIT.get(preset)
        else:
            zones = LAYOUT_PRESETS.get(preset)
        
        if zones is None:
            logger.warning(f"[UDP] Unknown layout preset {preset}")
            return
        
        # Remember the current layout preset number
        self._current_layout = preset
        
        logger.info(f"[UDP] LAYOUT preset={preset} ({len(zones)} segment(s)) in {self._orientation} mode")
        for i in range(MAX_SEGMENTS):
            if i < len(zones):
                x, y, w, h = zones[i]
                self._sm.configure(i, x, y, w, h)
                self._sm.activate(i, True)
            else:
                self._sm.activate(i, False)

    # ─── Internal ──────────────────────────────────────────────────────────

    def _run(self):
        while self._running:
            try:
                data, addr = self._sock.recvfrom(4096)
                raw = data.decode("utf-8", errors="replace").strip()
                logger.debug(f"[UDP] {addr[0]}:{addr[1]}  {raw[:120]}")
                self.dispatch(raw)
            except socket.timeout:
                continue
            except OSError:
                break   # socket closed by stop()
        logger.info("[UDP] Listener thread exited")
