# Node-RED LED Matrix Controller v2.0 🎯

**Simplified, dedicated nodes for easier LED Matrix control!**

## 🎉 What's New in v2.0

### Four Specialized Nodes

Instead of one complex node, you now get **four simple, focused nodes**:

1. **🔤 Text Node** - Display text on segments
2. **📐 Layout Node** - Switch between layout presets  
3. **☀️ Brightness Node** - Control display brightness
4. **🧹 Clear Node** - Clear segments or entire display

### Simplified Message Format

**Before (v1.0):**
```javascript
msg.command = "layout";
msg.preset = 7;
```

**Now (v2.0):**
```javascript
msg.layout = 7;  // That's it!
```

### Per-Segment Control

Create multiple text nodes, one for each segment:
- Drag **4 text nodes** onto canvas
- Configure each for segment 0, 1, 2, 3
- Send different messages to each
- Easy visual flow!

---

## 📦 Installation

```bash
cd ~/.node-red
npm install /path/to/QSYS-LED-Matrix/NODERED-LED-Matrix
node-red-restart
```

---

## 🚀 Quick Start Examples

### Example 1: Simple Text Display
```
[Inject: "Hello"] → [Text Node: Seg 0]
```

Configure the text node:
- Segment: 0
- Color: Green (00FF00)
- Just click inject!

### Example 2: Real-Time Clock
```
[Inject: every 1 sec] → [Function] → [Text Node: Seg 0]
```

Function code:
```javascript
msg.payload = new Date().toLocaleTimeString();
return msg;
```

### Example 3: Layout Switching
```
[Inject: msg.layout=7] → [Layout Node]
[Inject: msg.layout=1] → [Layout Node]
```

No function needed! Just set `msg.layout` in the inject node.

### Example 4: Four Segments at Once
```
[Inject] → [Text Seg 0: "Red"]
         → [Text Seg 1: "Green"]
         → [Text Seg 2: "Blue"]
         → [Text Seg 3: "Yellow"]
```

One trigger, four displays!

---

## 🎨 Node Types

### 🔤 Text Node (`led-matrix-text`)

Display text on a specific segment.

**Properties:**
- Segment (0-3)
- Color (hex)
- Background color
- Font (arial/mono)
- Alignment (L/C/R)
- Intensity (0-255)

**Input:**
```javascript
msg.payload = "Hello World";        // Text to display
msg.color = "FF0000";               // Optional: override color
msg.segment = 0;                    // Optional: override segment
```

**Use Case:** Perfect for status messages, sensor readings, clocks

---

### 📐 Layout Node (`led-matrix-layout`)

Switch between layout presets.

**Properties:**
- Preset (1-7, 11-14)

**Input:**
```javascript
msg.layout = 7;     // Quad view
// or
msg.payload = 1;    // Fullscreen
```

**Presets:**
- 1 - Fullscreen
- 2 - Top/Bottom
- 3 - Left/Right
- 4 - Triple Left
- 5 - Triple Right
- 6 - Thirds
- 7 - Quad View
- 11-14 - Single segment fullscreen

**Use Case:** Dashboard views, multi-panel layouts

---

### ☀️ Brightness Node (`led-matrix-brightness`)

Control display brightness.

**Properties:**
- Brightness (0-255)

**Input:**
```javascript
msg.brightness = 255;   // Max brightness
// or
msg.payload = 128;      // 50%
```

**Use Case:** Time-based dimming, auto-brightness, power saving

---

### 🧹 Clear Node (`led-matrix-clear`)

Clear segments or entire display.

**Properties:**
- Clear All (checkbox)
- Segment (0-3) if not clearing all

**Input:**
```javascript
msg.clearAll = true;    // Clear everything
// or
msg.segment = 0;        // Clear just segment 0
```

**Use Case:** Reset displays, clear old content, refresh

---

## 💡 Common Patterns

### Pattern 1: Temperature Display
```
[MQTT In] → [Function: Format] → [Text Node]
```

Function:
```javascript
msg.payload = `${msg.payload}°C`;
msg.color = msg.payload > 25 ? "FF0000" : "00FF00";
return msg;
```

### Pattern 2: Time-Based Brightness
```
[Inject: every 1 min] → [Function: Check Time] → [Brightness Node]
```

Function:
```javascript
const hour = new Date().getHours();
msg.brightness = (hour >= 22 || hour < 7) ? 50 : 200;
return msg;
```

