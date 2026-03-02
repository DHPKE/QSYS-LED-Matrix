"""
curtain_manager.py — Curtain Mode Manager (v7.0+)

Manages curtain mode: 3-pixel wide vertical bars on left and right edges.
Curtains are group-based (1-8) and can be toggled on/off via boolean input.

Curtain layout:
  - Left bar: pixels 0-2 (3 pixels wide, full height)
  - Right bar: pixels 61-63 (3 pixels wide, full height)
  - Middle area: pixels 3-60 (58 pixels wide) - segments stay as configured

Curtain bars are always rendered on top of segments (highest z-index).
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
        
        Args:
            group: Group ID (1-8)
            visible: Whether curtain should be visible
        """
        if not 1 <= group <= 8:
            logger.warning(f"[Curtain] Invalid group {group}, must be 1-8")
            return
        
        with self._lock:
            if group not in self._curtains:
                # Auto-create with default color if not configured
                self._curtains[group] = {
                    "enabled": True,
                    "visible": visible,
                    "color": CURTAIN_DEFAULT_COLOR
                }
            else:
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
    
    def render(self, image: Image.Image, group: int) -> None:
        """
        Render curtain bars on the given image if enabled for this group.
        
        Args:
            image: PIL Image to render on
            group: Current group ID (0 = no grouping, 1-8 = assigned group)
        """
        if group == 0 or not self.should_render(group):
            return
        
        color = self.get_color(group)
        draw = ImageDraw.Draw(image)
        
        # Left curtain bar (pixels 0-2, full height)
        draw.rectangle(
            [(0, 0), (CURTAIN_WIDTH - 1, MATRIX_HEIGHT - 1)],
            fill=color
        )
        
        # Right curtain bar (pixels 61-63, full height)
        right_x = MATRIX_WIDTH - CURTAIN_WIDTH
        draw.rectangle(
            [(right_x, 0), (MATRIX_WIDTH - 1, MATRIX_HEIGHT - 1)],
            fill=color
        )
        
        logger.debug(f"[Curtain] Rendered curtains for group {group} with color {color}")
    
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
