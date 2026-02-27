# Node-RED LED Matrix Controller

Control 64×32 RGB LED Matrix panels from Node-RED via UDP commands.

**Full feature parity with QSYS plugin** — All commands, parameters, and layouts from the Q-SYS LED Matrix plugin are supported.

---

## Features

✅ **All QSYS Plugin Commands**
- Text display with full styling (colors, fonts, alignment, effects)
- Segment control (0-3)
- Layout presets (1-7, 11-14)
- Brightness control (0-255)
- Clear segments or entire display
- Orientation switching (landscape/portrait)
- Group assignment (0-8)
- Manual segment configuration

✅ **Easy Integration**
- Drag-and-drop Node-RED node
- Visual configuration interface
- Message-based parameter override
- Pass-through with command metadata

✅ **Compatible**
- Works with QSYS-LED-Matrix RPi controller
- UDP JSON protocol
- Standard Node.js (no external dependencies)

---

## Installation

### Method 1: Install from npm (future)
```bash
cd ~/.node-red
npm install node-red-contrib-led-matrix
```

### Method 2: Install from local folder
```bash
cd ~/.node-red
npm install /path/to/QSYS-LED-Matrix/NODERED-LED-Matrix
```

### Method 3: Install from GitHub
```bash
cd ~/.node-red
npm install DHPKE/QSYS-LED-Matrix#main:NODERED-LED-Matrix
```

### Method 4: Manual installation
```bash
cd ~/.node-red/node_modules
ln -s /path/to/QSYS-LED-Matrix/NODERED-LED-Matrix node-red-contrib-led-matrix
```

After installation, restart Node-RED:
```bash
node-red-restart
# or
sudo systemctl restart nodered
```

---

## Quick Start

1. **Drag** the "led-matrix" node from the palette (output section)
2. **Configure** the IP address and port of your LED Matrix controller
3. **Select** a command type (text, brightness, layout, etc.)
4. **Set** default parameters in the node editor
5. **Connect** an inject or function node to trigger commands
6. **Deploy** and test!

---

## Usage Examples

### Example 1: Display Simple Text
```javascript
// Inject node or function node:
msg.payload = "Hello World";
return msg;
```

Configure the LED Matrix node:
- Command: `Text`
- Segment: `0`
- Color: `FFFFFF` (white)
- Background: `000000` (black)

### Example 2: Dynamic Text with Color
```javascript
msg.payload = "Temperature: 22°C";
msg.segment = 0;
msg.color = "00FF00";  // Green
msg.bgcolor = "000000"; // Black
msg.align = "C";        // Center
return msg;
```

### Example 3: Set Brightness
```javascript
msg.command = "brightness";
msg.brightness = 128;  // 50%
return msg;
```

### Example 4: Apply Layout Preset
```javascript
msg.command = "layout";
msg.preset = 7;  // Quad view
return msg;
```

### Example 5: Display on Multiple Segments
```javascript
// Use multiple LED Matrix nodes or loop:
const messages = [];

for (let seg = 0; seg < 4; seg++) {
    messages.push({
        command: "text",
        segment: seg,
        payload: `Seg ${seg+1}`,
        color: ["FF0000", "00FF00", "0000FF", "FFFF00"][seg]
    });
}

return [messages];  // Send array of messages
```

### Example 6: Clock Display
```javascript
const now = new Date();
const timeStr = now.toLocaleTimeString('en-US', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: false
});

msg.payload = timeStr;
msg.segment = 0;
msg.color = "00FFFF";  // Cyan
msg.align = "C";
return msg;
```

---

## Commands Reference

### 1. **text** - Display Text
Displays text on a specific segment with full styling options.

**Parameters:**
- `segment` (0-3) - Target segment
- `text` / `payload` - Text to display
- `color` (hex) - Text color (e.g., "FFFFFF" or "#FFFFFF")
- `bgcolor` (hex) - Background color
- `font` - "arial" or "mono"
- `size` - Font size or "auto"
- `align` - "L" (left), "C" (center), "R" (right)
- `effect` - "none", "scroll", "fade"
- `intensity` (0-255) - Brightness for this segment

