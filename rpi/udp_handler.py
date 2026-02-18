"""
udp_handler.py — Port of udp_handler.h

Listens on UDP_PORT for JSON commands from Q-SYS (or any UDP sender).
Runs in its own daemon thread so it never blocks the render loop.

Supported JSON commands (identical protocol to the ESP32 firmware):
  {"cmd":"text",       "seg":0, "text":"Hello", "color":"FFFFFF",
   "bgcolor":"000000", "font":"arial", "size":"auto",
   "align":"C", "effect":"none", "intensity":255}
  {"cmd":"clear",      "seg":0}
  {"cmd":"clear_all"}
  {"cmd":"brightness", "value":200}
  {"cmd":"config",     "seg":0, "x":0, "y":0, "w":64, "h":32}
"""

import socket
import json
import threading
import logging

from config import UDP_PORT, UDP_BIND_ADDR, MAX_TEXT_LENGTH
from segment_manager import SegmentManager

logger = logging.getLogger(__name__)

# Brightness is 0-255 in the protocol (same as ESP32 firmware).
# We convert to 0-100 for the rpi-rgb-led-matrix library when applying it.
_brightness = 128  # default
_brightness_lock = threading.Lock()


def get_brightness() -> int:
    with _brightness_lock:
        return _brightness


def _set_brightness(value: int):
    global _brightness
    with _brightness_lock:
        _brightness = max(0, min(255, int(value)))


class UDPHandler:
    """
    Receives JSON UDP packets and updates the SegmentManager.

    Call start() once — it spawns a background daemon thread.
    Call dispatch(raw_json_str) directly (e.g. from the web handler).
    """

    def __init__(self, segment_manager: SegmentManager,
                 brightness_callback=None):
        """
        :param segment_manager: shared SegmentManager instance
        :param brightness_callback: optional callable(int 0-255) invoked when
               brightness changes so the display can be updated immediately.
        """
        self._sm   = segment_manager
        self._sock = None
        self._running = False
        self._thread  = None
        self._brightness_callback = brightness_callback

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

    def dispatch(self, raw: str):
        """Parse a raw JSON string and apply the command (thread-safe)."""
        try:
            doc = json.loads(raw)
        except json.JSONDecodeError as e:
            logger.error(f"[UDP] JSON parse error: {e}  raw: {raw[:120]}")
            return

        cmd = doc.get("cmd", "")
        logger.debug(f"[UDP] cmd={cmd}")

        if cmd == "text":
            seg     = int(doc.get("seg", 0))
            text    = str(doc.get("text", ""))[:MAX_TEXT_LENGTH]
            color   = str(doc.get("color",   "FFFFFF"))
            bgcolor = str(doc.get("bgcolor", "000000"))
            align   = str(doc.get("align",   "C"))
            effect  = str(doc.get("effect",  "none"))
            intens  = int(doc.get("intensity", 255))
            font    = str(doc.get("font",    "arial"))
            logger.info(
                f'[UDP] TEXT seg{seg} "{text}" col={color} bg={bgcolor} '
                f'font={font} al={align} fx={effect} i={intens}'
            )
            self._sm.update_text(seg, text, color=color, bgcolor=bgcolor,
                                 align=align, effect=effect, intensity=intens)

        elif cmd == "clear":
            seg = int(doc.get("seg", 0))
            logger.info(f"[UDP] CLEAR seg{seg}")
            self._sm.clear_segment(seg)

        elif cmd == "clear_all":
            logger.info("[UDP] CLEAR ALL")
            self._sm.clear_all()

        elif cmd == "brightness":
            val = doc.get("value", -1)
            if 0 <= int(val) <= 255:
                _set_brightness(int(val))
                logger.info(f"[UDP] BRIGHTNESS {val}")
                if self._brightness_callback:
                    self._brightness_callback(int(val))

        elif cmd == "config":
            seg = int(doc.get("seg", 0))
            x   = int(doc.get("x", 0))
            y   = int(doc.get("y", 0))
            w   = int(doc.get("w", 64))
            h   = int(doc.get("h", 32))
            logger.info(f"[UDP] CONFIG seg{seg} x={x} y={y} w={w} h={h}")
            self._sm.configure(seg, x, y, w, h)

        else:
            logger.warning(f"[UDP] Unknown cmd: {cmd}")

    # ─── Internal ──────────────────────────────────────────────────────────

    def _run(self):
        while self._running:
            try:
                data, addr = self._sock.recvfrom(4096)
                raw = data.decode("utf-8", errors="replace").strip()
                logger.info(f"[UDP] RX {len(raw)} bytes from {addr[0]}: {raw[:120]}")
                self.dispatch(raw)
            except socket.timeout:
                continue
            except OSError:
                break   # socket closed by stop()
        logger.info("[UDP] Listener thread exited")
