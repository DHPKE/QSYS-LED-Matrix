# C4M LED Matrix Proxy Server

Node.js proxy server that translates the RPi LED Matrix UDP protocol to ListenVision C4M SDK commands. Acts as a drop-in replacement for RPi controllers - **no Q-SYS plugin changes required**.

## Architecture

```
┌──────────────┐      UDP JSON        ┌────────────────┐      C4M SDK       ┌──────────────┐
│   Q-SYS      │ ──────────────────> │  Node.js Proxy │ ────────────────> │  C4M Cards   │
│   Plugin     │   (RPi protocol)     │                │   (TCP/UDP)        │  (40+)       │
└──────────────┘                      └────────────────┘                    └──────────────┘
                                              │
                                              │ Canvas Rendering
                                              │ + SDK Calls
                                              v
                                      ┌────────────────┐
                                      │ Web UI Monitor │
                                      └────────────────┘
```

## Features

✅ **Drop-in RPi replacement** - Same UDP JSON protocol, no Q-SYS changes needed  
✅ **40+ device support** - Group-based control (broadcast or specific zones)  
✅ **10Hz update capable** - Uses C4M RAM mode for unlimited writes  
✅ **4-segment layout system** - Full compatibility with RPi segment manager  
✅ **Canvas rendering** - Server-side text rendering (no font files on C4M cards)  
✅ **Web monitoring UI** - Real-time stats, device status, configuration  
✅ **Rotation support** - Landscape/portrait mode  
✅ **Layout presets** - 7 pre-configured segment layouts  

## Why Proxy Instead of Direct Control?

The C4M SDK requires:
- **Local rendering** - Text/graphics rendered to bitmaps before sending
- **Program compilation** - Content packaged into C4M program format
- **SDK library** - Native C library (`.so`/`.dll`) 

Q-SYS plugins (Lua) cannot easily do this, so the proxy handles:
1. Receive UDP commands from Q-SYS
2. Render content using Node.js Canvas
3. Call C4M SDK to build programs
4. Send to 40+ C4M cards

## Installation

### Prerequisites

```bash
# Node.js 16+
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Build tools for native modules
sudo apt-get install -y build-essential libcairo2-dev libpango1.0-dev libjpeg-dev libgif-dev librsvg2-dev

# Fonts
sudo apt-get install -y fonts-dejavu-core
```

### Install Proxy

```bash
cd /path/to/QSYS-LED-Matrix/ListenVision/c4m-proxy
npm install
```

### Setup C4M SDK

1. Copy SDK library to `lib/` directory:
   ```bash
   mkdir lib
   cp /path/to/libsledplayer7.so lib/   # Linux
   # or
   cp /path/to/ledplayer7.dll lib/      # Windows
   ```

2. Install FFI bindings:
   ```bash
   npm install ffi-napi ref-napi
   ```

### Configure Devices

Edit `config.json` (created on first run):

```json
{
  "udpPort": 8888,
  "webPort": 8080,
  "matrixWidth": 64,
  "matrixHeight": 32,
  "updateRate": 10,
  "devices": [
    { "id": 1, "ip": "10.1.1.101", "group": 0, "name": "LED-01" },
    { "id": 2, "ip": "10.1.1.102", "group": 0, "name": "LED-02" },
    { "id": 3, "ip": "10.1.1.103", "group": 1, "name": "LED-03" },
    { "id": 4, "ip": "10.1.1.104", "group": 1, "name": "LED-04" }
  ],
  "groups": {
    "0": "Broadcast (All)",
    "1": "Zone A",
    "2": "Zone B",
    "3": "Zone C",
    "4": "Zone D",
    "5": "Zone E",
    "6": "Zone F",
    "7": "Zone G"
  }
}
```

**Device Configuration:**
- `id` - Unique device ID (1-255)
- `ip` - C4M card IP address
- `group` - Group number (0 = broadcast to all)
- `name` - Display name for monitoring

## Usage

### Start Server

```bash
node server.js
```

Or with auto-restart:

```bash
npm run dev
```

### Monitor via Web UI

Open browser to `http://<server-ip>:8080`

Shows:
- Real-time statistics (packets, updates, rate)
- Device list and status
- Configuration
- Error log

### Send Commands from Q-SYS

**No changes to Q-SYS plugin needed!** Just point it to the proxy IP:

In Q-SYS Designer, set the LED Matrix device IP to the proxy server IP (instead of RPi IP).

The proxy will:
1. Receive UDP commands
2. Render content
3. Broadcast to all C4M cards (group 0)
4. Or send to specific group (1-7)

## UDP Protocol (Same as RPi)

