# LED Matrix Project - Complete Status (2026-03-09)

**Date**: 2026-03-09 20:35 CET  
**Repository**: https://github.com/DHPKE/QSYS-LED-Matrix  
**Branch**: feature/curtain-frame-indicator  
**Status**: ✅ Production Ready

---

## Hardware Configuration

### LED Matrix Panel
- **Resolution**: 128×64 pixels
- **Color Order**: BGR (not RGB!)
- **Interface**: HUB75
- **Usable Area**: 122×58 pixels (with 3px curtain)

### Raspberry Pi CM4
- **IP**: 10.1.1.26 (DHCP)
- **Fallback IP**: 10.10.10.99/24 (NetworkManager static)
- **OS**: DietPi
- **Service**: led-matrix.service (running)
- **UDP Port**: 21324
- **HTTP Port**: 8080 (web UI)

### Performance
- **CPU Usage**: ~40% (CM4 has plenty headroom)
- **Scan Rate**: ~200Hz (GPIO_SLOWDOWN=3)
- **PWM Frequency**: Maximum (PWM_BITS=5)
- **Color Depth**: 32K colors (32³)
- **Anti-Flicker**: Optimized for solid backgrounds

---

## Software Stack

### Python Application (v7.1)
**Location**: `/opt/led-matrix/`

**Core Files**:
- `main.py` - Entry point
- `config.py` - All configuration parameters
- `udp_handler.py` - UDP command processor
- `segment_manager.py` - Layout & segment logic
- `text_renderer.py` - Text rendering with effects
- `curtain_manager.py` - Curtain frame rendering
- `web_server.py` - HTTP API (8080)

**Key Configuration** (`config.py`):
```python
# Display
MATRIX_WIDTH = 128
MATRIX_HEIGHT = 64
MATRIX_LED_RGB_SEQUENCE = "BGR"

# Anti-Flicker
MATRIX_GPIO_SLOWDOWN = 3
MATRIX_PWM_BITS = 5
MATRIX_REFRESH_LIMIT = 0
MATRIX_SCAN_MODE = 0
MATRIX_ROW_ADDRESS_TYPE = 0
MATRIX_MULTIPLEXING = 0
MATRIX_PWM_DITHER_BITS = 0

# Curtain
CURTAIN_AUTO_REMAP = True  # Auto-scale layouts 1-7, 11-14

# Text
FONT_SIZES = [64, 60, 56, ..., 8, 7, 6]  # 6-64px range
LEFT_ALIGN_PADDING = 1  # 1px from segment edge
```

**Text Padding**: 1px gap from all segment edges (avail_w/h - 2px)

### Q-SYS Plugin (v7.1)

**Files**:
- `LEDMatrix_v7.1.qplug` - 128×64 BGR variant
- Plugin ID: `dhpke.olimex.led.matrix.128x64.bgr`
- Version: 7.1.0

**Features**:
- 8 independent groups
- 4 segments per group
- Curtain mode with auto-sync
- Rotation (0°, 90°, 180°, 270°)
- Font sizes (6-64px)
- Effects (scroll, fade, blink)
- Layout presets (1-9, 11-14)

**Default IP**: 10.10.10.99 (can be changed in plugin)

### Node-RED Module (v3.0.0)

**Package**: `node-red-contrib-led-matrix`  
**Location**: `NODERED-LED-Matrix/`

**Files**:
- `led-matrix.js` - Node logic
- `led-matrix.html` - UI configuration
- `package.json` - v3.0.0
- `README.md` - Documentation

**All Parameters Configurable**:
- Segment (0-3)
- Group (0-8)
- Text (color, bgcolor, font, size, align, effect, intensity)
- Layout (1-9, 11-14)
- Brightness (0-255)
- Rotation (0°, 90°, 180°, 270°)
- Frame (enabled, color, width)
- Curtain (enabled, color)

**Installation**:
```bash
cd ~/.node-red
npm install node-red-contrib-led-matrix
```

---

## Features Implemented

