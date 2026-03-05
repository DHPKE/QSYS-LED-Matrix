# C4M LED Matrix Proxy - MQTT Edition

**ID-based control with MQTT messaging** for ListenVision C4M LED controllers.

## Why MQTT?

✅ **ID-based addressing** - No IP addresses in Q-SYS, just device IDs  
✅ **Publish/Subscribe** - Clean decoupling between Q-SYS and displays  
✅ **Group control** - Target individual displays, groups, or broadcast to all  
✅ **Status feedback** - Real-time device health and status updates  
✅ **Scalable** - Handle 100+ displays easily  
✅ **Standard protocol** - Works with any MQTT client  

## Architecture

```
┌──────────────┐                  ┌────────────────┐                ┌──────────────┐
│   Q-SYS      │  MQTT Publish    │  Node.js Proxy │  C4M SDK       │  C4M Cards   │
│   Plugin     ├─────────────────>│  + Broker      ├───────────────>│  (40+)       │
│              │  "led/display/   │                │  (TCP)          │              │
│              │   42/cmd/segment"│  - Renders     │                 │  HUB75 LED   │
└──────────────┘                  │  - Routes      │                 └──────────────┘
                                  │  - Sends       │
                                  └────────┬───────┘
                                           │ MQTT Subscribe
                                           v
                                  Status & Health
                                  (back to Q-SYS)
```

## MQTT Topic Structure

### Command Topics (Q-SYS → Proxy)

```
led/display/{id}/cmd/segment       - Set segment text/properties
led/display/{id}/cmd/layout        - Set layout preset  
led/display/{id}/cmd/brightness    - Set brightness
led/display/{id}/cmd/rotation      - Set rotation
led/display/{id}/cmd/clear         - Clear display
led/display/{id}/cmd/test          - Test pattern

led/display/all/cmd/*              - Broadcast to ALL displays
led/group/{group}/cmd/*            - Send to specific group

led/proxy/cmd/reload               - Reload configuration
led/proxy/cmd/stats                - Request statistics
```

### Status Topics (Proxy → Q-SYS)

```
led/display/{id}/status            - Device status updates
led/display/{id}/error             - Error messages
led/proxy/status                   - Proxy server status
led/proxy/stats                    - Performance statistics
led/proxy/devices                  - Device list
```

## Message Examples

### Set Segment Text on Display ID 42

**Topic:** `led/display/42/cmd/segment`  
**Payload:**
```json
{
  "seg": 0,
  "enabled": true,
  "text": "Hello World",
  "color": "FF0000",
  "effect": 1,
  "align": 1
}
```

### Broadcast to All Displays

**Topic:** `led/display/all/cmd/segment`  
**Payload:**
```json
{
  "seg": 0,
  "text": "EMERGENCY EXIT",
  "color": "FF0000"
}
```

### Send to Group 1 (e.g., "Hall A")

**Topic:** `led/group/1/cmd/segment`  
**Payload:**
```json
{
  "seg": 0,
  "text": "Hall A - Session Starting",
  "color": "00FF00"
}
```

### Clear Display

**Topic:** `led/display/5/cmd/clear`  
**Payload:** `{}`

### Test Pattern

**Topic:** `led/display/all/cmd/test`  
**Payload:** `{}`

## Configuration

### config.json

```json
{
  "mqtt": {
    "broker": "mqtt://localhost:1883",
    "username": "",
    "password": "",
    "clientId": "c4m-proxy",
    "topicPrefix": "led"
  },
  "devices": [
    { "id": 1, "ip": "10.1.1.101", "group": 0, "name": "Main-Entrance", "enabled": true },
    { "id": 2, "ip": "10.1.1.102", "group": 0, "name": "Lobby-Left", "enabled": true },
    { "id": 42, "ip": "10.1.1.142", "group": 1, "name": "Hall-A-Main", "enabled": true }
  ],
  "groups": {
    "0": { "name": "All Displays", "description": "Broadcast" },
    "1": { "name": "Hall A", "description": "Main auditorium" },
    "2": { "name": "Hall B", "description": "Secondary auditorium" }
  }
}
```

**Key Points:**
- **Device IDs** can be any number (1-255, or higher)
- **IP addresses** only in config file, never exposed to Q-SYS
- **Groups** allow zone-based control
- **enabled: false** to temporarily disable a device

