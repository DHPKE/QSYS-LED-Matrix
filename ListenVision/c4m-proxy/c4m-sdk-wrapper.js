/**
 * c4m-sdk-wrapper.js
 * 
 * Wrapper around the ListenVision C4M SDK dynamic library (.so on Linux, .dll on Windows)
 * Exposes functions to create programs, add text/image areas, and send to C4M cards.
 * 
 * This uses Node.js ffi-napi to call C functions from the SDK library.
 */

const ffi = require('ffi-napi');
const ref = require('ref-napi');
const path = require('path');

// Type definitions
const int = ref.types.int;
const void_ptr = ref.refType(ref.types.void);
const char_ptr = ref.types.CString;

// Determine library path based on platform
const LIB_PATH = process.platform === 'win32' 
  ? path.join(__dirname, 'lib', 'ledplayer7.dll')
  : path.join(__dirname, 'lib', 'libsledplayer7.so');

// LED Type constants (from SDK)
const LED_TYPE = {
  T_A_XC: 0,       // 6th gen T/A/XC series
  E: 1,             // 6th gen E series
  X1_X2: 2,         // X1/X2
  C: 3,             // C series (C2M, C4M, C2A)
  E5_E6_C8: 4       // E5, E6, C8
};

// Send Type constants
const SEND_TYPE = {
  TCP: 0,           // Fixed IP (TCP)
  UDP_BROADCAST: 1, // UDP broadcast
  SERIAL: 2,        // Serial port
  DISK: 3,          // Save to disk
  WAN: 4            // Wide area network
};

// Color constants (RGB)
const COLOR = {
  RED: 0xff,
  GREEN: 0xff00,
  YELLOW: 0xffff,
  BLUE: 0xff0000,
  PURPLE: 0xff00ff,
  CYAN: 0xffff00,
  WHITE: 0xffffff
};

// Color type (full color)
const COLOR_TYPE = {
  MONO: 1,
  DUAL: 2,
  RGB: 3,
  FULL: 4
};

// Effect styles (from SDK, 0-38+)
const EFFECT = {
  STATIC: 0,
  IMMEDIATE: 1,
  OPEN_UP: 2,
  OPEN_DOWN: 3,
  OPEN_LEFT: 4,
  OPEN_RIGHT: 5,
  SCROLL_LEFT: 6,
  SCROLL_RIGHT: 7,
  SCROLL_UP: 8,
  SCROLL_DOWN: 9,
  // ... add more as needed from SDK documentation
};

// Structure definitions (matching C structs)
const COMMUNICATIONINFO = ref.types.void; // We'll use a buffer
const AREARECT = ref.types.void;
const FONTPROP = ref.types.void;
const PLAYPROP = ref.types.void;

/**
 * Load the C4M SDK library and define functions
 */
class C4MSDK {
  constructor() {
    try {
      this.lib = ffi.Library(LIB_PATH, {
        // Initialize LED system
        'LV_InitLed': [int, [int, int]],
        
        // Create program
        'LV_CreateProgramEx': [void_ptr, [int, int, int, int, int]],
        
        // Delete program
        'LV_DeleteProgram': [int, [void_ptr]],
        
        // Add program
        'LV_AddProgram': [int, [void_ptr, int, int, int]],
        
        // Add text area
        'LV_AddImageTextArea': [int, [void_ptr, int, int, void_ptr, int]],
        
        // Quick add single line text (simplified)
        'LV_QuickAddSingleLineTextArea': [int, [void_ptr, int, int, void_ptr, int, char_ptr, void_ptr, int]],
        
        // Add single line text to area
        'LV_AddSingleLineTextToImageTextArea': [int, [void_ptr, int, int, int, char_ptr, void_ptr, void_ptr]],
        
        // Add multi-line text to area
        'LV_AddMultiLineTextToImageTextArea': [int, [void_ptr, int, int, int, char_ptr, void_ptr, void_ptr, int, int]],
        
        // Add file to area (images)
        'LV_AddFileToImageTextArea': [int, [void_ptr, int, int, char_ptr, void_ptr]],
        
        // Send program to card
        'LV_Send': [int, [void_ptr, void_ptr]],
        
        // Get error string
        'LV_GetError': [int, [int, int, char_ptr]],
        
        // Set basic info (screen parameters)
        'LV_SetBasicInfoEx': [int, [void_ptr, int, int, int, int]]
      });
      
      console.log('✓ C4M SDK library loaded successfully');
    } catch (error) {
      console.error('✗ Failed to load C4M SDK library:', error.message);
      console.error('  Expected library at:', LIB_PATH);
      throw error;
    }
  }

  /**
   * Initialize LED system
   * @param {number} ledType - LED type (use LED_TYPE constants)
   * @param {number} rgbSequence - RGB sequence order (0 for default)
   */
  initLed(ledType, rgbSequence = 0) {
    return this.lib.LV_InitLed(ledType, rgbSequence);
  }

