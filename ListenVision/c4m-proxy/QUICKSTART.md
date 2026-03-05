# C4M LED Matrix Proxy - Quick Start

## What This Does

This Node.js proxy server lets you use your existing Q-SYS RPi LED Matrix plugin with 40+ C4M cards **without changing any Q-SYS code**.

**Before:**
```
Q-SYS Plugin → UDP → RPi (renders text) → HUB75 display
```

**After:**
```
Q-SYS Plugin → UDP → Node.js Proxy (renders text) → C4M SDK → 40+ C4M cards → HUB75 displays
```

## 5-Minute Setup

### 1. Install

```bash
cd ListenVision/c4m-proxy
npm install
```

### 2. Copy SDK Library

```bash
mkdir lib
cp /path/to/SDK/libsledplayer7.so lib/    # Linux
# or
cp /path/to/SDK/ledplayer7.dll lib/       # Windows
```

### 3. Configure Your Devices

Copy example config:
```bash
cp config.example.json config.json
```

Edit `config.json` and add your 40 C4M devices:
```json
{
  "devices": [
    { "id": 1, "ip": "10.1.1.101", "group": 0, "name": "LED-01" },
    { "id": 2, "ip": "10.1.1.102", "group": 0, "name": "LED-02" },
    ...add all 40 devices...
  ]
}
```

### 4. Start Proxy

```bash
node server.js
```

Should see:
```
✓ UDP server listening on 0.0.0.0:8888
✓ Web UI available at http://localhost:8080
✓ Server ready
```

### 5. Point Q-SYS to Proxy

In Q-SYS Designer, change the RPi LED Matrix IP address to your proxy server IP.

**Done!** All existing Q-SYS controls now work with 40 C4M cards.

## Testing

### Test from Command Line

```bash
# Send test command
echo '{"cmd":"test","group":0}' | nc -u localhost 8888

# Set text on segment 0
echo '{"cmd":"set_segment","group":0,"seg":0,"text":"Hello","color":"FF0000"}' | nc -u localhost 8888
```

### Test from Q-SYS

Use your existing plugin controls - they should "just work"!

### Monitor via Web UI

Open browser: `http://<proxy-ip>:8080`

## Group Control

Assign devices to groups in `config.json`:

```json
{ "id": 1, "ip": "10.1.1.101", "group": 1, "name": "Zone-A-01" },
{ "id": 2, "ip": "10.1.1.102", "group": 1, "name": "Zone-A-02" },
{ "id": 3, "ip": "10.1.1.103", "group": 2, "name": "Zone-B-01" },
```

Then from Q-SYS:
- **Group 0** = Send to ALL devices (broadcast)
- **Group 1** = Send to Zone A only
- **Group 2** = Send to Zone B only

## Troubleshooting

**"Library not found"**  
→ Copy SDK `.so`/`.dll` to `lib/` directory

**"Canvas install failed"**  
→ Run: `sudo apt-get install -y libcairo2-dev libpango1.0-dev libjpeg-dev`

**"No devices responding"**  
→ Check IPs in config.json, verify network connectivity

**"Updates are slow"**  
→ Normal - C4M takes 50-100ms per device (SDK overhead)

## Next Steps

1. Add all 40 devices to config.json
2. Test with small group first (group 1 with 2-3 devices)
3. Verify text displays correctly
4. Scale to full deployment
5. Set up as systemd service for auto-start

## Performance

- **Expected**: 10-20 devices/second update rate
- **Bandwidth**: ~5-15 Mbps for 40 devices @ 10Hz
- **Latency**: 50-100ms per device

For better performance:
- Use gigabit network
- Update only changed content
- Consider reducing update rate to 5Hz

## Files

- `server.js` - Main proxy server
- `render-engine.js` - Canvas rendering (mimics RPi)
- `c4m-sdk-wrapper.js` - C4M SDK bindings
- `config.json` - Your device configuration
- `README.md` - Full documentation

## Production Deployment

```bash
# Install as systemd service
sudo cp c4m-proxy.service /etc/systemd/system/
sudo systemctl enable c4m-proxy
sudo systemctl start c4m-proxy
sudo systemctl status c4m-proxy
```

Create `c4m-proxy.service`:
```ini
[Unit]
Description=C4M LED Matrix Proxy
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/opt/c4m-proxy
ExecStart=/usr/bin/node server.js
Restart=always

[Install]
WantedBy=multi-user.target
```