**Example:**
```javascript
msg = {
    command: "text",
    segment: 0,
    payload: "Hello",
    color: "FF0000",
    bgcolor: "000000",
    font: "arial",
    size: "auto",
    align: "C",
    effect: "none",
    intensity: 255
};
```

---

### 2. **clear** - Clear Segment
Clears a specific segment.

**Parameters:**
- `segment` (0-3) - Segment to clear

**Example:**
```javascript
msg = {
    command: "clear",
    segment: 0
};
```

---

### 3. **clear_all** - Clear All
Clears the entire display (all segments).

**Example:**
```javascript
msg.command = "clear_all";
```

---

### 4. **brightness** - Set Brightness
Sets the global display brightness.

**Parameters:**
- `brightness` / `payload` (0-255) - Brightness level

**Example:**
```javascript
msg = {
    command: "brightness",
    brightness: 128  // 50%
};
```

---

### 5. **layout** - Apply Layout Preset
Applies a predefined layout configuration.

**Parameters:**
- `preset` / `payload` (1-7, 11-14) - Layout preset number

**Presets:**
- **1** - Fullscreen (64×32)
- **2** - Top / Bottom (2 rows)
- **3** - Left / Right (2 columns)
- **4** - Triple Left (left half + 2 right quarters)
- **5** - Triple Right (2 left quarters + right half)
- **6** - Thirds (21px | 21px | 22px columns)
- **7** - Quad View (4 equal quadrants)
- **11** - Fullscreen Segment 1 only
- **12** - Fullscreen Segment 2 only
- **13** - Fullscreen Segment 3 only
- **14** - Fullscreen Segment 4 only

**Example:**
```javascript
msg = {
    command: "layout",
    preset: 7  // Quad view
};
```

---

### 6. **orientation** - Set Orientation
Changes the display orientation.

**Parameters:**
- `orientation` / `payload` - "landscape" (64×32) or "portrait" (32×64)

**Example:**
```javascript
msg = {
    command: "orientation",
    orientation: "portrait"
};
```

---

### 7. **group** - Set Group Assignment
Assigns the display to a group (for multi-panel setups).

**Parameters:**
- `group` / `payload` (0-8) - Group number (0 = all/none)

**Groups:**
- **0** - None (responds to all commands)
- **1** - White
- **2** - Yellow
- **3** - Orange
- **4** - Red
- **5** - Magenta
- **6** - Blue
- **7** - Cyan
- **8** - Green

**Example:**
```javascript
msg = {
    command: "group",
    group: 1
};
```

---

### 8. **config** - Manual Segment Configuration
Manually configure segment geometry.

**Parameters:**
- `segment` (0-3) - Target segment
- `x` - X position (pixels)
- `y` - Y position (pixels)
- `w` - Width (pixels)
- `h` - Height (pixels)

**Example:**
```javascript
msg = {
    command: "config",
    segment: 0,
    x: 0,
    y: 0,
    w: 32,
    h: 32
};
```

---

## Message Properties

### Input Properties (msg)

All node configuration values can be overridden via message properties:

| Property | Type | Description |
|----------|------|-------------|
| `payload` | string/number | Text content (text cmd) or value (brightness/layout) |
| `command` | string | Command type (overrides node config) |
| `ip` | string | Target IP address |
| `port` | number | Target UDP port |
| `segment` | number | Segment number (0-3) |
| `text` | string | Text to display (alternative to payload) |
| `color` | string | Text color (hex) |
| `bgcolor` | string | Background color (hex) |
| `font` | string | Font family |
| `size` | string/number | Font size |
| `align` | string | Text alignment (L/C/R) |
| `effect` | string | Visual effect |
| `intensity` | number | Segment intensity (0-255) |
| `preset` | number | Layout preset (1-7, 11-14) |
| `brightness` | number | Display brightness (0-255) |
| `orientation` | string | Display orientation |
| `group` | number | Group assignment (0-8) |

### Output Properties (msg)

The node passes through the input message and adds:

| Property | Type | Description |
|----------|------|-------------|
| `msg.ledmatrix.command` | object | The JSON command that was sent |
| `msg.ledmatrix.ip` | string | Target IP address |
| `msg.ledmatrix.port` | number | Target UDP port |