### Pattern 3: Multi-Panel Display
```
[HTTP Request] → [Function: Split Data] → [Text Seg 0]
                                        → [Text Seg 1]
                                        → [Text Seg 2]
                                        → [Text Seg 3]
```

Function:
```javascript
const data = JSON.parse(msg.payload);
return [
    { payload: data.cpu + "%" },
    { payload: data.mem + "MB" },
    { payload: data.disk + "GB" },
    { payload: data.temp + "°C" }
];
```

### Pattern 4: Dashboard Toggle
```
[Dashboard Button: Quad] → [Change: msg.layout=7] → [Layout Node]
[Dashboard Button: Full] → [Change: msg.layout=1] → [Layout Node]
```

---

## 🎯 Message Reference

### Text Node
| Property | Type | Description |
|----------|------|-------------|
| `payload` | string | Text to display (required) |
| `segment` | number | 0-3 (overrides node config) |
| `color` | string | Hex color (FFFFFF) |
| `bgcolor` | string | Background hex |
| `align` | string | L/C/R |
| `intensity` | number | 0-255 |

### Layout Node
| Property | Type | Description |
|----------|------|-------------|
| `layout` | number | 1-7, 11-14 (recommended) |
| `preset` | number | Alternative to `layout` |
| `payload` | number | Alternative to `layout` |

### Brightness Node
| Property | Type | Description |
|----------|------|-------------|
| `brightness` | number | 0-255 (recommended) |
| `payload` | number | Alternative to `brightness` |

### Clear Node
| Property | Type | Description |
|----------|------|-------------|
| `clearAll` | boolean | Clear entire display |
| `segment` | number | 0-3 (if not clearing all) |

---

## 🔧 Advanced: Per-Panel Configuration

Control multiple panels with different IPs:

```javascript
// Panel 1
msg.ip = "10.1.1.24";
msg.payload = "Panel 1";

// Panel 2  
msg.ip = "10.1.1.25";
msg.payload = "Panel 2";
```

All nodes support `msg.ip` and `msg.port` override!

---

## 🆚 Comparison: v1.0 vs v2.0

| Feature | v1.0 (Old) | v2.0 (New) |
|---------|------------|------------|
| Node Count | 1 complex node | 4 simple nodes |
| Text Display | `msg.command="text"` | Just `msg.payload` |
| Layout | `msg.command="layout"`<br/>`msg.preset=7` | `msg.layout=7` |
| Brightness | `msg.command="brightness"`<br/>`msg.brightness=128` | `msg.brightness=128` |
| Multi-Segment | Complex function | Multiple text nodes |
| Visual Clarity | ❌ One node for everything | ✅ Clear, separate nodes |
| Ease of Use | ⭐⭐ | ⭐⭐⭐⭐⭐ |

---

## 📊 Example Flows Included

Import `examples-simplified.json` to get:

1. ✅ Simple text display
2. ✅ Real-time clock
3. ✅ Layout switching (msg.layout)
4. ✅ Brightness control (msg.brightness)
5. ✅ Four-segment display
6. ✅ Clear display

---

## 🎓 Migration from v1.0

If you have flows using v1.0:

**Replace this:**
```
[Inject] → [LED Matrix: command=text]
```

**With this:**
```
[Inject] → [Text Node: Seg 0]
```

**Replace this:**
```javascript
msg.command = "layout";
msg.preset = 7;
```

**With this:**
```javascript
msg.layout = 7;
```

Much cleaner! 🎉

---

## 🐛 Troubleshooting

**Nodes not appearing?**
- Restart Node-RED: `node-red-restart`
- Check installation: `npm list node-red-contrib-led-matrix`

**Text not displaying?**
- Check IP address in node config
- Verify Pi is online: `ping 10.1.1.24`
- Check segment configuration (0-3)

**Layout not working?**
- Use `msg.layout` instead of `msg.preset`
- Valid values: 1-7, 11-14

---

## 📚 More Info

- **Full protocol details:** See main README.md
- **Installation guide:** See INSTALL.md
- **Quick reference:** See QUICK-REFERENCE.md

---

## ✨ Key Benefits

✅ **Simpler** - One node per function  
✅ **Clearer** - Visual separation in flows  
✅ **Easier** - Just `msg.layout`, not `msg.command + msg.preset`  
✅ **Faster** - Less configuration, more action  
✅ **Flexible** - Mix and match as needed

---

**Version 2.0 - Simplified and Better! 🚀**

Ready to build amazing LED displays with less complexity!
