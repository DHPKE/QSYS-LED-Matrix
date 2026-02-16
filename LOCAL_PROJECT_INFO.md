# OlimexLED-Matrix Project Overview

## Project: OlimexLED-Matrix
**Location:** `/Users/user/.openclaw/workspace/OlimexLED-Matrix`  
**GitHub:** https://github.com/DHPKE/OlimexLED-Matrix.git  
**Status:** Cloned - Ready for Development

## What Is This?

**Olimex ESP32 Gateway LED Matrix Text Display** - Complete firmware solution for displaying dynamic text on a 64x32 pixel HUB75 LED matrix.

### Key Features:
- **UDP Text Control** (port 21324)
- **Multi-Segment Support** (up to 4 independent text areas)
- **Multiple Fonts** (Roboto, Digital, Monospace in various sizes)
- **Auto-Scaling** text to fit available space
- **Text Effects** (scrolling, blinking, fading, rainbow)
- **Web Interface** for configuration and testing
- **Q-SYS Integration** (ready-to-use Lua plugin)
- **Full RGB Color Support** (24-bit with hex codes)
- **Persistent Configuration** (saved to flash memory)

## Hardware Requirements

- **Olimex ESP32 Gateway** (ESP32-GATEWAY)
- **64x32 HUB75 LED Matrix Panel** (P3, P4, P5, or P6 pitch)
- **5V Power Supply** (minimum 4A for matrix panel)
- **Ethernet or WiFi** connection

## Project Structure

```
OlimexLED-Matrix/
├── arduino/
│   └── OlimexLED-Matrix/      # Main Arduino firmware
│       ├── OlimexLED-Matrix.ino
│       ├── config.h           # WiFi & pin config
│       ├── segment_manager.h  # Segment layouts
│       ├── text_renderer.h    # Text rendering
│       ├── udp_handler.h      # UDP protocol
│       └── fonts.h            # Font definitions
├── qsys-plugin/
│   └── led_matrix_controller.lua  # Q-SYS plugin
├── docs/                      # Setup guides
│   ├── ARDUINO_SETUP.md
│   ├── UDP_PROTOCOL.md
│   ├── QSYS_INTEGRATION.md
│   └── PINOUT.md
├── examples/                  # Example scripts
│   ├── send_text.py
│   ├── send_command.sh
│   └── led_matrix_client.js
└── README.md
```

## Quick Start Workflow

1. **Arduino IDE Setup**
   - Install ESP32 board support
   - Install required libraries (ESP32 HUB75, Adafruit GFX, ArduinoJson, AsyncWebServer, AsyncTCP)

2. **Configure WiFi**
   - Edit `arduino/OlimexLED-Matrix/config.h`
   - Set SSID and password

3. **Upload Firmware**
   - Connect Olimex ESP32 Gateway via USB
   - Upload sketch from Arduino IDE
   - Note IP address from Serial Monitor

4. **Test Web Interface**
   - Navigate to `http://[IP_ADDRESS]`
   - Send test text to segments

5. **Q-SYS Integration** (optional)
   - Copy `qsys-plugin/led_matrix_controller.lua` to Q-SYS Designer
   - Configure IP and port in plugin properties

## UDP Protocol Example

```
TEXT|0|Hello World|FF0000|roboto16|auto|C|none
```

**Format:**
```
TEXT|segment|content|color|font|size|align|effect
```

## Code Statistics

- **~2,000 lines of code**
- **22 files created**
- **Languages:** C++, Lua, Python, JavaScript, Bash
- **Documentation:** ~7,500 words

## Current Status

✅ **Cloned from GitHub**  
📂 **Location:** `/Users/user/.openclaw/workspace/OlimexLED-Matrix`  
🔧 **Ready for:** Development, testing, customization  

---

**Version:** v1.0.0 (initial release 2026-02-16)  
**License:** Open Source  
**Repository:** https://github.com/DHPKE/OlimexLED-Matrix.git
