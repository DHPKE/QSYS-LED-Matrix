# node-red-contrib-led-matrix

Node-RED node for controlling **128×64 BGR LED Matrix** panels via UDP.

Compatible with [QSYS-LED-Matrix](https://github.com/DHPKE/QSYS-LED-Matrix) v7.1+ Raspberry Pi controller.

## Features

- **128×64 BGR LED Matrix** support
- **All parameters configurable** from Node-RED
- **Rotation support** (0°, 90°, 180°, 270°) - persists across reboots
- **Font sizes** from 6px to 64px (auto-fit or fixed)
- **Text effects** (none, scroll, fade, blink)
- **Curtain mode** with auto-scale (3px frame)
- **8 independent groups** with segment management
- **9 layout presets** (fullscreen, splits, VO modes)
- **Frame/border** per segment (configurable color & width)
- **Color pickers** in UI for easy color selection
- **Node defaults** with message override

## Installation

```bash
cd ~/.node-red
npm install node-red-contrib-led-matrix
```

Or install via Node-RED palette manager: Search for "led-matrix"

## Quick Start

### Send Text
```javascript
msg.text = "Hello World";
msg.segment = 0;        // Segment 0-3
msg.color = "FF0000";   // Red text
msg.bgcolor = "000000"; // Black background
return msg;
```

### Change Layout
```javascript
msg.layout = 7;  // Quad view (4 segments)
return msg;
```

### Rotate Display
```javascript
msg.rotation = 90;  // Rotate 90° clockwise
return msg;
```

### Show Curtain
```javascript
msg.curtainEnable = true;
msg.curtainColor = "FF0000";  // Red frame
msg.group = 1;
return msg;
```

## Configuration

### Node Properties

All properties are **optional defaults**. Leave empty to use message values.

- **IP Address**: Default: `10.10.10.99` (fallback static IP)
- **UDP Port**: Default: `21324`
- **Segment**: Default segment (0-3)
- **Group**: Default group (0-8, 0=all)
- **Color**: Default text color (hex)
- **Background**: Default background color (hex)
- **Font**: arial, mono
- **Size**: auto or 6-64 pixels
- **Alignment**: L (left), C (center), R (right)
- **Effect**: none, scroll, fade, blink
- **Intensity**: 0-255
- **Layout**: 1-9, 11-14
- **Brightness**: 0-255
- **Rotation**: 0, 90, 180, 270
- **Frame**: Enable/disable segment border
- **Curtain**: Enable/disable 3px frame

### Message Properties

**Text Display**:
```javascript
{
  text: "Hello",           // Text to display (or use payload)
  segment: 0,              // Segment 0-3
  group: 1,                // Group 0-8 (0=all)
  color: "FFFFFF",         // Text color hex
  bgcolor: "000000",       // Background color hex
  font: "arial",           // Font: arial, mono
  size: 32,                // Size: auto or 6-64
  align: "C",              // Align: L, C, R
  effect: "scroll",        // Effect: none, scroll, fade, blink
  intensity: 255           // Intensity 0-255
}
```

**Layout & Display**:
```javascript
{
  layout: 7,               // Layout preset 1-9, 11-14
  brightness: 128,         // Brightness 0-255
  rotation: 90,            // Rotation 0, 90, 180, 270
  group: 0                 // Optional group filter
}
```

**Clear**:
```javascript
{
  clear: 0,                // Clear segment 0-3
  group: 1                 // Optional group filter
}

// Or clear all:
{ clear: "all" }
```

**Frame**:
```javascript
{
  frame: true,             // Enable frame
  segment: 0,              // Target segment
  framecolor: "FF0000",    // Frame color hex
  framewidth: 2,           // Width 1-5
  group: 0                 // Optional
}
```

**Curtain Mode**:
```javascript
{
  curtainEnable: true,     // Show curtain
  curtainColor: "FF0000",  // Curtain color hex
  group: 1                 // Group 1-8 (0=all)
}
```

**Test Mode** (via HTTP):
```javascript
{
  testmode: true           // Enable test mode (color bars + IP)
}
```

## Layouts

| Preset | Description |
|--------|-------------|
| 1 | Fullscreen (single segment) |
| 2 | Top / Bottom (2 segments) |
| 3 | Left / Right (2 segments) |
| 4 | Triple Left (3 segments) |
| 5 | Triple Right (3 segments) |
| 6 | Thirds (3 segments) |
| 7 | Quad View (4 segments) |
| 8 | VO Left (main + small) |
| 9 | VO Right (main + small) |
| 11-14 | Fullscreen per segment |

## Curtain Mode

Curtain mode creates a **3-pixel frame** around the entire display (all edges).

### Auto-Scale Behavior
- **Layouts 1-7, 11-14**: Auto-scale to **122×58** when curtain enabled
- **VO layouts 8-9**: Keep fixed **3px margins** (no scaling)

### Use Cases
- **Status indication** (red = error, green = ok)
- **Zone highlighting** per group
- **Frame overlay** on all segments

## Groups

Groups allow independent control of segment sets:

```javascript
// Define group 1 with segments 0 and 1
msg.group = 1;
msg.segments = [0, 1];

// Send text to group 1
msg.text = "Group 1";
msg.group = 1;

// Broadcast to all groups
msg.group = 0;
```

## Font Sizes

Fixed sizes: 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 36, 40, 44, 48, 52, 56, 60, 64 pixels

Or use `"auto"` to fit text to segment size.

## Effects

- **none**: Static text
- **scroll**: Horizontal scroll
- **fade**: Fade in/out
- **blink**: Blink on/off

## Hardware

### Display Specs
- **Resolution**: 128×64 pixels
- **Color order**: BGR (not RGB)
- **Usable area**: 122×58 (with curtain)
- **Anti-flicker**: PWM_BITS=5 (optimized)
- **Text padding**: 1px from edges

### Raspberry Pi Controller
- **QSYS-LED-Matrix** v7.1+
- **UDP port**: 21324
- **HTTP port**: 8080 (test mode)
- **Network**: DHCP + fallback 10.10.10.99/24

## Examples

### Dynamic Temperature Display
```javascript
const temp = msg.payload.temperature;
msg.text = temp + "°C";
msg.segment = 0;
msg.color = temp > 25 ? "FF0000" : "00FF00";
msg.size = 48;
return msg;
```

### Scrolling Status Text
```javascript
msg.text = "System Status: All OK";
msg.segment = 1;
msg.effect = "scroll";
msg.color = "00FF00";
return msg;
```

### Multi-Segment Dashboard
```javascript
// Layout: Quad view
flow.set("layout", { layout: 7 });

// Top-left: Time
msg[0] = { segment: 0, text: new Date().toLocaleTimeString() };

// Top-right: Temp
msg[1] = { segment: 1, text: "22°C", color: "00FF00" };

// Bottom-left: Status
msg[2] = { segment: 2, text: "ONLINE", color: "FFFFFF" };

// Bottom-right: Counter
msg[3] = { segment: 3, text: String(flow.get("counter") || 0) };

return msg;
```

### Error Notification with Red Curtain
```javascript
// Show error text
msg[0] = {
  text: "ERROR",
  segment: 0,
  color: "FF0000",
  effect: "blink"
};

// Enable red curtain
msg[1] = {
  curtainEnable: true,
  curtainColor: "FF0000",
  group: 1
};

return msg;
```

## Priority

**Message values always override node configuration.**

Node defaults are used only when message doesn't provide a value.

## License

MIT

## Repository

https://github.com/DHPKE/QSYS-LED-Matrix

## Changelog

### v3.0.0 (2026-03-09)
- **128×64 BGR** matrix support
- **Rotation** support (0°, 90°, 180°, 270°)
- **Font sizes** 6-64px + auto-fit
- **Effects** (scroll, fade, blink)
- **Curtain auto-scale** (layouts 1-7, 11-14)
- **Group** parameter support
- **VO layouts** 8-9 (voice-over modes)
- Updated default IP to **10.10.10.99** (fallback static)
- Enhanced UI with all parameters

### v2.4.0
- Curtain mode support (2px frame)
- Frame color & width configuration

### v2.0.0
- Initial release (64×32 RGB support)