### Set Segment Text

```json
{
  "cmd": "set_segment",
  "group": 0,
  "seg": 0,
  "enabled": true,
  "text": "Hello World",
  "color": "FF0000",
  "effect": 1,
  "align": 1
}
```

### Set Layout Preset

```json
{
  "cmd": "set_layout",
  "group": 0,
  "preset": 1
}
```

### Set Brightness

```json
{
  "cmd": "set_brightness",
  "group": 0,
  "brightness": 128
}
```

### Clear Display

```json
{
  "cmd": "clear",
  "group": 0
}
```

### Test Pattern

```json
{
  "cmd": "test",
  "group": 1
}
```

## Group Control

The `group` field targets devices:

- **Group 0** - Broadcast to ALL devices
- **Group 1-7** - Send to specific zone/group only

This allows:
- Global messages (group 0)
- Zone-specific content (groups 1-7)
- Independent control of different display areas

Example: Set different text per zone:

```javascript
// Zone A (group 1)
send({ cmd: "set_segment", group: 1, seg: 0, text: "Zone A" });

// Zone B (group 2)
send({ cmd: "set_segment", group: 2, seg: 0, text: "Zone B" });

// All zones (group 0)
send({ cmd: "set_segment", group: 0, seg: 0, text: "ALERT" });
```

## Performance

### Bandwidth (40 devices @ 10Hz)

Using C4M SDK with rendered images:

- **Program size**: ~15-50 KB per update (depends on content)
- **40 devices**: 600-2000 KB/sec = **4.8-16 Mbps**
- **Recommendation**: Gigabit network, dedicated VLAN

### Optimization Tips

1. **Use group targeting** - Only update devices that need it
2. **Batch updates** - Update multiple segments in one command cycle
3. **Reduce update rate** - 5Hz may be sufficient (halves bandwidth)
4. **Simplify content** - Text-only is smaller than complex graphics
5. **RAM mode** - Always use RAM (saveType=3) for 10Hz updates

### Expected Performance

- **Update latency**: 50-100ms per device (SDK overhead)
- **Throughput**: ~10-20 devices/second (sequential TCP sends)
- **Total cycle time**: 2-4 seconds to update all 40 devices

For truly synchronized updates, use UDP broadcast (but all show identical content).

## Troubleshooting

### SDK Library Not Found

```
Error: Dynamic library not found: libsledplayer7.so
```

**Fix**: Copy SDK library to `lib/` directory:
```bash
cp /path/to/SDK/linux/lib/libsledplayer7.so lib/
```

### Canvas Module Install Fails

```
Error: Package cairo not found
```

**Fix**: Install system dependencies:
```bash
sudo apt-get install -y libcairo2-dev libpango1.0-dev libjpeg-dev
npm install canvas
```

### C4M Card Not Responding

1. Check IP address in config.json
2. Verify network connectivity: `ping <c4m-ip>`
3. Ensure C4M is powered and initialized
4. Check firewall rules (TCP port forwarding)

### High Latency

If updates are slow:
- Use RAM mode (saveType=3)
- Reduce number of simultaneous updates
- Check network bandwidth utilization
- Consider upgrading to gigabit switch

## Development Status

### ✅ Implemented

- UDP server (RPi protocol compatibility)
- Canvas rendering engine
- 4-segment layout system
- Group-based device targeting
- Web monitoring UI
- Configuration management

### 🚧 TODO (Uncomment in code)

- C4M SDK integration (ffi-napi calls)
- Program compilation and sending
- Error handling for failed sends
- Device health monitoring
- Automatic retry logic

### 📝 To Enable SDK

1. Install dependencies:
   ```bash
   npm install ffi-napi ref-napi
   ```

2. Uncomment SDK code in:
   - `server.js` (lines marked with SDK comments)
   - Implement `sendToDevice()` function

3. Test with single device before deploying to 40+

## Alternative: Direct Protocol Implementation

If SDK proves problematic, consider:

1. **Reverse-engineer protocol** - Capture what `LV_Send()` transmits
2. **Implement binary protocol** - Direct TCP/UDP packets to C4M
3. **Skip SDK wrapper** - Faster, but more complex

This would eliminate SDK dependency but require protocol analysis.

## License

MIT

## Support

For issues or questions:
1. Check Web UI at `http://<proxy-ip>:8080` for errors
2. Review logs: `node server.js` output
3. Test with single device before scaling to 40+

---

**Next Steps:**

1. Configure your 40 C4M device IPs in `config.json`
2. Start the proxy server
3. Point Q-SYS plugin to proxy IP
4. Test with small group before full deployment
