# Q-SYS Plugin for ListenVision C4M (MQTT)

**File:** `LEDMatrix_LV-v1.qplug`

## Overview

This Q-SYS plugin controls ListenVision C4M LED controller cards via MQTT, using the `c4m-proxy-mqtt` Node.js server as a bridge.

## Architecture

```
Q-SYS Plugin → MQTT Broker → c4m-proxy-mqtt → C4M SDK → C4M Card → LED Display
```

## Installation

1. **Start MQTT Proxy Server**
   ```bash
   cd ListenVision/c4m-proxy-mqtt
   node server.js
   ```

2. **Import Plugin to Q-SYS**
   - Open Q-SYS Designer
   - Tools → Show Design Resources
   - Plugins → Add/Import
   - Select `LEDMatrix_LV-v1.qplug`

3. **Add Plugin to Design**
   - Drag plugin from Plugins list onto schematic
   - Configure properties (see below)

## Plugin Properties

| Property | Description | Example |
|----------|-------------|---------|
| **MQTT Broker** | IP/hostname of MQTT broker | `localhost` or `10.1.1.100` |
| **MQTT Port** | MQTT broker port | `1883` |
| **MQTT Username** | Optional authentication | _(leave empty)_ |
| **MQTT Password** | Optional authentication | _(leave empty)_ |
| **Device ID** | C4M device ID (1-255) | `42` |
| **Number of Segments** | Text segments (1-4) | `4` |

**Example Configuration:**
- Broker: `10.1.1.100` (proxy server IP)
- Port: `1883`
- Device ID: `42` (matches device in proxy config)

## Controls

### Connection
- **MQTT Broker** - Broker hostname/IP
- **MQTT Port** - Broker port (1883)
- **Device ID** - Target display ID
- **Connect** - Reconnect to broker
- **Status** - Connection indicator (green/red)
- **Last Command** - Shows last MQTT message sent

### Target Selection
Select where commands should go:
- **All** - Broadcast to ALL displays
- **1-8** - Send to specific group

### Segments (1-4)
For each segment:
- **Text** - Message to display
- **Color** - Text color (Red, Green, Blue, White, etc.)
- **Align** - Left, Center, Right
- **Effect** - Static, Scroll, Blink
- **Display** - Send text to display
- **Clear** - Clear this segment
- **Active LED** - Shows if segment has content

### Global Controls
- **Brightness** - 0-255 (128 = default)
- **Rotation** - 0°, 90°, 180°, 270°
- **Test** - Send test pattern
- **Clear All** - Clear all segments

### Layout Presets
- **1 - Fullscreen** - One large segment
- **2 - Top / Bottom** - Two horizontal segments
- **3 - Left / Right** - Two vertical segments
- **4 - Triple** - Three segments
- **5 - Quad View** - Four segments

## MQTT Topics Used

The plugin publishes to these topics:

### Individual Device
```
led/display/42/cmd/segment      - Set text on display ID 42
led/display/42/cmd/clear        - Clear display 42
led/display/42/cmd/brightness   - Set brightness
led/display/42/cmd/rotation     - Set rotation
led/display/42/cmd/layout       - Apply layout preset
led/display/42/cmd/test         - Test pattern
```

### Broadcast (when "All" selected)
```
led/display/all/cmd/segment     - Broadcast to all displays
led/display/all/cmd/clear       - Clear all displays
```

### Group (when group 1-8 selected)
```
led/group/1/cmd/segment         - Send to group 1
led/group/3/cmd/clear           - Clear group 3
```

### Status (subscribed)
```
led/display/42/status           - Receive status from display 42
```

## Message Format

### Set Segment Text
```json
{
  "seg": 0,
  "enabled": true,
  "text": "Hello World",
  "color": "FF0000",
  "align": 1,
  "effect": 0
}
```

### Clear Segment
```json
{
  "seg": 0
}
```

### Brightness
```json
{
  "value": 200
}
```

### Test Pattern
```json
{}
```

## Example Use Cases

### Single Display Control
1. Set **Device ID** to `42`
2. Select **All** (actually sends to device ID from property)
3. Type text in **Seg1** → Click **Display**

### Broadcast to All Displays
1. Select **All** target
2. Commands go to: `led/display/all/cmd/*`
3. All displays receive the command

### Group Control (e.g., "Hall A")
1. Configure displays in proxy config with `"group": 1`
2. Select **Group 1** target
3. Commands go to: `led/group/1/cmd/*`
4. Only group 1 displays receive

## Lua Scripting Examples

