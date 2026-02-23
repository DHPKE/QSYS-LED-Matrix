"""
text_renderer.py — Port of text_renderer.h

Renders all active segments onto the rpi-rgb-led-matrix canvas.
Uses Pillow (PIL) to composite text onto a per-segment Image, then
copies the pixels into the matrix's FrameCanvas.

Font sizes are auto-fitted to the segment bounding box, mirroring
the ESP32 firmware's `autoSize` behaviour.

Supports landscape and portrait orientation modes.
"""

import os
import logging
from typing import Optional
import numpy as np

from PIL import Image, ImageDraw, ImageFont

from config import (MATRIX_WIDTH, MATRIX_HEIGHT,
                    FONT_PATH, FONT_PATH_FALLBACK,
                    GROUP_COLORS, GROUP_INDICATOR_SIZE)
from segment_manager import SegmentManager, TextAlign, TextEffect
import udp_handler

logger = logging.getLogger(__name__)

# Expanded font size range for maximum granularity (tries every size from 32 down to 6)
_FONT_SIZES = [32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 13, 12, 11, 10, 9, 8, 7, 6]

# Cache loaded fonts keyed by (path, size) to avoid repeated disk I/O.
_font_cache: dict[tuple, ImageFont.FreeTypeFont] = {}

# Cache for hex to RGB conversions
_color_cache: dict[str, tuple[int, int, int]] = {}

# Cache for text measurements: (text, size) -> (width, height)
_text_measurement_cache: dict[tuple, tuple[int, int]] = {}


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
    """Convert '#RRGGBB' or 'RRGGBB' to (R, G, B). Cached for performance."""
    if hex_str in _color_cache:
        return _color_cache[hex_str]
    
    h = hex_str.lstrip("#")
    if len(h) != 6:
        rgb = (255, 255, 255)
    else:
        rgb = (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))
    
    _color_cache[hex_str] = rgb
    return rgb


