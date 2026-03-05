/**
 * render-engine.js - Enhanced with full RPi feature parity
 * 
 * Renders text/graphics to a Canvas matching the RPi implementation,
 * then converts to a bitmap that can be sent via C4M SDK.
 * 
 * NEW FEATURES (v2.0):
 * - Background colors per segment (bgcolor)
 * - Font selection (arial, digital)
 * - Size modes (auto, small, medium, large)
 * - Intensity/opacity control
 * - Frame/borders per segment
 * - Curtain mode (3px edge bars per group)
 * - Orientation (landscape/portrait)
 * - Manual segment config
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
};

// Alignment constants
const ALIGN = {
  LEFT: 0,
  CENTER: 1,
  RIGHT: 2
};

// Font mappings
const FONTS = {
  'arial': 'Arial',
  'digital': 'Courier New',
  'dejavu': 'DejaVu Sans'
};

// Size modes (as multipliers)
const SIZES = {
  'auto': 0.8,    // Fit to segment height
  'small': 0.5,
  'medium': 0.7,
  'large': 0.9
};

class RenderEngine {
  constructor(width = 64, height = 32) {
    this.width = width;
    this.height = height;
    this.rotation = 0; // 0, 90, 180, 270
    this.orientation = 'landscape'; // 'landscape' or 'portrait'
    
    // Segments (matching RPi 4-segment system)
    this.segments = [];
    for (let i = 0; i < 4; i++) {
      this.segments.push({
        id: i,
        x: 0,
        y: i * 8,
        w: 64,
        h: 8,
        enabled: false,
        text: '',
        color: '#FF0000',
        bgcolor: '#000000',
        font: 'arial',
        size: 'auto',
        intensity: 255,
        effect: EFFECTS.STATIC,
        align: ALIGN.LEFT,
        // Frame properties
        frame: {
          enabled: false,
          color: '#FFFFFF',
          width: 1
        }
      });
    }
    
    // Curtain mode (3px bars on edges, per-group)
    this.curtains = {};
    for (let group = 1; group <= 8; group++) {
      this.curtains[group] = {
        enabled: false,
        color: '#FFFFFF',
        configured: false
      };
    }
    this.activeGroup = 0; // Current group (0 = all)
    
    this.backgroundColor = '#000000';
    this.brightness = 255;
    this.displayEnabled = true;
    
    // Font settings
    this.defaultFont = 'DejaVu Sans';
    this.defaultFontSize = 8;
    
    // Register system fonts
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
   * Set segment properties (enhanced with all RPi features)
   */
  setSegment(id, props) {
    if (id < 0 || id >= this.segments.length) return;
    
    const seg = this.segments[id];
    
    // Basic properties
    if (props.enabled !== undefined) seg.enabled = props.enabled;
    if (props.text !== undefined) seg.text = props.text;
    if (props.color !== undefined) seg.color = this.normalizeColor(props.color);
    
    // NEW: Enhanced properties
    if (props.bgcolor !== undefined) seg.bgcolor = this.normalizeColor(props.bgcolor);
    if (props.font !== undefined) seg.font = props.font;
    if (props.size !== undefined) seg.size = props.size;
    if (props.intensity !== undefined) seg.intensity = Math.max(0, Math.min(255, props.intensity));
    
    // Effect and alignment
    if (props.effect !== undefined) seg.effect = props.effect;
    if (props.align !== undefined) seg.align = props.align;
    
    // Position/size
    if (props.x !== undefined) seg.x = props.x;
    if (props.y !== undefined) seg.y = props.y;
    if (props.w !== undefined) seg.w = props.w;
    if (props.h !== undefined) seg.h = props.h;
  }

  /**
   * Configure segment position/size manually
   */
  configureSegment(id, x, y, w, h) {
    if (id < 0 || id >= this.segments.length) return;
    
    const seg = this.segments[id];
    seg.x = x;
    seg.y = y;
    seg.w = w;
    seg.h = h;
    
    console.log(`[RENDER] Segment ${id} configured: ${x},${y} ${w}×${h}`);
  }

  /**
   * Set frame properties for a segment
   */
  setFrame(id, enabled, color = '#FFFFFF', width = 1) {
    if (id < 0 || id >= this.segments.length) return;
    
    const seg = this.segments[id];
    seg.frame.enabled = enabled;
    seg.frame.color = this.normalizeColor(color);
    seg.frame.width = Math.max(1, Math.min(3, width));
    
    console.log(`[RENDER] Segment ${id} frame: ${enabled ? 'ON' : 'OFF'} ${color} ${width}px`);
  }

  /**
   * Configure curtain for a group
   */
  configureCurtain(group, enabled, color) {
    if (group < 1 || group > 8) return;
    
    this.curtains[group] = {
      enabled: enabled,
      color: this.normalizeColor(color),
      configured: true
    };
    
    console.log(`[RENDER] Curtain group ${group}: ${enabled ? 'SHOW' : 'HIDE'} ${color}`);
  }

  /**
   * Set active group (for curtain rendering)
   */
  setGroup(group) {
    this.activeGroup = group;
    console.log(`[RENDER] Active group: ${group}`);
  }

  /**
   * Set display enable/disable
   */
  setDisplayEnabled(enabled) {
    this.displayEnabled = enabled;
    console.log(`[RENDER] Display: ${enabled ? 'ENABLED' : 'DISABLED'}`);
  }

  /**
   * Set orientation
   */
  setOrientation(value) {
    this.orientation = value === 'portrait' ? 'portrait' : 'landscape';
    console.log(`[RENDER] Orientation: ${this.orientation}`);
    
    // Adjust canvas dimensions if needed
    if (this.orientation === 'portrait') {
      this.canvas = createCanvas(32, 64);
    } else {
      this.canvas = createCanvas(64, 32);
    }
    this.ctx = this.canvas.getContext('2d');
  }

  /**
   * Set layout preset (matching RPi presets)
   */
  setLayout(preset) {
    // Landscape layouts (64×32)
    const landscapeLayouts = {
      1: [ // Fullscreen
        { x: 0, y: 0, w: 64, h: 32 }
      ],
      2: [ // Top/bottom split
        { x: 0, y: 0, w: 64, h: 16 },
        { x: 0, y: 16, w: 64, h: 16 }
      ],
      3: [ // Left/right split
        { x: 0, y: 0, w: 32, h: 32 },
        { x: 32, y: 0, w: 32, h: 32 }
      ],
      4: [ // Triple left
        { x: 0, y: 0, w: 32, h: 32 },
        { x: 32, y: 0, w: 32, h: 16 },
        { x: 32, y: 16, w: 32, h: 16 }
      ],
      5: [ // Triple right
        { x: 0, y: 0, w: 32, h: 16 },
        { x: 0, y: 16, w: 32, h: 16 },
        { x: 32, y: 0, w: 32, h: 32 }
      ],
      6: [ // Thirds
        { x: 0, y: 0, w: 20, h: 32 },
        { x: 20, y: 0, w: 22, h: 32 },
        { x: 42, y: 0, w: 22, h: 32 }
      ],
      7: [ // Quad
        { x: 0, y: 0, w: 32, h: 16 },
        { x: 32, y: 0, w: 32, h: 16 },
        { x: 0, y: 16, w: 32, h: 16 },
        { x: 32, y: 16, w: 32, h: 16 }
      ],
      11: [ { x: 0, y: 0, w: 64, h: 32 } ], // Seg 1 fullscreen
      12: [ {}, { x: 0, y: 0, w: 64, h: 32 } ], // Seg 2 fullscreen
      13: [ {}, {}, { x: 0, y: 0, w: 64, h: 32 } ], // Seg 3 fullscreen
      14: [ {}, {}, {}, { x: 0, y: 0, w: 64, h: 32 } ] // Seg 4 fullscreen
    };
    
    // Portrait layouts (32×64)
    const portraitLayouts = {
      1: [ { x: 0, y: 0, w: 32, h: 64 } ],
      2: [ // Top/bottom
        { x: 0, y: 0, w: 32, h: 32 },
        { x: 0, y: 32, w: 32, h: 32 }
      ],
      3: [ // Left/right (stacked in portrait)
        { x: 0, y: 0, w: 32, h: 21 },
        { x: 0, y: 21, w: 32, h: 43 }
      ]
    };
    
    const layouts = this.orientation === 'portrait' ? portraitLayouts : landscapeLayouts;
    const layout = layouts[preset];
    
    if (!layout) {
      console.warn(`[RENDER] Unknown layout preset ${preset}`);
      return;
    }
    
    // Apply layout
    layout.forEach((dims, i) => {
      if (i < this.segments.length && dims.w) {
        this.segments[i].x = dims.x;
        this.segments[i].y = dims.y;
        this.segments[i].w = dims.w;
        this.segments[i].h = dims.h;
        // Don't automatically enable - let caller decide
      }
    });
    
    console.log(`[RENDER] Layout ${preset} applied (${this.orientation})`);
  }

  /**
   * Normalize color from various formats to hex
   */
  normalizeColor(color) {
    if (typeof color === 'number') {
      return '#' + color.toString(16).padStart(6, '0');
    }
    if (typeof color === 'string') {
      if (color.startsWith('#')) return color;
      if (color.length === 6) return '#' + color;
      if (color.length === 8) return '#' + color.substring(0, 6); // Strip alpha
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
    // If display disabled, return blank canvas
    if (!this.displayEnabled) {
      this.clear();
      return this.canvas;
    }
    
    this.clear();
    
    // Render curtain bars first (underneath segments)
    this.renderCurtain();
    
    // Render each enabled segment
    for (const seg of this.segments) {
      if (!seg.enabled || !seg.text) continue;
      
      this.renderSegment(seg);
    }
    
    // Render frames on top
    for (const seg of this.segments) {
      if (!seg.enabled || !seg.frame.enabled) continue;
      
      this.renderFrame(seg);
    }
    
    return this.canvas;
  }

  /**
   * Render curtain bars (3px wide on left/right edges)
   */
  renderCurtain() {
    if (this.activeGroup === 0) return; // No curtain for broadcast
    
    const curtain = this.curtains[this.activeGroup];
    if (!curtain || !curtain.enabled || !curtain.configured) return;
    
    this.ctx.fillStyle = curtain.color;
    
    // Left bar (x=0-2)
    this.ctx.fillRect(0, 0, 3, this.height);
    
    // Right bar (x=61-63 for 64px width)
    this.ctx.fillRect(this.width - 3, 0, 3, this.height);
    
    console.log(`[RENDER] Curtain drawn: group ${this.activeGroup}, color ${curtain.color}`);
  }

  /**
   * Render a single segment
   */
  renderSegment(seg) {
    const { x, y, w, h, text, color, bgcolor, font, size, intensity, align } = seg;
    
    // Fill background
    if (bgcolor && bgcolor !== '#000000') {
      this.ctx.fillStyle = bgcolor;
      this.ctx.fillRect(x, y, w, h);
    }
    
    // Calculate font size based on mode
    let fontSize;
    const sizeMultiplier = SIZES[size] || SIZES['auto'];
    fontSize = Math.floor(h * sizeMultiplier);
    
    // Select font
    const fontFamily = FONTS[font] || FONTS['arial'];
    this.ctx.font = `${fontSize}px "${fontFamily}"`;
    
    // Apply intensity (opacity)
    const alpha = intensity / 255;
    const hexColor = this.normalizeColor(color);
    const r = parseInt(hexColor.substr(1, 2), 16);
    const g = parseInt(hexColor.substr(3, 2), 16);
    const b = parseInt(hexColor.substr(5, 2), 16);
    this.ctx.fillStyle = `rgba(${r}, ${g}, ${b}, ${alpha})`;
    
    this.ctx.textBaseline = 'middle';
    
    // Calculate text position based on alignment
    let textX = x;
    const textY = y + h / 2;
    
    if (align === ALIGN.LEFT) {
      this.ctx.textAlign = 'left';
      textX = x + 2;
    } else if (align === ALIGN.CENTER) {
      this.ctx.textAlign = 'center';
      textX = x + w / 2;
    } else if (align === ALIGN.RIGHT) {
      this.ctx.textAlign = 'right';
      textX = x + w - 2;
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
   * Render frame/border around segment
   */
  renderFrame(seg) {
    const { x, y, w, h, frame } = seg;
    if (!frame.enabled) return;
    
    this.ctx.strokeStyle = frame.color;
    this.ctx.lineWidth = frame.width;
    
    // Draw rectangle
    this.ctx.strokeRect(x + frame.width / 2, y + frame.width / 2, 
                       w - frame.width, h - frame.width);
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
    console.log(`[RENDER] Rotation: ${this.rotation}°`);
    
    // Note: Actual rotation transform would be applied during toC4MProgram()
    // For now, just track the value
  }

  /**
   * Get current state (for debugging)
   */
  getState() {
    return {
      width: this.width,
      height: this.height,
      rotation: this.rotation,
      orientation: this.orientation,
      brightness: this.brightness,
      displayEnabled: this.displayEnabled,
      activeGroup: this.activeGroup,
      curtains: this.curtains,
      segments: this.segments.map(s => ({
        id: s.id,
        enabled: s.enabled,
        text: s.text,
        color: s.color,
        bgcolor: s.bgcolor,
        font: s.font,
        size: s.size,
        intensity: s.intensity,
        x: s.x,
        y: s.y,
        w: s.w,
        h: s.h,
        frame: s.frame
      }))
    };
  }
}

module.exports = {
  RenderEngine,
  EFFECTS,
  ALIGN,
  FONTS,
  SIZES
};
