/**
 * render-engine.js
 * 
 * Renders text/graphics to a Canvas matching the RPi implementation,
 * then converts to a bitmap that can be sent via C4M SDK.
 * 
 * This mimics the text_renderer.py and segment_manager.py behavior.
 */

const { createCanvas, loadImage, registerFont } = require('canvas');
const fs = require('fs');
const path = require('path');

// Effect constants (matching RPi config)
const EFFECTS = {
  STATIC: 0,
  SCROLL_LEFT: 1,
  SCROLL_RIGHT: 2,
  SCROLL_UP: 3,
  SCROLL_DOWN: 4,
  BLINK: 5,
  // Add more as needed
};

// Alignment constants
const ALIGN = {
  LEFT: 0,
  CENTER: 1,
  RIGHT: 2,
  TOP: 0,
  MIDDLE: 1,
  BOTTOM: 2
};

class RenderEngine {
  constructor(width = 64, height = 32) {
    this.width = width;
    this.height = height;
    this.rotation = 0; // 0, 90, 180, 270
    
    // Segments (matching RPi 4-segment system)
    this.segments = [
      { id: 0, x: 0, y: 0, w: 64, h: 8, enabled: false, text: '', color: '#FF0000', effect: EFFECTS.STATIC, align: ALIGN.LEFT },
      { id: 1, x: 0, y: 8, w: 64, h: 8, enabled: false, text: '', color: '#00FF00', effect: EFFECTS.STATIC, align: ALIGN.LEFT },
      { id: 2, x: 0, y: 16, w: 64, h: 8, enabled: false, text: '', color: '#0000FF', effect: EFFECTS.STATIC, align: ALIGN.LEFT },
      { id: 3, x: 0, y: 24, w: 64, h: 8, enabled: false, text: '', color: '#FFFF00', effect: EFFECTS.STATIC, align: ALIGN.LEFT }
    ];
    
    this.backgroundColor = '#000000';
    this.brightness = 255;
    
    // Font settings (matching RPi defaults)
    this.defaultFont = 'DejaVu Sans';
    this.defaultFontSize = 8;
    
    // Try to register system fonts if available
    this.registerSystemFonts();
    
    // Canvas for rendering
    this.canvas = createCanvas(width, height);
    this.ctx = this.canvas.getContext('2d');
  }

  /**
   * Register system fonts (Linux/macOS)
   */
  registerSystemFonts() {
    const fontPaths = [
      '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
      '/System/Library/Fonts/Supplemental/Arial.ttf',
      '/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf'
    ];
    
    for (const fontPath of fontPaths) {
      if (fs.existsSync(fontPath)) {
        try {
          registerFont(fontPath, { family: 'DejaVu Sans' });
          console.log(`✓ Registered font: ${fontPath}`);
          break;
        } catch (e) {
          // Try next font
        }
      }
    }
  }

  /**
   * Set segment properties (matching RPi UDP protocol)
   */
  setSegment(id, props) {
    if (id < 0 || id >= this.segments.length) return;
    
    const seg = this.segments[id];
    
    if (props.enabled !== undefined) seg.enabled = props.enabled;
    if (props.text !== undefined) seg.text = props.text;
    if (props.color !== undefined) seg.color = this.normalizeColor(props.color);
    if (props.effect !== undefined) seg.effect = props.effect;
    if (props.align !== undefined) seg.align = props.align;
    if (props.x !== undefined) seg.x = props.x;
    if (props.y !== undefined) seg.y = props.y;
    if (props.w !== undefined) seg.w = props.w;
    if (props.h !== undefined) seg.h = props.h;
  }

  /**
   * Set layout preset (matching RPi presets)
   */
  setLayout(preset) {
    // Landscape layouts
    const layouts = {
      0: [ // Full screen
        { x: 0, y: 0, w: 64, h: 32 }
      ],
      1: [ // 4 equal rows
        { x: 0, y: 0, w: 64, h: 8 },
        { x: 0, y: 8, w: 64, h: 8 },
        { x: 0, y: 16, w: 64, h: 8 },
        { x: 0, y: 24, w: 64, h: 8 }
      ],
      2: [ // Top/bottom split
        { x: 0, y: 0, w: 64, h: 16 },
        { x: 0, y: 16, w: 64, h: 16 }
      ],
      3: [ // Left/right split
        { x: 0, y: 0, w: 32, h: 32 },
        { x: 32, y: 0, w: 32, h: 32 }
      ],
      4: [ // Quadrants
        { x: 0, y: 0, w: 32, h: 16 },
        { x: 32, y: 0, w: 32, h: 16 },
        { x: 0, y: 16, w: 32, h: 16 },
        { x: 32, y: 16, w: 32, h: 16 }
      ]
    };
    
    const layout = layouts[preset];
    if (!layout) return;
    
    layout.forEach((dims, i) => {
      if (i < this.segments.length) {
        this.segments[i].x = dims.x;
        this.segments[i].y = dims.y;
        this.segments[i].w = dims.w;
        this.segments[i].h = dims.h;
      }
    });
  }