### ✅ Display Features
- [x] 128×64 BGR color order support
- [x] Anti-flicker optimization (PWM_BITS=5)
- [x] Text padding (1px from edges)
- [x] Font sizes 6-64px + auto-fit
- [x] 9 layout presets (1-7 standard, 8-9 VO modes, 11-14 fullscreen)
- [x] Rotation (0°, 90°, 180°, 270°) - persists across reboots

### ✅ Curtain Mode
- [x] 3px frame on all edges (top, right, bottom, left)
- [x] Auto-scale: Layouts 1-7, 11-14 → 122×58 usable
- [x] Fixed margins: VO layouts 8-9 keep 3px gaps
- [x] Per-group curtain color
- [x] Show/hide per group

### ✅ Text Rendering
- [x] Left/Center/Right alignment with 1px padding
- [x] Effects: none, scroll, fade, blink
- [x] Auto-fit or fixed size (6-64px)
- [x] Color + background color per segment
- [x] Intensity control (0-255)

### ✅ Group Management
- [x] 8 independent groups
- [x] 4 segments per group
- [x] Per-group brightness
- [x] Per-group rotation
- [x] Per-group curtain
- [x] Broadcast to all groups (group=0)

### ✅ Network
- [x] UDP control (port 21324)
- [x] HTTP API (port 8080)
- [x] DHCP + fallback static IP (10.10.10.99/24)
- [x] NetworkManager integration
- [x] Web UI for configuration

### ✅ Integration
- [x] Q-SYS plugin v7.1
- [x] Node-RED module v3.0.0
- [x] UDP JSON protocol
- [x] HTTP REST API

---

## Layout Presets

### Standard Layouts (Auto-Scale with Curtain)
1. **Fullscreen** - Single segment (122×58 with curtain)
2. **Top/Bottom** - 2 segments horizontal split
3. **Left/Right** - 2 segments vertical split
4. **Triple Left** - 3 segments (left + 2 right)
5. **Triple Right** - 3 segments (right + 2 left)
6. **Thirds** - 3 equal segments
7. **Quad View** - 4 equal segments

### VO Layouts (Fixed 3px Margins)
8. **VO Left** - Large left (100px) + small right (18×14)
9. **VO Right** - Large right (64px) + small left (18×14)

### Fullscreen Per Segment (Auto-Scale with Curtain)
11. **Fullscreen Seg 1** - Segment 0 fullscreen
12. **Fullscreen Seg 2** - Segment 1 fullscreen
13. **Fullscreen Seg 3** - Segment 2 fullscreen
14. **Fullscreen Seg 4** - Segment 3 fullscreen

---

## UDP Command Protocol

### Text Command
```json
{
  "cmd": "text",
  "seg": 0,
  "group": 1,
  "text": "Hello World",
  "color": "FFFFFF",
  "bgcolor": "000000",
  "font": "arial",
  "size": "auto",
  "align": "C",
  "effect": "none",
  "intensity": 255
}
```

### Layout Command
```json
{
  "cmd": "layout",
  "preset": 7,
  "group": 0
}
```

### Brightness Command
```json
{
  "cmd": "brightness",
  "value": 128,
  "group": 0
}
```

### Rotation Command
```json
{
  "cmd": "rotation",
  "value": 90,
  "group": 0
}
```

### Curtain Command
```json
{
  "cmd": "curtain",
  "group": 1,
  "enabled": true,
  "color": "FF0000"
}
```

### Frame Command
```json
{
  "cmd": "frame",
  "seg": 0,
  "enabled": true,
  "color": "FFFFFF",
  "width": 1,
  "group": 0
}
```

### Clear Commands
```json
{"cmd": "clear", "seg": 0, "group": 1}
{"cmd": "clear_all", "group": 0}
```

---

## Installation (Fresh Pi)

### Automated Install Script
**Location**: `rpi/install.sh`  
**Version**: 7.1.0

**Features**:
- DietPi + Raspberry Pi OS support
- Auto-installs NetworkManager if needed
- Configures fallback IP (10.10.10.99/24)
- Compiles rpi-rgb-led-matrix library
- Installs Python bindings with verification
- Creates systemd service
- Zero manual intervention needed

