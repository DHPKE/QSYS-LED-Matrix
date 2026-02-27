# Node-RED LED Matrix - Quick Reference

## Installation
```bash
cd ~/.node-red
npm install /path/to/QSYS-LED-Matrix/NODERED-LED-Matrix
node-red-restart
```

## Node Configuration
- **Category:** Output
- **IP Address:** 10.1.1.24 (your panel IP)
- **Port:** 21324
- **Command:** Select from dropdown

## Commands Quick Reference

### Text
```javascript
msg.payload = "Hello";
msg.segment = 0;        // 0-3
msg.color = "00FF00";   // Green
msg.bgcolor = "000000"; // Black
msg.align = "C";        // L/C/R
```

### Brightness
```javascript
msg.command = "brightness";
msg.brightness = 128;   // 0-255 (50%)
```

### Layout
```javascript
msg.command = "layout";
msg.preset = 7;         // Quad view
```

### Clear
```javascript
msg.command = "clear_all";
```

## Layout Presets
| # | Layout |
|---|--------|
| 1 | Fullscreen |
| 2 | Top/Bottom |
| 3 | Left/Right |
| 4 | Triple Left |
| 5 | Triple Right |
| 6 | Thirds |
| 7 | Quad View |
| 11-14 | Single Segment Fullscreen |

## Color Reference
| Color | Hex |
|-------|-----|
| White | FFFFFF |
| Red | FF0000 |
| Green | 00FF00 |
| Blue | 0000FF |
| Yellow | FFFF00 |
| Cyan | 00FFFF |
| Magenta | FF00FF |
| Orange | FF8800 |
| Black | 000000 |

## Common Patterns

### Clock (every 1 sec)
```javascript
msg.payload = new Date().toLocaleTimeString();
msg.color = "00FFFF";
```

### Multi-Segment
```javascript
const msgs = [];
for (let i = 0; i < 4; i++) {
    msgs.push({
        segment: i,
        payload: `Seg ${i+1}`,
        color: ["FF0000","00FF00","0000FF","FFFF00"][i]
    });
}
return [msgs];
```

### Override IP
```javascript
msg.ip = "10.1.1.25";   // Different panel
msg.payload = "Panel 2";
```

## Troubleshooting
- **No display?** Check IP/port, ping panel
- **Not in palette?** Restart Node-RED
- **UDP errors?** Check firewall, verify service running

## More Info
- **Full docs:** README.md
- **Install guide:** INSTALL.md
- **Examples:** Import examples.json
