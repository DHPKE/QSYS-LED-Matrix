# MQTT Proxy - Quick Start

## 5-Minute Setup

### 1. Install MQTT Broker

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```

**macOS:**
```bash
brew install mosquitto
brew services start mosquitto
```

**Docker:**
```bash
docker run -d --name mosquitto -p 1883:1883 eclipse-mosquitto
```

### 2. Install Proxy

```bash
cd ListenVision/c4m-proxy-mqtt
npm install
```

### 3. Configure

```bash
cp config.example.json config.json
```

Edit `config.json` - add your 40 devices with IDs and IPs:

```json
{
  "mqtt": {
    "broker": "mqtt://localhost:1883"
  },
  "devices": [
    { "id": 1, "ip": "10.1.1.101", "group": 0, "name": "LED-01", "enabled": true },
    { "id": 2, "ip": "10.1.1.102", "group": 0, "name": "LED-02", "enabled": true },
    ...add 38 more...
  ]
}
```

### 4. Start Proxy

```bash
node server.js
```

Should see:
```
✓ Connected to MQTT broker: mqtt://localhost:1883
✓ Subscribed to MQTT topics
✓ Web UI available at http://localhost:8080
✓ Server ready
```

### 5. Test It

**Terminal 1** (proxy running):
```bash
node server.js
```

**Terminal 2** (test commands):
```bash
# Test pattern on display 1
mosquitto_pub -h localhost -t "led/display/1/cmd/test" -m '{}'

# Set text on display 1
mosquitto_pub -h localhost -t "led/display/1/cmd/segment" \
  -m '{"seg":0,"text":"Hello MQTT","color":"FF0000"}'

# Broadcast to all
mosquitto_pub -h localhost -t "led/display/all/cmd/segment" \
  -m '{"seg":0,"text":"BROADCAST","color":"00FF00"}'
```

**Or use Node.js test client:**
```bash
node test-client.js
```

### 6. Monitor

Open browser: `http://localhost:8080`

## Q-SYS Integration

### Install Q-SYS MQTT Plugin

1. Open Q-SYS Designer
2. Add "MQTT Client" plugin from Asset Manager
3. Configure broker: `mqtt://10.1.1.100:1883` (your broker IP)

### Send Commands from Q-SYS

```lua
-- Set text on display 42
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
UpdateDisplay(42, "Welcome!", "00FF00")
UpdateDisplay(1, "Lobby", "0000FF")

-- Broadcast:
function Broadcast(text)
  UpdateDisplay("all", text, "FF0000")
end
```

## MQTT Topic Structure

**Commands (Q-SYS → Proxy):**
```
led/display/42/cmd/segment      - Set text on display 42
led/display/all/cmd/segment     - Broadcast to all
led/group/1/cmd/segment         - Send to group 1
```

**Status (Proxy → Q-SYS):**
```
led/display/+/status            - Device status
led/proxy/stats                 - Proxy statistics
```

## Message Format

### Set Segment Text
```json
{
  "seg": 0,                  // Segment ID (0-3)
  "enabled": true,
  "text": "Hello World",
  "color": "FF0000",         // RGB hex
  "effect": 1,               // 0=static, 1=scroll
  "align": 1                 // 0=left, 1=center, 2=right
}
```

### Clear Display
```json
{}  // Empty payload to led/display/{id}/cmd/clear
```

### Test Pattern
```json
{}  // Empty payload to led/display/{id}/cmd/test
```

## Device ID Strategy

Use meaningful IDs for easy management:

| Range | Location | Example |
|-------|----------|---------|
| 1-9 | Lobby/Entrance | 1: Main entrance |
| 10-19 | Hall A | 10-19: Row displays |
| 20-29 | Hall B | 20-29: Row displays |
| 30-39 | Meeting Rooms | 30-35: Room signs |
| 40-49 | Backstage | 40-45: Crew monitors |

**In Q-SYS, you only reference IDs - no IP addresses!**

## Troubleshooting

**"Connection refused"**
→ Check MQTT broker is running: `sudo systemctl status mosquitto`

**"No devices found"**
→ Verify config.json has devices with `"enabled": true`

**"Device not updating"**
→ Check device IP, network connectivity, web UI for errors

**"Q-SYS not receiving status"**
→ Ensure MQTT plugin is subscribed to `led/display/+/status`

## Testing Commands

### Command Line (mosquitto_pub)

```bash
# Set text on display 1
mosquitto_pub -t "led/display/1/cmd/segment" \
  -m '{"seg":0,"text":"Test","color":"FF0000"}'

# Broadcast
mosquitto_pub -t "led/display/all/cmd/clear" -m '{}'

# Group control
mosquitto_pub -t "led/group/1/cmd/segment" \
  -m '{"seg":0,"text":"Group 1"}'

# Subscribe to status
mosquitto_sub -t "led/display/+/status"
```

### Node.js Test Client

```bash
node test-client.js              # Run all tests
node test-client.js stress 30 10 # Stress test 30s @ 10Hz
```

## What's Different from UDP Version?

| Feature | UDP Version | MQTT Version |
|---------|-------------|--------------|
| **Addressing** | IP address | Device ID |
| **Q-SYS knows** | All device IPs | Just device IDs |
| **Broadcast** | Send to each IP | Single topic publish |
| **Groups** | Manual logic | Native MQTT topics |
| **Status** | No feedback | Real-time status |
| **Scalability** | 10-20 devices | 100+ devices |
| **IP changes** | Reconfigure Q-SYS | Just update proxy config |

## Next Steps

1. ✅ MQTT broker installed and running
2. ✅ Proxy configured with 40 devices
3. ✅ Tested with mosquitto_pub or test-client.js
4. → Integrate Q-SYS MQTT plugin
5. → Create Q-SYS control logic
6. → Deploy to production

## Files Location

```
ListenVision/c4m-proxy-mqtt/
├── server.js           - Main proxy server
├── render-engine.js    - Canvas rendering
├── test-client.js      - Test utility
├── config.json         - Your config (create from example)
├── package.json        - Dependencies
├── README.md           - Full docs
└── QUICKSTART.md       - This file
```

---

**MQTT makes 40+ device control EASY! 🚀**

No more IP address management in Q-SYS - just simple device IDs!