**Run Installation**:
```bash
cd ~/QSYS-LED-Matrix/rpi
bash install.sh
```

**Installation Steps** (automatic):
1. Pre-flight checks (hardware, OS, network)
2. Blacklist audio driver (conflicts with PWM)
3. Install system packages + NetworkManager
4. Clone/build rpi-rgb-led-matrix library
5. Install Python bindings (with verification)
6. Copy application files to /opt/led-matrix
7. Configure network fallback IP
8. Install helper scripts
9. Configure sudoers
10. Install systemd service

**Reboot Required**: Yes (for audio blacklist + network config)

---

## Network Configuration

### Primary IP (DHCP)
- Obtained from DHCP server
- Currently: 10.1.1.26/24

### Fallback IP (Static)
- **IP**: 10.10.10.99/24
- **Gateway**: 10.10.10.1
- **DNS**: 8.8.8.8
- **Manager**: NetworkManager

**Behavior**:
- Both IPs active when DHCP works
- Fallback always accessible (even if DHCP fails)
- No downtime during network changes

**Access**:
- Web UI: http://10.10.10.99:8080/
- UDP: 10.10.10.99:21324

**Configuration**:
```bash
nmcli connection modify "Wired connection 1" +ipv4.addresses 10.10.10.99/24
```

---

## Documentation Files

### Deployment Docs
- `DEPLOYMENT-128x64-BGR.md` - Initial 128×64 migration
- `CURTAIN-AUTOSCALE-DEPLOYMENT.md` - Curtain auto-scale implementation
- `CURTAIN-BEHAVIOR.md` - Layout curtain behavior
- `ANTI-FLICKER-FINAL.md` - Anti-flicker optimization process
- `NETWORK-FALLBACK-IP.md` - Network configuration
- `FRESH-INSTALL-GUIDE.md` - Installation guide

### Reference Docs
- `VO-LAYOUT-FIX-128x64.md` - VO layout coordinate fixes
- `VO-LEFT-OVERLAP-FIX.md` - VO-left segment overlap fix
- `LEFT-ALIGN-PADDING-FIX.md` - Text padding implementation
- `FINAL-FIXES-CURTAIN-FONTS.md` - Curtain gap & font fixes

### Feature Docs
- `LAYOUT-8-9-REFERENCE.md` - VO layout documentation
- `LAYOUTS-15-16-VO.md` - Additional VO layouts
- `HOW-TO-ADJUST-VO-SIZES.md` - VO customization guide
- `CURTAIN_MODE.md` - Curtain mode guide (rpi/)
- `GROUPING_GUIDE.md` - Group management
- `HUB75_WIRING_MAP.md` - Hardware wiring

### Development Docs
- `CODE_REVIEW_2026-02-20.md` - Code review notes
- `PROJECT_STATUS.md` - Project status (legacy)
- `CHANGELOG.md` - Version history

---

## Backup Files (On Pi)

**Location**: `/opt/led-matrix/`

**config.py backups**:
- `config.py.point-zero` - Baseline before optimization
- `config.py.backup-20260309-144302` - Before VO layout fix
- `config.py.backup-vo-fix` - After VO layout fix
- `config.py.backup-before-vo-128x64` - Before 128×64 migration
- `config.py.backup-before-autoscale` - Before curtain auto-scale
- `config.py.backup-before-flicker-fix` - Before anti-flicker
- `config.py.backup-before-cm4-optimize` - Before CM4 optimization

**Other backups**:
- `segment_manager.py.backup-before-autoscale`
- `text_renderer.py.backup-before-1px-padding`
- `/etc/dhcpcd.conf.backup` - dhcpcd backup (if used)

---

## Git History (Today's Work)

```
31169cf - Node-RED v3.0.0: Full parameter control for 128×64 BGR matrix
d0c9ec2 - Improved install.sh for fresh DietPi/RaspberryPiOS installs
8d9d685 - Add curtain auto-scale + anti-flicker optimization
3645098 - Add 128×64 BGR matrix support (v7.1 plugin + deployment docs)
```