  /**
   * Create a communication info structure
   */
  createCommInfo(config) {
    const buffer = Buffer.alloc(256); // Allocate enough space
    let offset = 0;
    
    // LEDType (int)
    buffer.writeInt32LE(config.ledType || LED_TYPE.C, offset);
    offset += 4;
    
    // SendType (int)
    buffer.writeInt32LE(config.sendType || SEND_TYPE.TCP, offset);
    offset += 4;
    
    // IpStr (char[16])
    if (config.ip) {
      buffer.write(config.ip, offset, 16, 'ascii');
    }
    offset += 16;
    
    // Commport (int) - unused on Linux
    buffer.writeInt32LE(0, offset);
    offset += 4;
    
    // Baud (int)
    buffer.writeInt32LE(config.baud || 0, offset);
    offset += 4;
    
    // LedNumber (int)
    buffer.writeInt32LE(config.ledNumber || 1, offset);
    offset += 4;
    
    // OutputDir (char[260]) - skip for TCP/UDP
    offset += 260;
    
    // NetworkIdStr (char[19])
    if (config.networkId) {
      buffer.write(config.networkId, offset, 19, 'ascii');
    }
    
    return buffer;
  }

  /**
   * Create an area rect structure
   */
  createAreaRect(left, top, width, height) {
    const buffer = Buffer.alloc(16);
    buffer.writeInt32LE(left, 0);
    buffer.writeInt32LE(top, 4);
    buffer.writeInt32LE(width, 8);
    buffer.writeInt32LE(height, 12);
    return buffer;
  }

  /**
   * Create a font property structure
   */
  createFontProp(config) {
    const buffer = Buffer.alloc(280); // MAX_PATH + font properties
    let offset = 0;
    
    // FontPath (char[260])
    if (config.fontPath) {
      buffer.write(config.fontPath, offset, 260, 'utf8');
    }
    offset += 260;
    
    // FontSize (int)
    buffer.writeInt32LE(config.fontSize || 16, offset);
    offset += 4;
    
    // FontColor (COLORREF/DWORD)
    buffer.writeUInt32LE(config.fontColor || COLOR.RED, offset);
    offset += 4;
    
    // FontBold (BOOL)
    buffer.writeInt32LE(config.fontBold ? 1 : 0, offset);
    offset += 4;
    
    // FontItalic (BOOL)
    buffer.writeInt32LE(config.fontItalic ? 1 : 0, offset);
    offset += 4;
    
    // FontUnderLine (BOOL)
    buffer.writeInt32LE(config.fontUnderLine ? 1 : 0, offset);
    
    return buffer;
  }

  /**
   * Create a play property structure
   */
  createPlayProp(config) {
    const buffer = Buffer.alloc(16);
    
    // InStyle (int)
    buffer.writeInt32LE(config.inStyle || EFFECT.STATIC, 0);
    
    // OutStyle (int) - reserved, set to 0
    buffer.writeInt32LE(0, 4);
    
    // Speed (int) 1-255
    buffer.writeInt32LE(config.speed || 10, 8);
    
    // DelayTime (int) 1-65535
    buffer.writeInt32LE(config.delayTime || 3, 12);
    
    return buffer;
  }

  /**
   * Create a program handle
   */
  createProgram(width, height, colorType = COLOR_TYPE.FULL, grayLevel = 0, saveType = 3) {
    // saveType: 0 = flash, 3 = RAM (use RAM for 10Hz updates!)
    return this.lib.LV_CreateProgramEx(width, height, colorType, grayLevel, saveType);
  }

  /**
   * Delete program and free memory
   */
  deleteProgram(hProgram) {
    return this.lib.LV_DeleteProgram(hProgram);
  }

  /**
   * Add a program
   */
  addProgram(hProgram, programNo, programTime, loopCount) {
    return this.lib.LV_AddProgram(hProgram, programNo, programTime, loopCount);
  }

  /**
   * Add an image/text area
   */
  addImageTextArea(hProgram, programNo, areaNo, areaRect, layout = 1) {
    return this.lib.LV_AddImageTextArea(hProgram, programNo, areaNo, areaRect, layout);
  }

  /**
   * Quick add single line text (simplified API)
   */
  quickAddSingleLineText(hProgram, programNo, areaNo, areaRect, text, fontProp, speed) {
    const ADDTYPE_STRING = 0;
    return this.lib.LV_QuickAddSingleLineTextArea(
      hProgram, programNo, areaNo, areaRect, 
      ADDTYPE_STRING, text, fontProp, speed
    );
  }

  /**
   * Send program to C4M card
   */
  send(commInfo, hProgram) {
    return this.lib.LV_Send(commInfo, hProgram);
  }

  /**
   * Get error message
   */
  getError(errorCode) {
    const buffer = Buffer.alloc(256);
    this.lib.LV_GetError(errorCode, 256, buffer);
    return buffer.toString('utf8').replace(/\0.*$/, '');
  }
}

module.exports = {
  C4MSDK,
  LED_TYPE,
  SEND_TYPE,
  COLOR,
  COLOR_TYPE,
  EFFECT
};
