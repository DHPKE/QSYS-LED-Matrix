"""
curtain_manager.py — Curtain Mode Manager (v7.0+)

Manages curtain mode: 2-pixel frame around the entire display.
Curtains are group-based (1-8) and can be toggled on/off via boolean input.

Curtain layout:
  - 2-pixel frame rendered on all four edges (top, right, bottom, left)
  - Inner display area: (2,2) to (61,29) on 64x32 panel
  - Frame is rendered on top of all segments (highest z-index)

Curtain frame is always rendered on top of segments (highest z-index).
"""

import logging
import json
import os
import threading
from typing import Dict, Tuple
from PIL import Image, ImageDraw

from config import CURTAIN_WIDTH, CURTAIN_DEFAULT_COLOR, MATRIX_WIDTH, MATRIX_HEIGHT, CONFIG_FILE

logger = logging.getLogger(__name__)


class CurtainManager:
    """Manages curtain mode state and rendering."""
    
    def __init__(self):
        self._lock = threading.Lock()
        
        # Curtain state per group: {group_id: {"enabled": bool, "visible": bool, "color": (R,G,B)}}
        # "enabled" = configuration enabled for this group
        # "visible" = current visibility state (toggled by boolean trigger)
        self._curtains: Dict[int, Dict] = {}
        
        # Load persisted state
        self._load_state()
    
    def configure(self, group: int, enabled: bool, color_hex: str) -> None:
        """
        Configure curtain for a group.
        
        Args:
            group: Group ID (1-8)
            enabled: Whether curtain is enabled for this group
            color_hex: Color in hex format (FFFFFF)
        """
        if not 1 <= group <= 8:
            logger.warning(f"[Curtain] Invalid group {group}, must be 1-8")
            return
        
        # Parse hex color
        try:
            color_hex = color_hex.replace('#', '')
            r = int(color_hex[0:2], 16)
            g = int(color_hex[2:4], 16)
            b = int(color_hex[4:6], 16)
            color = (r, g, b)
        except:
            logger.warning(f"[Curtain] Invalid color {color_hex}, using default")
            color = CURTAIN_DEFAULT_COLOR
        
        with self._lock:
            if group not in self._curtains:
                self._curtains[group] = {
                    "enabled": enabled,
                    "visible": False,  # Start invisible
                    "color": color
                }
            else:
                self._curtains[group]["enabled"] = enabled
                self._curtains[group]["color"] = color
        
        logger.info(f"[Curtain] Group {group} configured: enabled={enabled}, color={color}")
        self._save_state()
    
    def set_visibility(self, group: int, visible: bool) -> None:
        """
        Toggle curtain visibility for a group (boolean trigger).
        Only sets visibility if curtain is enabled for this group.
        
        Args:
            group: Group ID (1-8)
            visible: Whether curtain should be visible
        """
        if not 1 <= group <= 8:
            logger.warning(f"[Curtain] Invalid group {group}, must be 1-8")
            return
        
        with self._lock:
            if group not in self._curtains:
                # Don't auto-create if not configured - curtain must be explicitly enabled first
                logger.info(f"[Curtain] Group {group} not configured, ignoring visibility request")
                return
            
            # Only set visibility if enabled
            if not self._curtains[group].get("enabled", False):
                logger.info(f"[Curtain] Group {group} not enabled, ignoring visibility request")
                return
            
            self._curtains[group]["visible"] = visible
        
        logger.info(f"[Curtain] Group {group} visibility: {visible}")
        self._save_state()
    
    def should_render(self, group: int) -> bool:
        """Check if curtain should be rendered for a group."""
        with self._lock:
            if group not in self._curtains:
                return False
            return self._curtains[group].get("enabled", False) and self._curtains[group].get("visible", False)
    
    def get_color(self, group: int) -> Tuple[int, int, int]:
        """Get curtain color for a group."""
        with self._lock:
            if group in self._curtains:
                return self._curtains[group].get("color", CURTAIN_DEFAULT_COLOR)
            return CURTAIN_DEFAULT_COLOR
    
    def render(self, image: Image.Image, group: int, rotation: int = 0) -> None:
        """
        Render 2-pixel frame around the entire display if enabled for this group.
        Frame is always rendered on all four edges of the canvas,
        regardless of rotation.
        
        Args:
            image: PIL Image to render on
            group: Current group ID (0 = no grouping, 1-8 = assigned group)
            rotation: Current rotation in degrees (0, 90, 180, 270)
        """
        if group == 0 or not self.should_render(group):
            return
        
        color = self.get_color(group)
        draw = ImageDraw.Draw(image)
        
        # Get actual canvas dimensions
        img_width, img_height = image.size
        
        # Frame border width (2 pixels)
        border = 2
        
        # Draw 2-pixel frame around entire display
        # Top edge (2 pixels high)
        draw.rectangle(
            [(0, 0), (img_width - 1, border - 1)],
            fill=color
        )
        
        # Bottom edge (2 pixels high)
        draw.rectangle(
            [(0, img_height - border), (img_width - 1, img_height - 1)],
            fill=color
        )
        
        # Left edge (2 pixels wide, full height)
        draw.rectangle(
            [(0, 0), (border - 1, img_height - 1)],
            fill=color
        )
        
        # Right edge (2 pixels wide, full height)
        draw.rectangle(
            [(img_width - border, 0), (img_width - 1, img_height - 1)],
            fill=color
        )
        
        logger.debug(f"[Curtain] Rendered 2px frame for group {group} at {rotation}° (canvas {img_width}×{img_height}) with color {color}")
    
    def _load_state(self) -> None:
        """Load curtain state from persistent storage."""
        try:
            if os.path.exists(CONFIG_FILE):
                with open(CONFIG_FILE, 'r') as f:
                    data = json.load(f)
                    if "curtains" in data:
                        with self._lock:
                            self._curtains = data["curtains"]
                        logger.info(f"[Curtain] Loaded state: {len(self._curtains)} groups configured")
        except Exception as e:
            logger.warning(f"[Curtain] Failed to load state: {e}")
    
    def _save_state(self) -> None:
        """Save curtain state to persistent storage."""
        try:
            # Load existing config
            data = {}
            if os.path.exists(CONFIG_FILE):
                with open(CONFIG_FILE, 'r') as f:
                    data = json.load(f)
            
            # Update curtain state
            with self._lock:
                data["curtains"] = self._curtains
            
            # Save back
            with open(CONFIG_FILE, 'w') as f:
                json.dump(data, f, indent=2)
            
            logger.debug("[Curtain] State saved")
        except Exception as e:
            logger.error(f"[Curtain] Failed to save state: {e}")
    
    def get_state(self) -> Dict:
        """Get current curtain state for all groups."""
        with self._lock:
            return self._curtains.copy()