## Installation

### 1. Install MQTT Broker (Mosquitto)

```bash
# Ubuntu/Debian
sudo apt-get install mosquitto mosquitto-clients

# macOS
brew install mosquitto
brew services start mosquitto

# Docker
docker run -d -p 1883:1883 eclipse-mosquitto
```

### 2. Install Proxy

```bash
cd ListenVision/c4m-proxy-mqtt
npm install
```

### 3. Configure Devices

```bash
cp config.example.json config.json
# Edit config.json with your 40 device IDs and IPs
```

### 4. Start Proxy

```bash
node server.js
```

## Q-SYS Integration

### MQTT Client Plugin

You'll need the **Q-SYS MQTT Client plugin** (available on Asset Manager).

**Plugin Configuration:**
- Broker: `mqtt://10.1.1.100:1883` (your MQTT broker IP)
- Client ID: `qsys-led-controller`
- No username/password (or set if using auth)

### Control Script Example

```lua
-- Q-SYS control script to send LED commands

function SetDisplayText(displayId, segmentId, text, color)
  local topic = string.format("led/display/%d/cmd/segment", displayId)
  
  local payload = {
    seg = segmentId,
    enabled = true,
    text = text,
    color = color or "FF0000",
    effect = 1,  -- scroll left
    align = 1    -- center
  }
  
  -- Publish via MQTT client plugin
  Controls.MQTT.String = rapidjson.encode(payload)
  Controls.MQTT_Topic.String = topic
  Controls.MQTT_Publish.Boolean = true
end

-- Example usage:
SetDisplayText(42, 0, "Welcome!", "00FF00")        -- Display 42, segment 0
SetDisplayText(1, 0, "Lobby", "0000FF")            -- Display 1, segment 0

-- Broadcast to all:
function BroadcastText(text)
  local topic = "led/display/all/cmd/segment"
  local payload = { seg = 0, text = text, color = "FF0000" }
  -- ... publish as above
end

-- Group control:
function SetGroupText(groupId, text)
  local topic = string.format("led/group/%d/cmd/segment", groupId)
  -- ... publish as above
end
```

### Button Control Example

```lua
-- Button handler for "Emergency" button
Controls.EmergencyButton.EventHandler = function()
  local topic = "led/display/all/cmd/segment"
  local payload = {
    seg = 0,
    text = "EMERGENCY EXIT",
    color = "FF0000",
    effect = 5,  -- blink
    align = 1
  }
  
  Controls.MQTT.String = rapidjson.encode(payload)
  Controls.MQTT_Topic.String = topic
  Controls.MQTT_Publish.Boolean = true
end
```

## Testing

### Test from Command Line

```bash
# Install mosquitto-clients
sudo apt-get install mosquitto-clients

# Set text on display 42
mosquitto_pub -h localhost -t "led/display/42/cmd/segment" \
  -m '{"seg":0,"text":"Hello","color":"FF0000"}'

# Broadcast test pattern
mosquitto_pub -h localhost -t "led/display/all/cmd/test" -m '{}'

# Clear display 5
mosquitto_pub -h localhost -t "led/display/5/cmd/clear" -m '{}'

# Subscribe to status updates
mosquitto_sub -h localhost -t "led/display/+/status"
```

### Test with Node.js Client

```bash
node test-client.js              # Standard test sequence
node test-client.js stress 30 10 42  # Stress test display 42
```

## Device ID Mapping

| Display Location | Device ID | IP Address | Group |
|------------------|-----------|------------|-------|
| Main Entrance | 1 | 10.1.1.101 | 0 (All) |
| Lobby Left | 2 | 10.1.1.102 | 0 (All) |
| Hall A - Row 1 | 10 | 10.1.1.110 | 1 (Hall A) |
| Hall A - Row 2 | 11 | 10.1.1.111 | 1 (Hall A) |
| Hall B - Row 1 | 20 | 10.1.1.120 | 2 (Hall B) |

**Pro Tip:** Use meaningful ID numbers:
- 1-9: Lobby/entrance
- 10-19: Hall A
- 20-29: Hall B
- 30-39: Meeting rooms
- etc.

## Advantages Over UDP

### vs. UDP/IP-based Control

