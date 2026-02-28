# Installation Guide - Raspbian Lite 64

## 🎯 One-Command Install

```bash
sudo ./install.sh
```

This **out-of-the-box installer** handles everything automatically on a fresh Raspbian Lite 64 installation.

---

## 📦 What It Does

### Step 1: System Dependencies
Installs via `apt-get`:
- `build-essential` - gcc, g++, make
- `git`, `cmake`, `pkg-config` - Build tools
- `libfreetype6-dev` - Font rendering
- `nlohmann-json3-dev` - JSON parsing
- `netcat-openbsd` - Testing tool

### Step 2: Audio Disable
Prevents audio interference with LED timing:
- Adds `dtparam=audio=off` to `/boot/config.txt` (or `/boot/firmware/config.txt`)
- Creates `/etc/modprobe.d/blacklist-rgb-matrix.conf` to blacklist `snd_bcm2835`
- **May require reboot** (script will prompt)

### Step 3: RGB Matrix Library
Clones and compiles [hzeller/rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix):
- Clones to `/tmp/rpi-rgb-led-matrix`
- Builds with `make -j$(nproc)` (uses all CPU cores)
- Installs headers to `/usr/local/include/`
- Installs library to `/usr/local/lib/`
- Runs `ldconfig` to update linker cache

**Skips if already installed** (checks for `/usr/local/include/led-matrix.h`)

### Step 4: Build Controller
Compiles the LED matrix controller:
- Runs `make` in current directory
- Links against rgbmatrix, freetype, pthread
- Produces `led-matrix` binary (~100KB)

**Prompts to rebuild** if binary already exists

### Step 5: Install Service
- Copies `led-matrix` → `/usr/local/bin/led-matrix`
- Copies `led-matrix.service` → `/etc/systemd/system/`
- Runs `systemctl daemon-reload`
- Creates `/var/lib/led-matrix/` for config persistence
- Sets ownership to real user (not root)

### Step 6: Enable Service
- Runs `systemctl enable led-matrix`
- Service will auto-start on boot

### Step 7: Network Check
- Detects device IP address
- Shows test commands
- Optionally starts service immediately

---

## ⏱️ Installation Time

**Fresh Raspbian Lite 64** (with internet):
- Fast Pi 4/5: ~5-10 minutes
- Pi Zero 2W: ~15-20 minutes

**Most time spent on**:
- Downloading packages (~2-3 min)
- Building rpi-rgb-led-matrix (~3-5 min on Pi 4, ~10-15 min on Pi Zero 2W)
- Building controller (<1 min)

---

## 🔄 Post-Install

### If Reboot Required
```bash
sudo reboot
```

After reboot:
```bash
sudo systemctl start led-matrix
sudo systemctl status led-matrix
```

### Check Display
Should show device IP address in white text with frame:
```
┌──────────────┐
│  10.1.1.22   │
└──────────────┘
```

### Send Test Command
```bash
IP=$(hostname -I | awk '{print $1}')
echo '{"cmd":"text","seg":0,"text":"WORKS!","color":"00FF00"}' | nc -u -w1 "$IP" 21324
```

### Run Full Test Suite
```bash
./test-commands.sh $(hostname -I | awk '{print $1}')
```

---

## 🎛️ Pre-Install Configuration

### Before Running install.sh

If you need custom hardware settings, edit `config.h` first:

```bash
nano config.h
# Edit MATRIX_WIDTH, MATRIX_HEIGHT, GPIO_SLOWDOWN, etc.
# Save and exit

sudo ./install.sh  # Will build with your settings
```

### Common Adjustments

**For 32×16 panel:**
```cpp
#define MATRIX_WIDTH   32
#define MATRIX_HEIGHT  16
```

**For Pi 4 (faster GPIO):**
```cpp
#define GPIO_SLOWDOWN  2
```

**For WiFi instead of Ethernet:**
```cpp
#define FALLBACK_IFACE "wlan0"
```

