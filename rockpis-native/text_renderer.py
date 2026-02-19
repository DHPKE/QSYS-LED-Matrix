"""
Text renderer for native HUB75 driver
Renders text using Pillow (PIL) and writes to driver's back buffer
"""

import logging
from PIL import Image, ImageDraw, ImageFont

logger = logging.getLogger(__name__)

# Default font path (Debian/Ubuntu)
FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"


class TextRenderer:
    """
    Renders text segments to the HUB75 driver's back buffer
    """
    
    def __init__(self, driver, segment_manager):
        self.driver = driver
        self.sm = segment_manager
        
        # Font cache (size -> font object)
        self.font_cache = {}
        
        logger.info(f"TextRenderer initialized for {driver.width}×{driver.height} display")
    
    def get_font(self, size: int):
        """Get cached font or load new one"""
        if size not in self.font_cache:
            try:
                self.font_cache[size] = ImageFont.truetype(FONT_PATH, size)
            except Exception as e:
                logger.warning(f"Failed to load font size {size}: {e}, using default")
                self.font_cache[size] = ImageFont.load_default()
        return self.font_cache[size]
    
    def render(self):
        """
        Render all active segments to the driver's back buffer
        """
        # Clear back buffer
        self.driver.clear(0, 0, 0)
        
        # Get active segments
        active_segments = self.sm.get_active_segments()
        
        if not active_segments:
            return
        
        # Render each segment
        for seg_id, seg_data in active_segments.items():
            self.render_segment(seg_data)
    
    def render_segment(self, seg_data: dict):
        """
        Render a single segment
        Args:
            seg_data: Segment data with keys: text, x, y, w, h, color, bgcolor, etc.
        """
        text = seg_data.get('text', '')
        if not text:
            return
        
        # Get segment position and size
        x = seg_data.get('x', 0)
        y = seg_data.get('y', 0)
        w = seg_data.get('w', self.driver.width)
        h = seg_data.get('h', self.driver.height)
        
        # Get color (default white)
        color = seg_data.get('color', (255, 255, 255))
        if isinstance(color, str):
            color = self.parse_color(color)
        
        # Get background color (default black)
        bgcolor = seg_data.get('bgcolor', (0, 0, 0))
        if isinstance(bgcolor, str):
            bgcolor = self.parse_color(bgcolor)
        
        # Auto-size font to fit segment height (use 80% of height)
        font_size = max(6, int(h * 0.8))
        font = self.get_font(font_size)
        
        # Create PIL image for this segment
        img = Image.new('RGB', (w, h), bgcolor)
        draw = ImageDraw.Draw(img)
        
        # Draw text with alignment
        try:
            # Get text bounding box
            bbox = draw.textbbox((0, 0), text, font=font)
            text_width = bbox[2] - bbox[0]
            text_height = bbox[3] - bbox[1]
            
            # Calculate position based on alignment
            align = seg_data.get('align', 'C')
            if align == 'L' or align == 'left':
                text_x = 2
            elif align == 'R' or align == 'right':
                text_x = w - text_width - 2
            else:  # Center
                text_x = (w - text_width) // 2
            
            text_y = (h - text_height) // 2
            
            draw.text((text_x, text_y), text, font=font, fill=color)
        except Exception as e:
            logger.warning(f"Failed to draw text '{text}': {e}")
            return
        
        # Copy to driver's back buffer
        for py in range(h):
            for px in range(w):
                if x + px < self.driver.width and y + py < self.driver.height:
                    r, g, b = img.getpixel((px, py))
                    self.driver.set_pixel(x + px, y + py, r, g, b)
    
    def parse_color(self, color_str: str) -> tuple:
        """Parse color string to RGB tuple"""
        # Remove # if present
        color_str = color_str.lstrip('#')
        
        # Handle hex color
        if len(color_str) == 6:
            return tuple(int(color_str[i:i+2], 16) for i in (0, 2, 4))
        
        # Handle named colors (basic set)
        colors = {
            'red': (255, 0, 0),
            'green': (0, 255, 0),
            'blue': (0, 0, 255),
            'yellow': (255, 255, 0),
            'cyan': (0, 255, 255),
            'magenta': (255, 0, 255),
            'white': (255, 255, 255),
            'black': (0, 0, 0),
        }
        return colors.get(color_str.lower(), (255, 255, 255))