def _fit_text(text: str, max_w: int, max_h: int, debug_seg_id: int = -1) -> tuple[ImageFont.ImageFont, int]:
    """Return (font, font_size) for the largest size that fits within max_w × max_h.
    Uses cached measurements for performance."""
    
    # Try to use cached measurement first
    for size in _FONT_SIZES:
        cache_key = (text, size)
        
        if cache_key in _text_measurement_cache:
            tw, th = _text_measurement_cache[cache_key]
        else:
            # Measure and cache
            font = _load_font(size)
            bbox = font.getbbox(text) if hasattr(font, "getbbox") else (0, 0, len(text) * size, size)
            tw = bbox[2] - bbox[0]
            th = bbox[3] - bbox[1]
            _text_measurement_cache[cache_key] = (tw, th)
        
        # Return first (largest) size that fits
        if tw <= max_w and th <= max_h:
            if debug_seg_id >= 0:
                logger.debug(f"[FONT] Seg {debug_seg_id}: '{text[:20]}' → size {size} ({tw}×{th} in {max_w}×{max_h})")
            return _load_font(size), size
    
    # Fallback: smallest size
    if debug_seg_id >= 0:
        logger.debug(f"[FONT] Seg {debug_seg_id}: '{text[:20]}' → min size {_FONT_SIZES[-1]} (text too long)")
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
        # Off-screen PIL image for compositing - size depends on orientation
        # Always matches the physical matrix dimensions (64×32)
        self._image   = Image.new("RGB", (MATRIX_WIDTH, MATRIX_HEIGHT), (0, 0, 0))
        self._draw    = ImageDraw.Draw(self._image)
        self._render_count = 0
        self._current_orientation = "landscape"

    def render_all(self):
        """Composite all active segments and push to the matrix canvas."""
        
        # Fast dirty check without lock for initial test
        needs_render = any(seg.is_dirty for seg in self._sm._segments)
        
        # Skip rendering entirely if nothing changed
        if not needs_render:
            return
        
        # Get current orientation and check if we need to recreate the image
        orientation = udp_handler.get_orientation()
        
        # Determine virtual canvas dimensions based on orientation
        if orientation == "portrait":
            # Portrait mode: 32 wide × 64 tall virtual space
            canvas_width = MATRIX_HEIGHT  # 32
            canvas_height = MATRIX_WIDTH  # 64
        else:
            # Landscape mode: 64 wide × 32 tall
            canvas_width = MATRIX_WIDTH
            canvas_height = MATRIX_HEIGHT
        
        # Recreate image if orientation changed
        if orientation != self._current_orientation:
            self._image = Image.new("RGB", (canvas_width, canvas_height), (0, 0, 0))
            self._draw = ImageDraw.Draw(self._image)
            self._current_orientation = orientation
            logger.info(f"[RENDER] Canvas resized to {canvas_width}×{canvas_height} for {orientation} mode")
            # Log active segment dimensions for debugging
            with self._sm._lock:
                for seg in self._sm._segments:
                    if seg.is_active:
                        logger.debug(f"[RENDER]   Segment {seg.id}: ({seg.x},{seg.y}) {seg.width}×{seg.height}")
        
        # Clear full framebuffer to black (fast operation)
        self._image.paste(0, [0, 0, canvas_width, canvas_height])

        rendered_count = 0
        # Render ALL active segments when anything is dirty
        with self._sm._lock:
            for seg in self._sm._segments:
                if not seg.is_active:
                    seg.is_dirty = False
                    continue
                if seg.width <= 0 or seg.height <= 0:
                    seg.is_dirty = False
                    continue
                self._render_segment(seg)
                seg.is_dirty = False
                rendered_count += 1
        
        # Reduce logging frequency to every 500 renders (minimize overhead)
        self._render_count += 1
        if self._render_count % 500 == 0:
            logger.debug(f"[RENDER] Rendered {rendered_count} segments (count: {self._render_count})")

        # Render group indicator (always visible on top of everything)
        self._render_group_indicator(canvas_width, canvas_height)

        # Apply rotation if in portrait mode to convert virtual 32×64 to physical 64×32
        if orientation == "portrait":
            # Rotate 270 degrees clockwise (= -90 degrees) to convert 32×64 to 64×32
            # This maps virtual (0,0) at top-left to physical top-left corner
            rotated_img = self._image.rotate(-90, expand=True)
            self._canvas.SetImage(rotated_img)
        else:
            # Landscape mode - use image as-is
            self._canvas.SetImage(self._image)
        
        # Swap canvas
        self._canvas = self._matrix.SwapOnVSync(self._canvas)

    # ─── Private ───────────────────────────────────────────────────────────

    def _render_group_indicator(self, canvas_width: int, canvas_height: int):
        """Draw a small colored square in the bottom-left corner to indicate the group."""
        group_id = udp_handler.get_group_id()
        
        # Skip rendering if no group assigned (0)
        if group_id == 0:
            return
        
        # Get color for this group
        color = GROUP_COLORS.get(group_id, (0, 0, 0))
        
        # Skip if color is black (invisible)
        if color == (0, 0, 0):
            return
        
        # Position: bottom-left corner (4×4 pixel square by default)
        indicator_size = GROUP_INDICATOR_SIZE
        x1 = 0
        y1 = canvas_height - indicator_size
        x2 = indicator_size - 1
        y2 = canvas_height - 1
        
        # Draw the colored square
        self._draw.rectangle([x1, y1, x2, y2], fill=color)

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
            return

        # Auto-fit font with 1px margin on all borders
        avail_w = max(1, seg.width  - 2)  # 1px margin left and right
        avail_h = max(1, seg.height - 2)  # 1px margin top and bottom
        font, font_size = _fit_text(text, avail_w, avail_h, debug_seg_id=seg.id)

        # Measure text
        try:
            bbox = font.getbbox(text)
            tw = bbox[2] - bbox[0]
            th = bbox[3] - bbox[1]
            descent = bbox[1]
        except AttributeError:
            tw = len(text) * font_size
            th = font_size
            descent = 0

        # Handle scroll effect
        draw_x = seg.x
        if seg.effect == TextEffect.SCROLL:
            total_scroll = tw + seg.width
            offset = seg.scroll_offset % total_scroll
            draw_x = seg.x + seg.width - offset
        else:
            # Static alignment - minimal margins for maximum font size
            if seg.align == TextAlign.LEFT:
                draw_x = seg.x + 1
            elif seg.align == TextAlign.RIGHT:
                draw_x = seg.x + seg.width - tw - 1
            else:  # CENTER
                draw_x = seg.x + (seg.width - tw) // 2

        draw_y = seg.y + (seg.height - th) // 2 - descent

        # SHARP NO-ALIAS RENDERING: Binary threshold for crisp edges
        # Render on grayscale then convert to pure black/white
        if seg.effect == TextEffect.SCROLL:
            # For scrolling text, render on larger canvas
            tmp_gray = Image.new("L", (MATRIX_WIDTH + tw, seg.height), 0)
            tmp_draw = ImageDraw.Draw(tmp_gray)
            tmp_draw.text((draw_x - seg.x, (seg.height - th) // 2 - descent),
                          text, font=font, fill=255)
            
            # Binary threshold: > 128 becomes white (pixels > 50% gray)
            tmp_binary = tmp_gray.point(lambda p: 255 if p > 128 else 0, mode='L')
            
            # Crop to segment width
            tmp_crop = tmp_binary.crop((0, 0, seg.width, seg.height))
            
            # Convert to RGB with pure colors (no anti-aliasing)
            mask_array = np.array(tmp_crop, dtype=np.uint8)
            mask_bool = mask_array == 255
            
            region_array = np.full((seg.height, seg.width, 3), bg, dtype=np.uint8)
            region_array[mask_bool] = fg
            
            region = Image.fromarray(region_array, 'RGB')
            self._image.paste(region, (seg.x, seg.y))
        else:
            # For static text
            tmp_gray = Image.new("L", (seg.width, seg.height), 0)
            tmp_draw = ImageDraw.Draw(tmp_gray)
            tmp_draw.text((draw_x - seg.x, draw_y - seg.y), text, font=font, fill=255)
            
            # Binary threshold: > 128 becomes white (pixels > 50% gray)
            tmp_binary = tmp_gray.point(lambda p: 255 if p > 128 else 0, mode='L')
            
            # Convert to RGB with pure colors (no anti-aliasing)
            mask_array = np.array(tmp_binary, dtype=np.uint8)
            mask_bool = mask_array == 255
            
            segment_array = np.full((seg.height, seg.width, 3), bg, dtype=np.uint8)
            segment_array[mask_bool] = fg
            
            segment_img = Image.fromarray(segment_array, 'RGB')
            self._image.paste(segment_img, (seg.x, seg.y))

    @property
    def canvas(self):
        return self._canvas