### Send Text to Display 42
```lua
-- This happens automatically when you click "Display" button
-- But you can also trigger programmatically:
Controls.seg1_text.String = "Welcome!"
Controls.seg1_send:Trigger()
```

### Broadcast Message
```lua
-- Select "All" target
Controls.group0_select:Trigger()

-- Set text
Controls.seg1_text.String = "EMERGENCY EXIT"
Controls.seg1_send:Trigger()
```

### Group Message
```lua
-- Select group 1
Controls.group1_select:Trigger()

-- Set text
Controls.seg1_text.String = "Hall A - Session Starting"
Controls.seg1_send:Trigger()
```

### Change Brightness
```lua
Controls.brightness.String = "200"  -- Triggers EventHandler automatically
```

### Clear All Segments
```lua
Controls.clear_all:Trigger()
```

## Differences from RPi UDP Plugin

| Feature | RPi UDP Plugin | MQTT Plugin |
|---------|---------------|-------------|
| **Connection** | UDP socket to IP | MQTT broker connection |
| **Addressing** | IP address per panel | Device ID (1-255) |
| **Config** | IP in plugin property | ID in plugin, IP in proxy |
| **Broadcasting** | Multiple UDP sends | Single MQTT publish |
| **Status** | No feedback | Subscribe to status topic |
| **Group Control** | UDP payload field | MQTT topic (led/group/N) |
| **Reboot** | HTTP to panel | Not available (proxy manages) |

## Troubleshooting

### "Not connected" status
- ✅ Check MQTT broker is running: `mosquitto`
- ✅ Check broker IP/port in plugin properties
- ✅ Check network connectivity
- ✅ Click **Connect** button

### Commands not working
- ✅ Verify proxy server is running: `node server.js`
- ✅ Check proxy web UI: `http://<proxy-ip>:8080`
- ✅ Verify device ID matches proxy config.json
- ✅ Check "Last Command" field for errors

### Display not updating
- ✅ Test from proxy web UI first
- ✅ Check device health in proxy web UI
- ✅ Verify C4M card has correct IP
- ✅ Check C4M SDK library is installed

### Wrong display responds
- ✅ Check Device ID in plugin properties
- ✅ Verify ID matches proxy config.json mapping
- ✅ Test with proxy test client first

## Proxy Server Setup

Before using this plugin, ensure proxy server is configured:

```bash
cd c4m-proxy-mqtt

# Install dependencies
npm install

# Configure devices
cp config.example.json config.json
nano config.json  # Add your devices

# Start server
node server.js

# Open web UI
open http://localhost:8080
```

## MQTT Broker Setup

Install Mosquitto MQTT broker:

```bash
# Ubuntu/Debian
sudo apt-get install mosquitto

# macOS
brew install mosquitto
brew services start mosquitto

# Docker
docker run -d -p 1883:1883 eclipse-mosquitto
```

## Multiple Displays

To control multiple displays from one Q-SYS design:

1. **Option A:** Multiple plugin instances (one per display)
   - Add plugin, set Device ID = 1
   - Add plugin, set Device ID = 2
   - etc.

2. **Option B:** Single plugin, change target
   - Change **Device ID** property as needed
   - Or use **Group** buttons to target zones

3. **Option C:** Lua scripting
   ```lua
   -- Function to update any display
   function UpdateDisplay(deviceId, text)
     -- Change device ID
     -- (Note: this requires modifying plugin property at runtime)
     -- Better to use Option A or Option B
   end
   ```

## Performance

### Message Rate
- Q-SYS can send ~10-20 MQTT messages/second
- Proxy handles batching and rate limiting
- C4M cards update at ~10Hz

### Latency
- MQTT publish: <5ms
- Proxy processing: 10-20ms
- C4M SDK send: 50-100ms
- **Total: 60-125ms** (acceptable for signage)

## Support

- **Proxy Web UI:** `http://<proxy-ip>:8080` - Device management and monitoring
- **Proxy Logs:** Check console output of `node server.js`
- **MQTT Monitor:** Use `mosquitto_sub -t "led/#"` to see all messages
- **Q-SYS Debug:** Enable plugin debug window (Tools → Logging)

## Files

- **LEDMatrix_LV-v1.qplug** - This Q-SYS plugin
- **c4m-proxy-mqtt/** - Node.js MQTT proxy server
- **c4m-proxy-mqtt/README.md** - Proxy documentation
- **c4m-proxy-mqtt/QUICKSTART.md** - 5-minute setup guide

## Repository

https://github.com/DHPKE/QSYS-LED-Matrix/tree/main/ListenVision

---

**For 40+ displays, use this MQTT plugin + c4m-proxy-mqtt server!** 🚀
