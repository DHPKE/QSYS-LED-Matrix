# Deployment Summary - 2026-03-01

## All Changes Pushed to Git ✅

All code changes have been committed and pushed to the main branch:
- **Repository**: https://github.com/DHPKE/QSYS-LED-Matrix
- **Latest commit**: 79bab00 - "Change behavior: text commands no longer auto-activate segments"
- **Total commits today**: 12

## Key Features Implemented

### 1. Test Mode Fixes (commits 04436d1, 35fad7f, d62cec4)
- Fixed hostname/IP text rendering issues when entering test mode from non-fullscreen layouts
- Deactivate all segments before configuring segment 0 as full-screen
- Prevent segment overlap causing cut-off text
- Remove redundant segment clearing/reconfiguration during cycle

### 2. Display Control (commits 3f28ed0, f6436ab, 295e348)
- **Display Enable/Disable toggle**: OFF clears display instantly, ON restores immediately
- **Watchdog timer**: Auto-blank after 30s of no UDP commands (Q-SYS plugin disabled)
- **Fixed Q-SYS plugin**: Display Enable button now properly sends UDP commands
- Double-buffered rendering with proper buffer swaps

### 3. Font System (commits 94bcbd2, d908499)
- Implemented per-segment font selection
- **Default font**: Arial Bold (was regular Arial, changed back)
- **Font mapping**:
  - `arial` → Arial_Bold.ttf
  - `verdana` → Verdana_Bold.ttf
  - `digital12` → DejaVuSansMono-Bold.ttf
  - `mono9` → DejaVuSansMono.ttf
- Font parameter from Q-SYS plugin now properly used
- Font cache keyed by (font_name, size) for performance

### 4. Segment Visibility Control (commit 79bab00)
- **Breaking change**: Text commands NO LONGER auto-activate segments
- Segments only visible when activated by layout presets
- **Use case**: Pre-load all 4 segments with data, switch layouts to show different combinations
- No need to resend text when changing layouts

### 5. WebUI Network Config (commit 6a43c73)
- Fixed WebUI to read actual network configuration from NetworkManager
- Query `nmcli` for ipv4.method (manual=static, auto=dhcp)
- Auto-detect connection name if netplan-eth0 doesn't exist

## Deployment Status

### Code Status
- ✅ All Python files committed to git
- ✅ Q-SYS plugin v6.7.1 committed
- ✅ install.sh already copies all .py files to /opt/led-matrix/

### Pi Deployment Status
- ⚠️ Files copied to `/tmp/` on Pi but not moved to `/opt/led-matrix/`
- ⚠️ Service not restarted due to DNS timeout issues (`unable to resolve host LZ-001`)

### Manual Steps Required on Pi

To apply ALL changes, run these commands on the Pi (10.1.1.99):

```bash
# Copy updated files from /tmp to /opt/led-matrix/
sudo cp /tmp/config.py /opt/led-matrix/
sudo cp /tmp/segment_manager.py /opt/led-matrix/
sudo cp /tmp/udp_handler.py /opt/led-matrix/
sudo cp /tmp/text_renderer.py /opt/led-matrix/

# Restart service
sudo systemctl restart led-matrix

# Verify service is running
sudo systemctl status led-matrix
```

### Alternative: Fresh Install

Or run the complete installation from scratch:

```bash
cd ~
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/rpi
bash install.sh
```

## Q-SYS Plugin

**Version**: 6.7.1
**Location**: `qsys-plugin/LEDMatrix_v6.qplug`

**Reload plugin in Q-SYS Designer** to get the display enable button fix.

## Testing Checklist

After deployment:

- [ ] Test mode from all layouts (fullscreen, triple, 50/50, vertical split)
- [ ] Display Enable toggle (OFF = instant black, ON = instant restore)
- [ ] Q-SYS plugin disable (should blank after 30 seconds)
- [ ] Font selection (Arial/Verdana/Digital/Mono)
- [ ] Segment visibility (set all 4 segments, switch layouts to show different ones)
- [ ] WebUI network config (shows correct static/DHCP mode)

## Known Issues

### DNS Resolution on Pi
- Pi hostname changed from `ledmatrixCM4` to `LZ-001`
- NetworkManager and hosts file might be out of sync
- Causing: `sudo: unable to resolve host LZ-001: Temporary failure in name resolution`
- **Fix**: Edit `/etc/hosts` to add `127.0.1.1 LZ-001` entry

### System Time Drift
- Pi clock is ~17 minutes ahead of actual time
- Causing: tar warnings about "timestamp in the future"
- **Fix**: Run `sudo timedatectl set-ntp true` or manually sync time

## Configuration Reference

### Current Settings
- **IP**: 10.1.1.99 (eth1, static via NetworkManager)
- **Hostname**: 000111 (Q-SYS plugin shows this)
- **System hostname**: LZ-001 (causing DNS issues)
- **UDP Port**: 21324
- **Web Port**: 8080
- **Matrix**: 64×32 LED panel, chain=1
- **Max Brightness**: 128/255 (50%)
- **Service User**: root (drops to daemon at runtime via rgbmatrix library)

### Font Paths
```python
FONT_PATHS = {
    "arial":     "/usr/share/fonts/truetype/msttcorefonts/Arial_Bold.ttf",
    "verdana":   "/usr/share/fonts/truetype/msttcorefonts/Verdana_Bold.ttf",
    "digital12": "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
    "mono9":     "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
}
FONT_FALLBACK = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
```

## Next Steps

1. User to manually apply changes on Pi (see "Manual Steps Required" above)
2. Test all features per checklist
3. Fix hostname/DNS issue if needed
4. Implement curtain overlay graphics feature (per FEATURE-PLAN.md)
5. Add rotation dropdown to Q-SYS plugin (0°/90°/180°/270°)

---

**Generated**: 2026-03-01 23:31 GMT+1
**Git commit**: 79bab00
**Session**: OpenClaw workspace /Users/user/.openclaw/workspace/QSYS-LED-Matrix/