**Total commits today**: 4 major commits  
**Lines changed**: ~2000+ lines (code + docs)  
**Files modified**: 20+ files

---

## Testing Checklist

### ✅ Completed Tests
- [x] 128×64 BGR color order verified
- [x] All layouts render correctly (1-9, 11-14)
- [x] Curtain auto-scale works (layouts 1-7, 11-14)
- [x] VO layouts keep fixed margins (8, 9)
- [x] Text padding 1px from edges
- [x] Anti-flicker optimization (PWM_BITS=5)
- [x] Rotation persists across reboots
- [x] Fallback IP always accessible (10.10.10.99)
- [x] Service restarts cleanly
- [x] Web UI accessible
- [x] UDP commands work
- [x] Python module imports successfully

### 📋 User Acceptance
- [x] LED flicker acceptable on solid backgrounds
- [x] Text looks sharp with 1px margins
- [x] Curtain frame rendering correct
- [x] Install script ready for fresh installs

---

## Future Development Ideas

### Potential Enhancements
1. **Animation Library**
   - Pre-built animations (fire, rain, matrix effect)
   - Background effects
   - Transition animations between layouts

2. **Real-Time Data**
   - Clock display (auto-update every second)
   - Weather integration (OpenWeather API)
   - System monitoring (CPU temp, memory, uptime)

3. **Advanced Features**
   - QR code generation & display
   - RSS feed ticker
   - Audio visualization (FFT, spectrum bars)
   - MQTT integration (Home Assistant, IoT)

4. **Games**
   - Snake (UDP control)
   - Pong
   - Tetris

5. **Scheduling**
   - Time-based layout switching
   - Content scheduling
   - Auto-dimming (time of day)

---

## Troubleshooting

### Service Won't Start
```bash
sudo systemctl status led-matrix
sudo journalctl -u led-matrix -f
```

### Check Python Module
```bash
python3 -c "from rgbmatrix import RGBMatrix; print('OK')"
```

### Verify Network
```bash
ip addr show eth0
ping 10.10.10.99
```

### Test UDP
```bash
echo '{"cmd":"text","seg":0,"text":"TEST","color":"FF0000"}' | nc -u -w1 10.10.10.99 21324
```

### Check Logs
```bash
sudo journalctl -u led-matrix -n 100 --no-pager
```

### Restart Service
```bash
sudo systemctl restart led-matrix
```

### Revert to Backup
```bash
sudo cp /opt/led-matrix/config.py.point-zero /opt/led-matrix/config.py
sudo systemctl restart led-matrix
```

---

## Key Contacts & Resources

**Repository**: https://github.com/DHPKE/QSYS-LED-Matrix  
**Branch**: feature/curtain-frame-indicator  
**Author**: DHPKE  
**Library**: https://github.com/hzeller/rpi-rgb-led-matrix  

**Hardware**:
- LED Matrix: 128×64 HUB75 BGR
- Controller: Raspberry Pi CM4
- Power: 5V regulated supply

**Support**:
- Q-SYS Forum: https://q-sysforums.qsc.com
- GitHub Issues: https://github.com/DHPKE/QSYS-LED-Matrix/issues

---

## Summary

**Status**: ✅ Production ready, fully documented, all features working

**What Works**:
- 128×64 BGR display with anti-flicker
- Curtain auto-scale + fixed VO margins
- All parameters controllable (Q-SYS, Node-RED, UDP)
- Network fallback (always accessible at 10.10.10.99)
- Fresh install script (zero manual steps)

**What's Ready**:
- Q-SYS plugin v7.1
- Node-RED module v3.0.0
- Python application v7.1
- Installation script v7.1
- Complete documentation

**Next Steps** (when needed):
- Merge feature branch to main
- Tag release v7.1.0
- Publish Node-RED module to npm
- Deploy to additional panels
- Implement future enhancements

---

**Document Date**: 2026-03-09 20:35 CET  
**Last Commit**: 31169cf  
**Session Duration**: ~9 hours (6 sessions)  
**Final Status**: All objectives complete ✅
