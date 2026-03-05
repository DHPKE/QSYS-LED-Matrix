# ListenVision C4M LED Controller Integration

This directory contains Q-SYS plugins and proxy servers for controlling ListenVision C4M LED controller cards.

## Contents

### 📱 Q-SYS Plugins

- **LVLEDController_v01.qplug** - Original plugin version
- **LVLEDController_v02.qplug** - Latest plugin (v2.0.2)
  - 4 text areas with 16+ scroll effects
  - Digital clock and countdown timer
  - Frame and background colors
  - 8 groups + broadcast routing
  - Display rotation support
  - Flash/RAM storage selection

### 🖥️ Proxy Servers

Two Node.js proxy implementations for large-scale deployments:

#### **c4m-proxy/** - UDP-Based Proxy
Drop-in replacement for RPi controllers using UDP JSON protocol.

**Use when:**
- Migrating from RPi LED Matrix setup
- Want minimal Q-SYS plugin changes
- Need direct IP addressing

**Features:**
- Compatible with existing RPi UDP protocol
- 40+ device support with group control
- Server-side Canvas rendering
- Web monitoring UI

[→ UDP Proxy Documentation](c4m-proxy/README.md)

#### **c4m-proxy-mqtt/** - MQTT-Based Proxy ⭐ **RECOMMENDED**
Modern MQTT architecture with ID-based addressing.

**Use when:**
- Starting new deployment
- Need 40+ displays with easy management
- Want status feedback and health monitoring
- Prefer pub/sub over point-to-point

**Features:**
- ID-based addressing (no IPs in Q-SYS!)
- Native broadcast and group control via MQTT topics
- Real-time device status and health reporting
- Better scalability (100+ devices)
- Network-change resilient

[→ MQTT Proxy Documentation](c4m-proxy-mqtt/README.md)

## Quick Comparison

| Feature | Q-SYS Plugin | UDP Proxy | MQTT Proxy |
|---------|--------------|-----------|------------|
| **Best for** | 1-5 displays | 10-20 displays | 40+ displays |
| **Addressing** | IP per device | IP per device | Device ID |
| **Q-SYS knows** | All IPs | All IPs | Just IDs |
| **Broadcasting** | Manual | Via group param | MQTT topic |
| **Status feedback** | No | No | Yes ✓ |
| **Scalability** | Limited | Good | Excellent |
| **IP changes** | Reconfigure | Reconfigure | Transparent |

## Use Case Guide

### Small Installation (1-5 Displays)
**→ Use Q-SYS Plugin directly**
- LVLEDController_v02.qplug
- Simple, no extra infrastructure
- Direct TCP/UDP to C4M cards

### Medium Installation (5-20 Displays)
**→ Use UDP Proxy**
- Reuse existing RPi Q-SYS logic
- Add server-side rendering
- Group control capability

### Large Installation (20-100+ Displays)
**→ Use MQTT Proxy** ⭐
- ID-based device management
- Zone/group control via MQTT topics
- Real-time health monitoring
- Easy to scale and maintain

## Architecture

### Direct Plugin Control
```
Q-SYS Plugin → TCP/UDP → C4M Card → HUB75 LED
```

### Proxy-Based Control
```
Q-SYS Plugin → UDP/MQTT → Node.js Proxy → C4M SDK → 40+ C4M Cards → HUB75 LEDs
                                    ↓
                              Canvas Rendering
                              Text → Bitmap
```

## Why Use a Proxy?

The C4M SDK requires:
1. **Local rendering** - Text must be rendered to bitmaps before sending
2. **Program compilation** - Content packaged into C4M program format  
3. **SDK library** - Native C library (`.so`/`.dll`)

Q-SYS plugins (Lua) cannot easily:
- Load native C libraries
- Render text with arbitrary fonts/sizes
- Generate bitmaps

**Solution:** Proxy server handles rendering and SDK calls, Q-SYS just sends simple commands.

## Getting Started

### For Small Deployments
1. Open Q-SYS Designer
2. Drag LVLEDController_v02.qplug into design
3. Configure C4M IP addresses
4. Done!

