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
        # Cache group indicator color to prevent flashing
        self._cached_group_id = udp_handler.get_group_id()
        self._cached_group_color = GROUP_COLORS.get(self._cached_group_id, (0, 0, 0))

    def render_all(self):
        """Composite all active segments and push to the matrix canvas.
        
        Uses snapshot-based rendering to eliminate race conditions:
        1. Take atomic snapshot of segment data (with lock)
        2. Render from snapshot (without lock)
        3. Clear dirty flags (with lock)
        """
        
        # Step 1: Take atomic snapshot (minimal lock time)
        segment_snapshots, any_dirty = self._sm.get_render_snapshot()
        
        # Skip rendering entirely if nothing changed
        if not any_dirty:
            return
        
        # Get current orientation and canvas dimensions
        orientation = udp_handler.get_orientation()
        
        if orientation == "portrait":
            canvas_width = MATRIX_HEIGHT  # 32
            canvas_height = MATRIX_WIDTH  # 64
        else:
            canvas_width = MATRIX_WIDTH   # 64
            canvas_height = MATRIX_HEIGHT # 32
        
        # Recreate image if orientation changed
        if orientation != self._current_orientation:
            self._image = Image.new("RGB", (canvas_width, canvas_height), (0, 0, 0))
            self._draw = ImageDraw.Draw(self._image)
            self._current_orientation = orientation
            logger.info(f"[RENDER] Canvas resized to {canvas_width}×{canvas_height} for {orientation} mode")
        
        # Step 2: Clear canvas to black
        self._image.paste(0, [0, 0, canvas_width, canvas_height])
        
        # Step 3: Render all segments from snapshots (NO LOCK - fully thread-safe)
        rendered_count = 0
        for snap in segment_snapshots:
            if not snap['is_active']:
                continue
            if snap['width'] <= 0 or snap['height'] <= 0:
                continue
            # Render this segment from snapshot data
            self._render_segment_from_snapshot(snap)
            rendered_count += 1
        
        # Step 4: Render group indicator on top
        self._render_group_indicator(canvas_width, canvas_height)
        
        # Step 5: Apply rotation if needed and push to matrix
        if orientation == "portrait":
            rotated_img = self._image.rotate(-90, expand=True)
            self._canvas.SetImage(rotated_img)
        else:
            self._canvas.SetImage(self._image)
        
        # Step 6: Swap canvas (vsync)
        self._canvas = self._matrix.SwapOnVSync(self._canvas)
        
        # Step 7: Clear dirty flags after successful render
        self._sm.clear_dirty_flags()
        
        # Logging (throttled)
        self._render_count += 1
        if self._render_count % 500 == 0:
            logger.debug(f"[RENDER] Rendered {rendered_count} segments (count: {self._render_count})")

    # ─── Private ───────────────────────────────────────────────────────────

    def _render_group_indicator(self, canvas_width: int, canvas_height: int):
        """Draw a small colored square in the bottom-left corner to indicate the group."""
        group_id = udp_handler.get_group_id()
        
        # Update cache if group changed
        if group_id != self._cached_group_id:
            self._cached_group_id = group_id
            self._cached_group_color = GROUP_COLORS.get(group_id, (0, 0, 0))
        
        # Skip rendering if no group assigned (0)
        if self._cached_group_id == 0:
            return
        
        # Skip if color is black (invisible)
        if self._cached_group_color == (0, 0, 0):
            return
        
        # Position: bottom-left corner (2×2 pixel square by default)
        indicator_size = GROUP_INDICATOR_SIZE
        x1 = 0
        y1 = canvas_height - indicator_size
        x2 = indicator_size - 1
        y2 = canvas_height - 1
        
        # Draw the colored square using cached color
        self._draw.rectangle([x1, y1, x2, y2], fill=self._cached_group_color)

    def _render_segment_from_snapshot(self, snap):
        """Render a segment from snapshot data (no Segment object needed)."""
        fg = _hex_to_rgb(snap['color'])
        bg = _hex_to_rgb(snap['bgcolor'])

        # Fill background
        self._draw.rectangle(
            [snap['x'], snap['y'], snap['x'] + snap['width'] - 1, snap['y'] + snap['height'] - 1],
            fill=bg
        )

        text = snap['text']
        if not text:
            return

        # Handle blink effect
        if snap['effect'] == TextEffect.BLINK and not snap['blink_state']:
            return

        # Auto-fit font with 1px margin on all borders
        avail_w = max(1, snap['width']  - 2)  # 1px margin left and right
        avail_h = max(1, snap['height'] - 2)  # 1px margin top and bottom

        font, font_size = _fit_text(text, avail_w, avail_h, snap['id'])
        if not font:
            return

        # Render text to temporary image with anti-aliasing
        tmp = Image.new('RGB', (snap['width'], snap['height']), bg)
        tmp_draw = ImageDraw.Draw(tmp)
        
        # Get text bounding box
        bbox = tmp_draw.textbbox((0, 0), text, font=font)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        
        # Calculate position based on alignment
        if snap['align'] == TextAlign.LEFT:
            tx = 1
        elif snap['align'] == TextAlign.RIGHT:
            tx = snap['width'] - tw - 1
        else:  # CENTER
            tx = (snap['width'] - tw) // 2
        
        ty = (snap['height'] - th) // 2
        
        # Draw text with anti-aliasing
        tmp_draw.text((tx, ty), text, font=font, fill=fg)
        
        # Convert to binary (no anti-aliasing on LED display)
        tmp_gray = tmp.convert('L')
        tmp_binary = tmp_gray.point(lambda p: 255 if p > 128 else 0, mode='L')
        
        # Convert to RGB with pure colors
        mask_array = np.array(tmp_binary, dtype=np.uint8)
        mask_bool = mask_array == 255
        
        segment_array = np.full((snap['height'], snap['width'], 3), bg, dtype=np.uint8)
        segment_array[mask_bool] = fg
        
        segment_img = Image.fromarray(segment_array, 'RGB')
        self._image.paste(segment_img, (snap['x'], snap['y']))

        # Render frame if enabled (draw after text so it appears on top)
        if snap['frame_enabled']:
            frame_color = _hex_to_rgb(snap['frame_color'])
            frame_w = snap['frame_width']
            # Draw frame as rectangles
            for offset in range(frame_w):
                # Top edge
                self._draw.line(
                    [(snap['x'] + offset, snap['y'] + offset),
                     (snap['x'] + snap['width'] - 1 - offset, snap['y'] + offset)],
                    fill=frame_color
                )
                # Bottom edge
                self._draw.line(
                    [(snap['x'] + offset, snap['y'] + snap['height'] - 1 - offset),
                     (snap['x'] + snap['width'] - 1 - offset, snap['y'] + snap['height'] - 1 - offset)],
                    fill=frame_color
                )
                # Left edge
                self._draw.line(
                    [(snap['x'] + offset, snap['y'] + offset),
                     (snap['x'] + offset, snap['y'] + snap['height'] - 1 - offset)],
                    fill=frame_color
                )
                # Right edge
                self._draw.line(
                    [(snap['x'] + snap['width'] - 1 - offset, snap['y'] + offset),
                     (snap['x'] + snap['width'] - 1 - offset, snap['y'] + snap['height'] - 1 - offset)],
                    fill=frame_color
                )

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

        # Render frame if enabled (draw after text so it appears on top)
        if seg.frame_enabled:
            frame_color = _hex_to_rgb(seg.frame_color)
            frame_w = seg.frame_width
            # Draw frame as rectangles
            for offset in range(frame_w):
                # Top edge
                self._draw.line(
                    [(seg.x + offset, seg.y + offset),
                     (seg.x + seg.width - 1 - offset, seg.y + offset)],
                    fill=frame_color
                )
                # Bottom edge
                self._draw.line(
                    [(seg.x + offset, seg.y + seg.height - 1 - offset),
                     (seg.x + seg.width - 1 - offset, seg.y + seg.height - 1 - offset)],
                    fill=frame_color
                )
                # Left edge
                self._draw.line(
                    [(seg.x + offset, seg.y + offset),
                     (seg.x + offset, seg.y + seg.height - 1 - offset)],
                    fill=frame_color
                )
                # Right edge
                self._draw.line(
                    [(seg.x + seg.width - 1 - offset, seg.y + offset),
                     (seg.x + seg.width - 1 - offset, seg.y + seg.height - 1 - offset)],
                    fill=frame_color
                )

    @property
    def canvas(self):
        return self._canvas