---

## Advanced Usage

### Multi-Panel Control

Control multiple panels by overriding IP addresses:

```javascript
const panels = [
    { ip: "10.1.1.24", text: "Panel 1" },
    { ip: "10.1.1.25", text: "Panel 2" },
    { ip: "10.1.1.26", text: "Panel 3" }
];

const messages = panels.map(panel => ({
    ip: panel.ip,
    payload: panel.text,
    color: "00FF00"
}));

return [messages];
```

### Group Routing

Set up group-based routing:

```javascript
// Assign panels to groups
msg.command = "group";
msg.group = 1;  // Assign to group 1

// Later, send to all group 1 panels
// (requires controller to filter by group)
```

### Dynamic Content Updates

Use Node-RED's built-in inject node with interval:

1. Inject node → `repeat: interval` → every 1 second
2. Function node → generate dynamic content
3. LED Matrix node → display

**Example function:**
```javascript
const date = new Date();
msg.payload = date.toLocaleTimeString();
msg.segment = 0;
msg.color = "00FFFF";
return msg;
```

### Status Monitoring

Use the output to track sent commands:

```javascript
// Connect LED Matrix output to a debug node
// or a function node:

if (msg.ledmatrix) {
    node.status({
        fill: "green",
        shape: "dot",
        text: `Sent: ${msg.ledmatrix.command.cmd}`
    });
}
```

---

## Troubleshooting

### Node not appearing in palette
- Check Node-RED logs: `node-red-log` or `~/.node-red/node-red.log`
- Verify installation: `npm list node-red-contrib-led-matrix`
- Restart Node-RED: `node-red-restart`

### Commands not reaching panel
- Check IP address and port configuration
- Verify network connectivity: `ping <panel-ip>`
- Check firewall rules (UDP port must be open)
- View Node-RED debug logs for UDP errors

### Display not updating
- Verify the LED Matrix service is running on the Pi:
  ```bash
  ssh node@<panel-ip>
  sudo systemctl status led-matrix
  ```
- Check panel logs:
  ```bash
  sudo journalctl -u led-matrix -f
  ```

### Colors not displaying correctly
- Remove `#` prefix from hex colors (use "FFFFFF" not "#FFFFFF")
- Ensure 6-character hex format (RGB, not RGBA)

---

## Protocol Details

### UDP JSON Format

The node sends JSON commands over UDP to the LED Matrix controller:

**Text Command:**
```json
{
  "cmd": "text",
  "seg": 0,
  "text": "Hello",
  "color": "FFFFFF",
  "bgcolor": "000000",
  "font": "arial",
  "size": "auto",
  "align": "C",
  "effect": "none",
  "intensity": 255
}
```

**Brightness Command:**
```json
{
  "cmd": "brightness",
  "value": 128
}
```

**Layout Command:**
```json
{
  "cmd": "layout",
  "preset": 7
}
```

**Clear Command:**
```json
{
  "cmd": "clear",
  "seg": 0
}
```

**Clear All Command:**
```json
{
  "cmd": "clear_all"
}
```

---

## Compatibility

✅ **RPi Controller:** QSYS-LED-Matrix (this repository)  
✅ **Q-SYS Plugin:** LEDMatrix_v4.qplug and later  
✅ **Node-RED:** v1.0.0+ (tested on v2.x, v3.x)  
✅ **Node.js:** v14.0.0+  
✅ **Platforms:** All platforms (Linux, macOS, Windows)

---

## Development

### Building from Source

```bash
cd NODERED-LED-Matrix
npm install
npm link  # Symlink to global node_modules
```

### Testing

```bash
# Start Node-RED in safe mode
node-red --safe

# Or with verbose logging
node-red -v
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

---

## License

MIT License - See LICENSE file in repository root

---

## Links

- **GitHub:** https://github.com/DHPKE/QSYS-LED-Matrix
- **Node-RED:** https://nodered.org
- **Q-SYS:** Compatible with Q-SYS plugin version 4.3.0+

---

## Support

For issues, questions, or contributions:
- Open an issue on GitHub
- Check existing documentation in the repository
- Review Q-SYS plugin documentation for protocol details

---

**Happy building! 🎨🚀**
