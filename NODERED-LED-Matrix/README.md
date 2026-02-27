# Node-RED LED Matrix Controller v2.2 🎯

**One node, flat message structure with optional defaults!**

## ✨ Flexible Configuration

Set default values in the node properties, or leave them empty to use message values:

- **Set in node:** Values used when message doesn't provide them
- **Leave empty:** Always use values from incoming messages
- **Message always wins:** Message values override node defaults

Perfect for:
- **Fixed displays:** Set segment/color once in node, just send text
- **Dynamic displays:** Leave empty, control everything via messages
- **Hybrid:** Set some defaults, override others per message

## 🎨 Node Configuration

Configure defaults in the node properties:

### Connection (Required)
- **IP Address:** LED Matrix IP (default: 10.1.1.24)
- **UDP Port:** UDP port (default: 21324)

### Text Defaults (Optional)
- **Segment:** Default segment (0-3) or leave empty
- **Text Color:** Default text color (hex) or leave empty
- **Background:** Default background color or leave empty
- **Font:** Default font (arial/mono) or leave empty
- **Alignment:** Default alignment (L/C/R) or leave empty
- **Intensity:** Default intensity (0-255) or leave empty

**Leave any field empty to always use message values.**

---

## 💡 Usage Patterns

### Pattern 1: Fixed Segment Display
Configure in node:
- Segment: 0
- Color: 00FF00 (green)
- Font: mono

Then just send:
```javascript
msg.payload = "22°C";  // Uses node defaults
```

### Pattern 2: Dynamic Control
Leave all fields empty in node.

Send complete messages:
```javascript
msg.text = "Hello";
msg.segment = 0;
msg.color = "FF0000";
```

### Pattern 3: Hybrid (Recommended)
Set common defaults in node:
- Font: mono
- Alignment: C

Override per message:
```javascript
msg.text = "Warning!";
msg.segment = 1;
msg.color = "FF0000";  // Override for this message
// Uses node defaults for font and alignment
```

---

## 🚀 Quick Examples

### 🔤 Text Display
```javascript
// Minimal (uses node defaults if configured)
msg.payload = "Hello";

// Full control (overrides node defaults)
msg.text = "Hello World";
msg.segment = 0;             // Segment 0-3
msg.color = "FF0000";        // Text color (hex)
msg.bgcolor = "000000";      // Background color
msg.font = "arial";          // "arial" or "mono"
msg.align = "C";             // "L", "C", "R"
msg.intensity = 255;         // 0-255
```

**Priority:** Message values > Node defaults > Built-in defaults

### Change Layout
```javascript
msg.layout = 7;  // Quad view
```

### Set Brightness
```javascript
msg.brightness = 128;  // 0-255
```

### Clear Display
```javascript
msg.clear = "all";  // Clear everything
// or
msg.clear = 0;      // Clear segment 0
```

---

## 📋 All Commands

### 🔤 Text Display
```javascript
msg.text = "Hello";          // Text to display (or msg.payload)
msg.segment = 0;             // Segment 0-3
msg.color = "FF0000";        // Text color (hex)
msg.bgcolor = "000000";      // Background color
msg.font = "arial";          // "arial" or "mono"
msg.align = "C";             // "L", "C", "R"
msg.intensity = 255;         // 0-255
```

### 📐 Layout
```javascript
msg.layout = 7;              // Layout preset 1-7, 11-14
```

**Presets:**
- 1 - Fullscreen
- 2 - Top/Bottom (2 segments)
- 3 - Left/Right (2 segments)
- 4 - Triple Left (3 segments)
- 5 - Triple Right (3 segments)
- 6 - Thirds (3 columns)
- 7 - Quad View (4 segments)
- 11 - Fullscreen Segment 1
- 12 - Fullscreen Segment 2
- 13 - Fullscreen Segment 3
- 14 - Fullscreen Segment 4

### ☀️ Brightness
```javascript
msg.brightness = 200;        // 0-255 (0=off, 255=max)
```

### 🧹 Clear
```javascript
msg.clear = "all";           // Clear entire display
// or
msg.clear = 0;               // Clear segment 0 (0-3)
// or
msg.clear = true;            // Also clears all
```

### 🔄 Orientation
```javascript
msg.orientation = 180;       // 0, 90, 180, 270
```

### 👥 Group
```javascript
msg.group = 1;               // Group number 0-8
msg.segments = [0, 1];       // Segments in group
```