  /**
   * Normalize color from various formats to hex
   */
  normalizeColor(color) {
    if (typeof color === 'number') {
      // RGB integer (0xRRGGBB)
      return '#' + color.toString(16).padStart(6, '0');
    }
    if (typeof color === 'string') {
      if (color.startsWith('#')) return color;
      if (color.length === 6) return '#' + color;
    }
    return color;
  }

  /**
   * Clear canvas
   */
  clear() {
    this.ctx.fillStyle = this.backgroundColor;
    this.ctx.fillRect(0, 0, this.width, this.height);
  }

  /**
   * Render all enabled segments to canvas
   */
  render() {
    this.clear();
    
    // Render each enabled segment
    for (const seg of this.segments) {
      if (!seg.enabled || !seg.text) continue;
      
      this.renderSegment(seg);
    }
    
    return this.canvas;
  }

  /**
   * Render a single segment
   */
  renderSegment(seg) {
    const { x, y, w, h, text, color, align } = seg;
    
    // Calculate font size to fit height (leaving some padding)
    const fontSize = Math.floor(h * 0.8);
    this.ctx.font = `${fontSize}px "${this.defaultFont}"`;
    this.ctx.fillStyle = color;
    this.ctx.textBaseline = 'middle';
    
    // Calculate text position based on alignment
    let textX = x;
    let textY = y + h / 2;
    
    if (align === ALIGN.LEFT) {
      this.ctx.textAlign = 'left';
      textX = x + 2; // Small padding
    } else if (align === ALIGN.CENTER) {
      this.ctx.textAlign = 'center';
      textX = x + w / 2;
    } else if (align === ALIGN.RIGHT) {
      this.ctx.textAlign = 'right';
      textX = x + w - 2; // Small padding
    }
    
    // Clip to segment bounds
    this.ctx.save();
    this.ctx.beginPath();
    this.ctx.rect(x, y, w, h);
    this.ctx.clip();
    
    // Draw text
    this.ctx.fillText(text, textX, textY);
    
    this.ctx.restore();
  }

  /**
   * Get canvas as buffer (PNG)
   */
  toBuffer() {
    return this.canvas.toBuffer('image/png');
  }

  /**
   * Get canvas as bitmap data (raw RGB)
   */
  toBitmap() {
    const imageData = this.ctx.getImageData(0, 0, this.width, this.height);
    return imageData.data; // Uint8ClampedArray (RGBA)
  }

  /**
   * Save canvas to file (for debugging)
   */
  async saveToFile(filename) {
    const buffer = this.toBuffer();
    await fs.promises.writeFile(filename, buffer);
  }

  /**
   * Set brightness (0-255)
   */
  setBrightness(value) {
    this.brightness = Math.max(0, Math.min(255, value));
  }

  /**
   * Set rotation (0, 90, 180, 270)
   */
  setRotation(degrees) {
    this.rotation = degrees % 360;
    
    // Swap width/height for 90/270 degree rotations
    if (this.rotation === 90 || this.rotation === 270) {
      [this.width, this.height] = [this.height, this.width];
      this.canvas = createCanvas(this.width, this.height);
      this.ctx = this.canvas.getContext('2d');
    }
  }

  /**
   * Get current state (for debugging)
   */
  getState() {
    return {
      width: this.width,
      height: this.height,
      rotation: this.rotation,
      brightness: this.brightness,
      backgroundColor: this.backgroundColor,
      segments: this.segments.map(s => ({
        id: s.id,
        enabled: s.enabled,
        text: s.text,
        color: s.color,
        x: s.x,
        y: s.y,
        w: s.w,
        h: s.h
      }))
    };
  }
}

module.exports = {
  RenderEngine,
  EFFECTS,
  ALIGN
};
