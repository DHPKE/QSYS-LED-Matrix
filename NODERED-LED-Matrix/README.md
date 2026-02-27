# Node-RED LED Matrix Controller v2.1 🎯

**One node, flat message structure - no msg.command hierarchy!**

## ✨ Simplified Command Syntax

Send commands using simple, flat message properties:

```javascript
// Before (old way)
msg.command = "layout";
msg.preset = 7;

// Now (v2.1)
msg.layout = 7;  // Clean and simple!
```

---

## 🚀 Quick Examples

### Display Text
```javascript
msg.text = "Hello World";
msg.segment = 0;
msg.color = "00FF00";
```

Or use `msg.payload`:
```javascript
msg.payload = "Temperature: 22°C";
msg.segment = 1;
```

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

## ✨ Why v2.1?

✅ **Simpler** - No `msg.command` hierarchy  
✅ **Cleaner** - Flat message structure  
✅ **Intuitive** - `msg.layout` instead of `msg.command + msg.preset`  
✅ **Less typing** - More productive  
✅ **Flexible** - Multiple outputs to same node works perfectly

---

**Version 2.1.0 - Clean, Simple, Effective! 🚀**