### ⚙️ Config
```javascript
msg.config = {
    network: true,           // Get network info
    name: true,             // Get device name
    orientation: true       // Get orientation
};
```

---

## 💡 Common Use Cases

### Real-Time Clock
```
[Inject: every 1s] → [Function] → [LED Matrix]
```

Function:
```javascript
msg.text = new Date().toLocaleTimeString();
msg.segment = 0;
msg.color = "00FFFF";
msg.font = "mono";
return msg;
```

### Temperature Display with Color
```javascript
const temp = msg.payload;
msg.text = `${temp}°C`;
msg.segment = 0;
msg.color = temp > 25 ? "FF0000" : "00FF00";  // Red if hot
return msg;
```

### Dashboard with Layout Buttons
```
[Button: Quad] → [Change: msg.layout=7] → [LED Matrix]
[Button: Full] → [Change: msg.layout=1] → [LED Matrix]
```

### Time-Based Brightness
```javascript
const hour = new Date().getHours();
msg.brightness = (hour >= 22 || hour < 7) ? 50 : 200;
return msg;
```

### Four-Segment Display
```javascript
// Use function with 4 outputs
return [
    { text: "CPU: 45%", segment: 0, color: "00FF00" },
    { text: "RAM: 2GB", segment: 1, color: "00FFFF" },
    { text: "Disk: 50GB", segment: 2, color: "FFFF00" },
    { text: "22°C", segment: 3, color: "FF8800" }
];
```

Connect all 4 outputs to the same LED Matrix node!

---

## 📦 Installation

```bash
cd ~/.node-red
npm install /path/to/QSYS-LED-Matrix/NODERED-LED-Matrix
node-red-restart
```

Find the **led-matrix** node in your palette under "output" category.

---

## 🎯 Node Configuration

Drag the **LED Matrix** node onto your canvas and configure:

- **Name:** Display name (optional)
- **IP Address:** LED Matrix IP (default: 10.1.1.24)
- **UDP Port:** UDP port (default: 21324)

That's it! Now send messages to control it.

---

## 🔄 Message Priority

The node detects the command automatically from message properties:

1. **Text** - If `msg.text` or `msg.payload` exists
2. **Layout** - If `msg.layout` exists
3. **Brightness** - If `msg.brightness` exists
4. **Clear** - If `msg.clear` exists
5. **Orientation** - If `msg.orientation` exists
6. **Group** - If `msg.group` exists
7. **Config** - If `msg.config` exists

Only **one command per message** is processed.

---

## 📊 Examples File

Import `examples.json` to get ready-to-use flows:

1. ✅ Text display with color
2. ✅ Layout switching (Quad/Full)
3. ✅ Brightness control
4. ✅ Clear display
5. ✅ Real-time clock
6. ✅ Four-segment display

---

## 🆚 v2.1 vs Old Versions

| Feature | Old (v1.0) | New (v2.1) |
|---------|-----------|-----------|
| **Command Style** | `msg.command="layout"`<br/>`msg.preset=7` | `msg.layout=7` |
| **Text Display** | `msg.command="text"`<br/>`msg.text="Hello"` | `msg.text="Hello"` |
| **Brightness** | `msg.command="brightness"`<br/>`msg.brightness=128` | `msg.brightness=128` |
| **Clear** | `msg.command="clear"` or `"clear_all"` | `msg.clear=0` or `"all"` |
| **Complexity** | 🟡 Medium | 🟢 Simple |
| **Typing** | More | Less |

---

## 🐛 Troubleshooting

**Node not appearing?**
```bash
node-red-restart
```

**Commands not working?**
- Check IP address matches your LED Matrix
- Verify UDP port 21324 is correct
- Test with simple command: `msg.layout = 1`

**Text not displaying?**
- Ensure `msg.text` or `msg.payload` contains a string
- Check segment number (0-3)
- Verify color format (6-digit hex without #)

---

## 📚 Full Documentation

- **Installation:** See INSTALL.md
- **Quick Reference:** See QUICK-REFERENCE.md
- **Protocol Details:** See main README.md

---

## ✨ Why v2.2?

✅ **Simpler** - No `msg.command` hierarchy  
✅ **Cleaner** - Flat message structure  
✅ **Flexible defaults** - Set once in node or control via messages  
✅ **Message priority** - Message values always override node defaults  
✅ **Less typing** - Set defaults for common values, only send what changes  
✅ **Works both ways** - Fixed displays OR dynamic control

---

**Version 2.2.0 - Flexible Configuration with Smart Defaults! 🚀**
