"""
segment_manager.py — Port of segment_manager.h

Manages up to MAX_SEGMENTS display zones, each with independent text,
colour, alignment, and effect state. Thread-safe via a RLock so the UDP
listener thread and the render thread can both access segments safely.
"""

import logging
import threading
import time
import copy
from enum import Enum
from config import (MAX_SEGMENTS, MAX_TEXT_LENGTH, MATRIX_WIDTH,
                    MATRIX_HEIGHT, DEFAULT_SCROLL_SPEED, DEFAULT_SEGMENTS)

logger = logging.getLogger(__name__)


class TextAlign(Enum):
    LEFT   = "L"
    CENTER = "C"
    RIGHT  = "R"


class TextEffect(Enum):
    NONE    = "none"
    SCROLL  = "scroll"
    BLINK   = "blink"
    FADE    = "fade"
    RAINBOW = "rainbow"


def _parse_align(value: str) -> TextAlign:
    v = (value or "C").upper()
    if v == "L": return TextAlign.LEFT
    if v == "R": return TextAlign.RIGHT
    return TextAlign.CENTER


def _parse_effect(value: str) -> TextEffect:
    v = (value or "none").lower()
    mapping = {e.value: e for e in TextEffect}
    return mapping.get(v, TextEffect.NONE)


class Segment:
    """Single display zone."""
    __slots__ = (
        "id", "x", "y", "width", "height",
        "text", "color", "bgcolor",
        "align", "effect", "effect_speed",
        "scroll_offset", "last_scroll_update",
        "blink_state", "last_blink_update",
        "is_active", "is_dirty",
    )

    def __init__(self, seg_id: int, x: int, y: int, w: int, h: int):
        self.id     = seg_id
        self.x      = x
        self.y      = y
        self.width  = w
        self.height = h
        self.text   = ""
        self.color  = "#FFFFFF"
        self.bgcolor = "#000000"
        self.align  = TextAlign.CENTER
        self.effect = TextEffect.NONE
        self.effect_speed = DEFAULT_SCROLL_SPEED
        self.scroll_offset = 0
        self.last_scroll_update = 0.0
        self.blink_state = True
        self.last_blink_update = 0.0
        self.is_active = False
        self.is_dirty  = False

    def to_dict(self) -> dict:
        return {
            "id":      self.id,
            "x":       self.x,
            "y":       self.y,
            "w":       self.width,
            "h":       self.height,
            "text":    self.text,
            "color":   self.color,
            "bgcolor": self.bgcolor,
            "align":   self.align.value,
            "effect":  self.effect.value,
            "active":  self.is_active,
        }


class SegmentManager:
    """Thread-safe manager for all display segments."""

    def __init__(self):
        self._lock = threading.RLock()
        self._segments: list[Segment] = []
        self._init_default_layout()

    # ─── Initialisation ────────────────────────────────────────────────────

    def _init_default_layout(self):
        self._segments = []
        for d in DEFAULT_SEGMENTS:
            seg = Segment(d["id"], d["x"], d["y"], d["w"], d["h"])
            seg.color    = d["color"]
            seg.bgcolor  = d["bgcolor"]
            seg.align    = _parse_align(d["align"])
            seg.effect   = _parse_effect(d["effect"])
            seg.is_active = d["active"]
            self._segments.append(seg)

    # ─── Read access ───────────────────────────────────────────────────────

    def get_segment(self, seg_id: int) -> Segment | None:
        if 0 <= seg_id < len(self._segments):
            return self._segments[seg_id]
        return None

    def snapshot(self) -> list[dict]:
        """Return a JSON-serialisable copy of all segments (no lock held by caller)."""
        with self._lock:
            return [copy.deepcopy(s.to_dict()) for s in self._segments]

    # ─── Write access (all take the lock) ─────────────────────────────────

    def update_text(self, seg_id: int, text: str, color: str = None,
                    bgcolor: str = None, align: str = None,
                    effect: str = None, intensity: int = 255):
        with self._lock:
            seg = self.get_segment(seg_id)
            if seg is None:
                return
            seg.text = text[:MAX_TEXT_LENGTH]
            if color is not None:
                seg.color = f"#{color.lstrip('#')}"
            if bgcolor is not None:
                seg.bgcolor = f"#{bgcolor.lstrip('#')}"
            if align is not None:
                seg.align = _parse_align(align)
            if effect is not None:
                seg.effect = _parse_effect(effect)
            seg.is_active = True
            seg.is_dirty  = True

    def clear_segment(self, seg_id: int):
        with self._lock:
            seg = self.get_segment(seg_id)
            if seg:
                seg.text = ""
                seg.is_active = False  # Deactivate when clearing
                seg.is_dirty = True

    def clear_all(self):
        with self._lock:
            for seg in self._segments:
                seg.text = ""
                seg.is_dirty = True

    def configure(self, seg_id: int, x: int, y: int, w: int, h: int):
        with self._lock:
            seg = self.get_segment(seg_id)
            if seg:
                seg.x = x
                seg.y = y
                seg.width  = w
                seg.height = h
                seg.is_active = True
                seg.is_dirty  = True
                # Mark ALL segments as dirty to force full redraw
                for s in self._segments:
                    s.is_dirty = True

    def activate(self, seg_id: int, active: bool):
        with self._lock:
            seg = self.get_segment(seg_id)
            if seg:
                seg.is_active = active
                seg.is_dirty  = True
                # Mark ALL segments as dirty to force full redraw when activating/deactivating
                for s in self._segments:
                    s.is_dirty = True

    # ─── Effect tick (call from render loop) ──────────────────────────────

    def update_effects(self):
        now = time.monotonic()
        with self._lock:
            for seg in self._segments:
                if not seg.is_active:
                    continue
                if seg.effect == TextEffect.SCROLL:
                    interval = 1.0 / max(seg.effect_speed, 1)
                    if now - seg.last_scroll_update >= interval:
                        seg.scroll_offset += 1
                        seg.last_scroll_update = now
                        seg.is_dirty = True
                elif seg.effect == TextEffect.BLINK:
                    if now - seg.last_blink_update >= 0.5:
                        seg.blink_state = not seg.blink_state
                        seg.last_blink_update = now
                        seg.is_dirty = True