**For different network:**
```cpp
#define FALLBACK_IP      "192.168.1.100"
#define FALLBACK_NETMASK "255.255.255.0"
#define FALLBACK_GATEWAY "192.168.1.1"
```

---

## 🔍 Verification Steps

After installation:

### 1. Check Service Status
```bash
sudo systemctl status led-matrix
```

Should show:
```
● led-matrix.service - RGB LED Matrix Controller
   Loaded: loaded
   Active: active (running)
```

### 2. Check Logs
```bash
sudo journalctl -u led-matrix -n 50
```

Should see:
```
==================================================
RPi RGB LED Matrix Controller (C++)
==================================================
Matrix: 64×32, chain=1
[NET] ✓ DHCP address: 10.1.1.22
✓ LED matrix initialized (64×32)
[UDP] Listening on 0.0.0.0:21324
System ready
```

### 3. Check Network
```bash
sudo netstat -ulnp | grep 21324
```

Should show:
```
udp  0  0.0.0.0:21324  0.0.0.0:*  <pid>/led-matrix
```

### 4. Send Test Command
```bash
echo '{"cmd":"clear_all"}' | nc -u -w1 <IP> 21324
```

Display should go black (all segments cleared).

---

## 🆘 Common Issues

### "Permission denied" when running
**Solution**: Must run as root
```bash
sudo systemctl start led-matrix  # Preferred
# OR
sudo ./led-matrix                # Direct run
```

### Flickering/unstable display
**Solution 1**: Disable audio (installer does this automatically)
```bash
grep "dtparam=audio=off" /boot/config.txt
# Should return: dtparam=audio=off
```

**Solution 2**: Increase GPIO slowdown
```bash
nano config.h
# Change: #define GPIO_SLOWDOWN 4
make clean && make
sudo systemctl restart led-matrix
```

### "apt-get: command not found"
**Solution**: Use `apt` instead
```bash
sudo apt update
sudo apt install build-essential
```

### Build takes too long
**Solution**: Use parallel compilation (installer does this automatically)
```bash
make -j$(nproc)  # Uses all CPU cores
```

---

## 🧹 Uninstall

```bash
sudo ./uninstall.sh
```

Interactive prompts to:
- Remove service and binary
- Keep or delete config files (`/var/lib/led-matrix/`)
- Keep or delete rpi-rgb-led-matrix library

---

## 📊 Install Log

Full installation output is saved to:
```
/var/log/led-matrix-install.log
```

View with:
```bash
sudo less /var/log/led-matrix-install.log
```

---

## 🎬 Next Steps After Install

1. **Test with QSYS plugin** - Point plugin to device IP
2. **Run test suite** - `./test-commands.sh <IP>`
3. **Check performance** - `top` or `htop` while running
4. **Verify effects** - Test scroll, blink, layouts
5. **Adjust settings** - Edit `config.h` and rebuild if needed

---

## 💡 Pro Tips

### Fast Rebuild
```bash
make && sudo systemctl restart led-matrix
```

### Live Logs
```bash
sudo journalctl -u led-matrix -f
# Press Ctrl+C to exit
```

### Quick Test
```bash
# Save as alias in ~/.bashrc
alias ledtest='echo "{\"cmd\":\"text\",\"seg\":0,\"text\":\"TEST\"}" | nc -u -w1 $(hostname -I | awk "{print \$1}") 21324'

# Then just run:
ledtest
```

### Monitor Performance
```bash
# CPU usage
top -p $(pgrep led-matrix)

# Memory
ps aux | grep led-matrix | grep -v grep
```

---

**Installation support**: See `DEPLOYMENT_GUIDE.md` for detailed troubleshooting.  
**Protocol reference**: See `PORTING_NOTES.md` for technical details.  
**Hardware wiring**: See `HUB75_WIRING_MAP.md` and `VERIFIED_PINOUT.md`.

---

✅ **Ready to deploy on Raspbian Lite 64!**
