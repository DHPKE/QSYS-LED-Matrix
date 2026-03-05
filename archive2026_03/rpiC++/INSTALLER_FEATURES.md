# install.sh - Complete Feature List

## ✅ Fully Automated Fresh System Installation

The installer handles **everything** needed for a zero-config deployment on Raspbian Lite 64.

---

## Installation Steps (8 total)

### Step 1/8: System Dependencies
- Updates apt package lists
- Installs build tools: `build-essential`, `git`, `cmake`, `pkg-config`
- Installs libraries: `libfreetype6-dev`, `nlohmann-json3-dev`
- Installs fonts: `fonts-dejavu-core` (DejaVu TTF fonts)
- Installs tools: `netcat-openbsd`, `jq`

### Step 2/8: Audio Disable
- Adds `dtparam=audio=off` to `/boot/config.txt` (or `/boot/firmware/config.txt`)
- Creates `/etc/modprobe.d/blacklist-rgb-matrix.conf` to blacklist `snd_bcm2835`
- **Why:** Audio PWM conflicts with LED matrix GPIO timing
- **Requires reboot** to take effect

### Step 3/8: RGB Matrix Library
- Clones `hzeller/rpi-rgb-led-matrix` from GitHub
- Builds with `-j$(nproc)` (parallel, optimized)
- **Manually installs** (no install target in library):
  - Headers → `/usr/local/include/`
  - Libraries → `/usr/local/lib/librgbmatrix.*`
- Runs `ldconfig` to update library cache
- **Idempotent:** Skips if already installed

### Step 4/8: Build Controller
- Runs `make clean` and `make` to compile C++ controller
- Compiles: main.cpp, segment_manager.cpp, udp_handler.cpp, text_renderer.cpp, web_server.cpp
- Links with: `-lrgbmatrix -lpthread -lfreetype`
- Uses: `-O3 -march=native` optimizations
- **Interactive:** Prompts to rebuild if binary exists

### Step 5/8: Install Services
- Copies `led-matrix` binary → `/usr/local/bin/`
- Copies `apply-network-config.sh` → `/usr/local/bin/`
- Installs `led-matrix.service` → `/etc/systemd/system/`
- Installs `led-matrix-network.service` → `/etc/systemd/system/`
- Creates `/var/lib/led-matrix/` for config storage
- Sets proper ownership to the real user (not root)
- Runs `systemctl daemon-reload`

### Step 6/8: Enable Services
- Enables `led-matrix.service` (main controller)
- Enables `led-matrix-network.service` (network config applier)
- Both start automatically on boot

### Step 7/8: CPU Isolation (Optional)
- **Interactive prompt:** Configure CPU isolation?
- If yes: Adds `isolcpus=3` to `/boot/cmdline.txt`
- Dedicates CPU core 3 to LED matrix refresh thread
- Recommended for Pi 4/5 (4 cores available)
- **Requires reboot** to take effect

### Step 8/8: Network Summary
- Shows current IP address
- Provides quick access information

---

## Interactive Prompts

| Prompt | When | Recommended |
|--------|------|-------------|
| Rebuild? | Binary exists | `y` |
| CPU isolation? | Always | `y` (Pi 4/5), `n` (Pi Zero) |
| Reboot now? | Audio disabled | `y` |
| Start service? | After install | `y` |
| Send test? | Service started | `y` |

---

## What Gets Installed

### Packages (via apt)
```
build-essential       # GCC, G++, make, etc.
git                   # Version control
cmake                 # Build system
pkg-config            # Library config
libfreetype6-dev      # Font rendering
nlohmann-json3-dev    # JSON parsing
fonts-dejavu-core     # TrueType fonts
netcat-openbsd        # UDP testing
jq                    # JSON CLI tool
```

### Binaries
```
/usr/local/bin/led-matrix                 # Main controller
/usr/local/bin/apply-network-config.sh    # Network config applier
```

### Libraries
```
/usr/local/include/*.h                    # RGB matrix headers
/usr/local/lib/librgbmatrix.a             # Static library
/usr/local/lib/librgbmatrix.so.1          # Shared library
```

### Services
```
/etc/systemd/system/led-matrix.service          # Main service
/etc/systemd/system/led-matrix-network.service  # Network applier
```

