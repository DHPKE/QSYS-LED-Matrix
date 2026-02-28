# Network Fallback Configuration

## How It Works

The LED Matrix Controller has **built-in network fallback**:

1. **First:** Tries DHCP for 15 seconds on `eth0`
2. **If DHCP fails:** Applies static fallback IP from `config.h`
3. **Shows IP on display:** Splash screen shows current IP on startup

---

## Default Fallback Settings

From `config.h`:

```cpp
#define FALLBACK_IP      "10.20.30.40"
#define FALLBACK_NETMASK "255.255.255.0"
#define FALLBACK_GATEWAY "10.20.30.1"
#define FALLBACK_IFACE   "eth0"
#define DHCP_TIMEOUT_S   15
```

---

## Customizing the Fallback IP

### Before Installation

Edit `config.h` **before running** `install.sh`:

```cpp
#define FALLBACK_IP      "192.168.1.100"   // Your static IP
#define FALLBACK_NETMASK "255.255.255.0"    // Your subnet
#define FALLBACK_GATEWAY "192.168.1.1"      // Your router
```

Then run the installer.

### After Installation

1. Edit `config.h` with your desired IP
2. Rebuild and reinstall:
   ```bash
   cd rpiC++
   make clean
   make
   sudo cp led-matrix /usr/local/bin/
   sudo systemctl restart led-matrix
   ```

---

## Using the Web UI Instead

**Easier method:** Use the web configuration UI!

1. Let DHCP work first (so you can access the Pi)
2. Open http://<pi-ip>:8080 in your browser
3. Switch to **Static** mode
4. Enter your desired IP, subnet, gateway
5. Save and reboot

The web UI stores config in `/var/lib/led-matrix/network-config.json` and applies it automatically on boot via the `led-matrix-network.service`.

---

## Checking Current IP

### On the LED display
The IP address shows on the display at startup (splash screen) until the first command is received.

### Via SSH
```bash
hostname -I
```

### Via logs
```bash
sudo journalctl -u led-matrix | grep "NET"
```

---

## Troubleshooting

### DHCP works, but I want static IP
Use the web UI (http://<current-ip>:8080) - easier than editing code!

### No network at all
1. Check ethernet cable
2. Check switch/router
3. Connect via serial console or monitor
4. Check fallback IP was applied:
   ```bash
   ip addr show eth0
   ```

### Wrong fallback IP subnet
If your fallback IP doesn't match your network:
- The Pi will have an IP but won't be reachable
- Use a monitor/serial console to access it
- Reconfigure via web UI or `config.h`

---

## Network Flow Diagram

```
Boot
 ↓
Try DHCP (15s timeout)
 ↓
├─ DHCP Success → Use assigned IP
 ↓
└─ DHCP Timeout → Apply FALLBACK_IP
                  ↓
                  Show IP on LED display
                  ↓
                  Start UDP listener (21324)
                  Start web server (8080)
```

---

## Security Note

The fallback IP is applied **without authentication**. This is designed for local networks only.

For production:
- Set a fallback IP in your private subnet
- Don't expose to internet without firewall rules
- Use the web UI on trusted networks only

---

## Related Files

- `config.h` - Fallback IP constants
- `main.cpp` - `ensureNetwork()` and `applyFallbackIP()` functions
- `NETWORK_CONFIG_GUIDE.md` - Web UI usage
- `/var/lib/led-matrix/network-config.json` - Web UI saved config
