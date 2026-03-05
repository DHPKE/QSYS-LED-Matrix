"""
text_renderer.py — Port of text_renderer.h

Renders all active segments onto the rpi-rgb-led-matrix canvas.
Uses Pillow (PIL) to composite text onto a per-segment Image, then
copies the pixels into the matrix's FrameCanvas.

Font sizes are auto-fitted to the segment bounding box, mirroring
the ESP32 firmware's `autoSize` behaviour.

(Identical logic to rpi/text_renderer.py — only the config import differs.)
"""

import os
import logging
from typing import Optional

from PIL import Image, ImageDraw, ImageFont

from config import (MATRIX_WIDTH, MATRIX_HEIGHT,
                    FONT_PATH, FONT_PATH_FALLBACK)
from segment_manager import SegmentManager, TextAlign, TextEffect

logger = logging.getLogger(__name__)

# Candidate font sizes tried in descending order (largest first).
_FONT_SIZES = [24, 20, 18, 16, 14, 12, 10, 9, 8, 6]

# Cache loaded fonts keyed by (path, size) to avoid repeated disk I/O.
_font_cache: dict[tuple, ImageFont.FreeTypeFont] = {}


def _load_font(size: int) -> ImageFont.ImageFont:
    for path in (FONT_PATH, FONT_PATH_FALLBACK):
        key = (path, size)
        if key in _font_cache:
            return _font_cache[key]
        if os.path.exists(path):
            try:
                font = ImageFont.truetype(path, size)
                _font_cache[key] = font
                return font
            except Exception:
                pass
    # Last resort: PIL built-in bitmap font (no size argument)
    return ImageFont.load_default()


def _hex_to_rgb(hex_str: str) -> tuple[int, int, int]:
    """Convert '#RRGGBB' or 'RRGGBB' to (R, G, B)."""
    h = hex_str.lstrip("#")
    if len(h) != 6:
        return (255, 255, 255)
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))


def _fit_text(text: str, max_w: int, max_h: int) -> tuple[ImageFont.ImageFont, int]:
    """Return (font, font_size) for the largest size that fits within max_w × max_h."""
    for size in _FONT_SIZES:
        font = _load_font(size)
        bbox = font.getbbox(text) if hasattr(font, "getbbox") else (0, 0, len(text) * size, size)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        if tw <= max_w and th <= max_h:
            return font, size
    # Fallback: smallest size
    return _load_font(_FONT_SIZES[-1]), _FONT_SIZES[-1]


class TextRenderer:
    """
    Draws all active segments onto a shared RGBMatrix canvas.

    Usage:
        renderer = TextRenderer(matrix, canvas, segment_manager)
        # In render loop:
        renderer.render_all()
        matrix.SwapOnVSync(canvas)
    """

    def __init__(self, matrix, canvas, segment_manager: SegmentManager):
        """
        :param matrix:  rgbmatrix.RGBMatrix instance
        :param canvas:  FrameCanvas returned by matrix.CreateFrameCanvas()
        :param segment_manager: shared SegmentManager
        """
        self._matrix  = matrix
        self._canvas  = canvas
        self._sm      = segment_manager
        # Off-screen PIL image for compositing
        self._image   = Image.new("RGB", (MATRIX_WIDTH, MATRIX_HEIGHT), (0, 0, 0))
        self._draw    = ImageDraw.Draw(self._image)

    def render_all(self):
        """Composite all active segments and push to the matrix canvas."""
        # Clear full framebuffer to black
        self._image.paste((0, 0, 0), [0, 0, MATRIX_WIDTH, MATRIX_HEIGHT])

        with self._sm._lock:
            segments = list(self._sm._segments)

        for seg in segments:
            if not seg.is_active:
                continue
            if seg.width <= 0 or seg.height <= 0:
                continue
            self._render_segment(seg)
            seg.is_dirty = False

        # Push the PIL image to the matrix canvas
        self._canvas.SetImage(self._image)
        self._canvas = self._matrix.SwapOnVSync(self._canvas)

    # ─── Private ───────────────────────────────────────────────────────────

    def _render_segment(self, seg):
        fg = _hex_to_rgb(seg.color)
        bg = _hex_to_rgb(seg.bgcolor)

        # Fill background
        self._draw.rectangle(
            [seg.x, seg.y, seg.x + seg.width - 1, seg.y + seg.height - 1],
            fill=bg
        )

        text = seg.text
        if not text:
            return

        # Handle blink effect
        if seg.effect == TextEffect.BLINK and not seg.blink_state:
            return  # invisible half of blink cycle

        # Auto-fit font
        avail_w = max(1, seg.width  - 4)
        avail_h = max(1, seg.height - 2)
        font, font_size = _fit_text(text, avail_w, avail_h)

        # Measure text
        try:
            bbox = font.getbbox(text)
            tw = bbox[2] - bbox[0]
            th = bbox[3] - bbox[1]
            descent = bbox[1]   # may be negative
        except AttributeError:
            tw = len(text) * font_size
            th = font_size
            descent = 0

        # Handle scroll effect
        draw_x = seg.x
        if seg.effect == TextEffect.SCROLL:
            # Scroll text from right to left, wrapping
            total_scroll = tw + seg.width
            offset = seg.scroll_offset % total_scroll
            draw_x = seg.x + seg.width - offset
        else:
            # Static alignment
            if seg.align == TextAlign.LEFT:
                draw_x = seg.x + 2
            elif seg.align == TextAlign.RIGHT:
                draw_x = seg.x + seg.width - tw - 2
            else:  # CENTER
                draw_x = seg.x + (seg.width - tw) // 2

        draw_y = seg.y + (seg.height - th) // 2 - descent

        # For scroll we need a clip region
        if seg.effect == TextEffect.SCROLL:
            # Render into a temporary image and paste with clipping
            tmp = Image.new("RGB", (MATRIX_WIDTH + tw, seg.height), bg)
            tmp_draw = ImageDraw.Draw(tmp)
            tmp_draw.text((draw_x - seg.x, (seg.height - th) // 2 - descent),
                          text, font=font, fill=fg)
            # Crop to segment width
            region = tmp.crop((0, 0, seg.width, seg.height))
            self._image.paste(region, (seg.x, seg.y))
        else:
            self._draw.text((draw_x, draw_y), text, font=font, fill=fg)

    @property
    def canvas(self):
        return self._canvas