### For Large Deployments (MQTT)
1. Install Mosquitto MQTT broker
2. Install proxy: `cd c4m-proxy-mqtt && npm install`
3. Configure 40 devices in `config.json`
4. Start proxy: `node server.js`
5. Add Q-SYS MQTT plugin
6. Send commands via MQTT topics

[→ MQTT Quick Start Guide](c4m-proxy-mqtt/QUICKSTART.md)

## MQTT Topic Structure

**Commands (Q-SYS → Proxy):**
```
led/display/42/cmd/segment      - Display ID 42
led/display/all/cmd/segment     - Broadcast to all
led/group/1/cmd/segment         - Group 1 (e.g., "Hall A")
```

**Status (Proxy → Q-SYS):**
```
led/display/42/status           - Device health/status
led/proxy/stats                 - Performance metrics
```

## Example Q-SYS Code (MQTT)

```lua
-- Set text on display 42 (no IP address needed!)
function UpdateDisplay(id, text, color)
  local topic = string.format("led/display/%d/cmd/segment", id)
  local payload = {
    seg = 0,
    text = text,
    color = color or "FF0000"
  }
  
  Controls.MQTT_Topic.String = topic
  Controls.MQTT_Payload.String = rapidjson.encode(payload)
  Controls.MQTT_Publish.Boolean = true
end

-- Usage:
UpdateDisplay(42, "Welcome!", "00FF00")   -- Display 42
UpdateDisplay(1, "Lobby", "0000FF")       -- Display 1

-- Broadcast to all 40 displays:
function BroadcastMessage(text)
  local topic = "led/display/all/cmd/segment"
  local payload = { seg = 0, text = text, color = "FF0000" }
  -- ... publish
end

BroadcastMessage("EMERGENCY EXIT")
```

## SDK Integration

Both proxy servers include C4M SDK wrapper code but need:

1. **SDK Library File:**
   ```bash
   mkdir lib
   cp /path/to/SDK/libsledplayer7.so lib/    # Linux
   cp /path/to/SDK/ledplayer7.dll lib/       # Windows
   ```

2. **FFI Dependencies:**
   ```bash
   npm install ffi-napi ref-napi
   ```

3. **Enable SDK calls** in server.js (uncomment SDK code blocks)

## Performance

### Bandwidth (40 devices @ 10Hz)

| Solution | Bandwidth | Latency | Scalability |
|----------|-----------|---------|-------------|
| **Direct Plugin** | N/A (1 device) | 20-50ms | Poor |
| **UDP Proxy** | ~20 Mbps | 60-125ms | Good |
| **MQTT Proxy** | ~20 Mbps | 60-125ms | Excellent |

### Update Rate
- **Target**: 10 Hz (10 updates/second)
- **Achievable**: 5-10 Hz depending on content
- **Recommendation**: Use RAM mode (saveType=3) on C4M cards

## Documentation

- **UDP Proxy**:
  - [README.md](c4m-proxy/README.md) - Full documentation
  - [QUICKSTART.md](c4m-proxy/QUICKSTART.md) - 5-minute setup
  - [IMPLEMENTATION.md](c4m-proxy/IMPLEMENTATION.md) - Architecture details

- **MQTT Proxy**:
  - [README.md](c4m-proxy-mqtt/README.md) - Full documentation
  - [QUICKSTART.md](c4m-proxy-mqtt/QUICKSTART.md) - 5-minute setup

## Testing

### UDP Proxy
```bash
cd c4m-proxy
npm install
node test-client.js                    # Standard tests
node test-client.js stress 30 10       # Stress test
```

### MQTT Proxy
```bash
cd c4m-proxy-mqtt
npm install

# Start proxy
node server.js

# Test (in another terminal)
node test-client.js                    # Standard tests
node test-client.js stress 30 10 42    # Stress test display 42

# Or use mosquitto_pub
mosquitto_pub -t "led/display/1/cmd/test" -m '{}'
```

## Repository

**GitHub:** https://github.com/DHPKE/QSYS-LED-Matrix/tree/main/ListenVision

## Support

- Check Web UI at `http://<proxy-ip>:8080` for monitoring
- Review server logs for errors
- Test with single device before scaling to 40+
- See individual README files for detailed troubleshooting

## License

MIT

---

**For large installations (40+ displays), use the MQTT proxy!** 🚀