### Configuration
```
/var/lib/led-matrix/                      # Config directory
/var/lib/led-matrix/config.json           # Runtime config (created on first run)
/var/lib/led-matrix/network-config.json   # Network config (created via web UI)
```

### System Changes
```
/boot/config.txt                          # dtparam=audio=off added
/etc/modprobe.d/blacklist-rgb-matrix.conf # Audio blacklist
/boot/cmdline.txt                         # isolcpus=3 (if selected)
/etc/dhcpcd.conf                          # Static IP (if configured via web UI)
```

---

## Idempotency

The installer is **safe to run multiple times**:

- Skips already-installed packages
- Skips library if headers/libs exist
- Prompts before rebuilding binary
- Skips audio disable if already done
- Skips CPU isolation if already set
- Restarts services cleanly

---

## Error Handling

The installer uses `set -e` and will abort on first error. All changes up to the error point remain applied.

**If installation fails:**
1. Fix the issue
2. Re-run `sudo ./install.sh`
3. It will skip completed steps

**To start fresh:**
```bash
sudo ./uninstall.sh
sudo ./install.sh
```

---

## Validation

After installation completes:

```bash
# Automated check
sudo ./validate-install.sh

# Manual checks
sudo systemctl status led-matrix
curl http://localhost:8080/api/config
echo '{"cmd":"text","seg":0,"text":"TEST"}' | nc -u -w1 localhost 21324
```

---

## Time Estimates

| Pi Model | Total Time |
|----------|------------|
| **Pi Zero 2W** | 15-20 min |
| **Pi 3** | 10-15 min |
| **Pi 4** | 5-10 min |
| **Pi 5** | 3-5 min |

Breakdown:
- Apt packages: 1-2 min
- RGB library build: 5-15 min (depends on Pi model)
- Controller build: 2-4 min
- Service setup: <1 min

---

## Network Behavior

### With DHCP (default):
1. Boots and gets IP from router
2. Shows IP on LED display
3. Services start automatically
4. Web UI accessible

### With Static IP (via web UI):
1. Boots, applies saved static config
2. Shows static IP on LED display
3. Services start automatically
4. Settings persist forever

### With Fallback IP (config.h):
1. Tries DHCP for 15 seconds
2. If fails, applies `FALLBACK_IP` from code
3. Useful for offline/isolated setups

---

## What Persists

After installation, these survive reboots and power loss:

✅ All binaries and libraries  
✅ Systemd service configurations  
✅ Audio disable settings  
✅ CPU isolation (if configured)  
✅ Network config (via web UI)  
✅ Runtime config (layout, brightness, orientation)  

---

## Tested Scenarios

✅ Fresh Raspbian Lite 64 (out-of-the-box)  
✅ Re-run on existing installation  
✅ DHCP network  
✅ Static IP via web UI  
✅ Fallback IP (no DHCP)  
✅ Service auto-start on boot  
✅ Config persistence  
✅ Uninstall and reinstall  

---

## Security

- No passwords in code
- No API keys required
- Web UI has no auth (local network only)
- Services run as `daemon` user
- Config files world-readable (no secrets stored)

---

## Support

**Documentation:**
- `README.md` - Overview and features
- `INSTALL_GUIDE.md` - Detailed installation
- `NETWORK_CONFIG_GUIDE.md` - Web UI usage
- `NETWORK_FALLBACK.md` - IP fallback behavior
- `CPU_OPTIMIZATION.md` - Performance tuning
- `DEPLOYMENT_SUCCESS.md` - Deployment report

**Validation:**
- `pre-install-check.sh` - Check files before install
- `validate-install.sh` - Verify installation
- `test-commands.sh` - Protocol testing

**Cleanup:**
- `uninstall.sh` - Complete removal

---

## The Bottom Line

**Run `sudo ./install.sh` on a fresh Pi and everything just works.** ✨

No manual steps, no missing dependencies, no configuration files to edit (unless you want to customize). Just run and go.

If anything fails, the installer shows exactly what went wrong. Fix it and re-run - it picks up where it left off.

**Total commands for fresh deployment:**
```bash
# 1. Copy files
scp -r rpiC++/ pi@<ip>:/home/pi/

# 2. Install
ssh pi@<ip>
cd rpiC++
sudo ./install.sh

# Done! 🎉
```