| Feature | MQTT (This Solution) | UDP (Old RPi Method) |
|---------|----------------------|----------------------|
| **Addressing** | Device ID | IP address |
| **Q-SYS Config** | Just IDs | Must know all IPs |
| **Broadcasting** | Built-in (`/all`) | Must send to each IP |
| **Group Control** | Native (`/group/{n}`) | Manual filtering |
| **Status Feedback** | Yes (subscribe) | No |
| **Device Discovery** | Via proxy | Manual |
| **Network Changes** | Transparent | Must reconfigure Q-SYS |
| **Scalability** | Excellent (100+) | Poor (IP management) |

## Status Feedback

Subscribe to status topics in Q-SYS to get real-time updates:

### Device Status

**Topic:** `led/display/+/status`  
**Payload:**
```json
{
  "id": 42,
  "status": "updated",
  "timestamp": 1709673600000,
  "health": {
    "online": true,
    "lastUpdate": 1709673600000,
    "updateCount": 1523,
    "errorCount": 2
  },
  "segments": 2
}
```

### Proxy Stats

**Topic:** `led/proxy/stats`  
**Payload:**
```json
{
  "messagesReceived": 15234,
  "commandsProcessed": 15180,
  "updatesSent": 60720,
  "errors": 12,
  "uptime": 86400,
  "updateRate": "10.23"
}
```

## Web Monitoring

Open browser: `http://<proxy-ip>:8080`

Shows:
- MQTT broker connection status
- Real-time statistics
- Device list with health status
- MQTT topic examples
- Live device updates

## Production Deployment

### Systemd Service

Create `/etc/systemd/system/c4m-proxy-mqtt.service`:

```ini
[Unit]
Description=C4M LED Matrix Proxy (MQTT)
After=network.target mosquitto.service

[Service]
Type=simple
User=pi
WorkingDirectory=/opt/c4m-proxy-mqtt
ExecStart=/usr/bin/node server.js
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable c4m-proxy-mqtt
sudo systemctl start c4m-proxy-mqtt
sudo systemctl status c4m-proxy-mqtt
```

### MQTT Broker Security

**Enable authentication:**

```bash
# Create password file
sudo mosquitto_passwd -c /etc/mosquitto/passwd qsys

# Configure mosquitto (/etc/mosquitto/mosquitto.conf)
allow_anonymous false
password_file /etc/mosquitto/passwd

# Restart
sudo systemctl restart mosquitto
```

Update proxy `config.json`:
```json
{
  "mqtt": {
    "broker": "mqtt://localhost:1883",
    "username": "qsys",
    "password": "your-password"
  }
}
```

### TLS/SSL (Optional)

For secure MQTT:
1. Generate certificates
2. Configure Mosquitto for TLS
3. Update broker URL: `mqtts://...`

## Performance

### Bandwidth

MQTT adds minimal overhead (~50 bytes per message):

- **Without MQTT**: 6 KB per update
- **With MQTT**: ~6.05 KB per update

**40 devices @ 10Hz:**
- MQTT traffic: ~20 Mbps (same as UDP solution)
- Broker overhead: Negligible on LAN

### Latency

- **MQTT publish**: <5ms on LAN
- **Proxy processing**: 10-20ms
- **C4M SDK send**: 50-100ms
- **Total**: 60-125ms per device

### Scalability

MQTT broker can handle:
- **1000+ clients**
- **10,000+ messages/second**
- **Much better** than point-to-point UDP

## Troubleshooting

### MQTT Broker Not Running

```bash
sudo systemctl status mosquitto
sudo systemctl start mosquitto
```

### Connection Refused

Check broker IP in config.json matches your MQTT broker.

### No Status Updates in Q-SYS

Ensure Q-SYS MQTT plugin is subscribed to status topics.

### Device Not Updating

Check:
1. Device enabled in config.json
2. IP address correct
3. Network connectivity to C4M
4. Web UI shows device health

## Files

- `server.js` - MQTT proxy server
- `render-engine.js` - Canvas rendering (same as UDP version)
- `test-client.js` - MQTT test utility
- `config.json` - Device and broker configuration
- `README.md` - This file

## Next Steps

1. Install Mosquitto MQTT broker
2. Configure devices in `config.json` (add all 40)
3. Start proxy: `node server.js`
4. Test with `node test-client.js`
5. Integrate Q-SYS MQTT plugin
6. Deploy to production

---

**MQTT = Much Better for Large Installations! 🎉**
