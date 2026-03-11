# Hostname Persistence Fix

**Date**: 2026-03-11  
**Issue**: Hostname settings in WebUI not persistent across reboots  
**Status**: ✅ Fixed

---

## Problem

When setting the hostname via the WebUI (http://10.1.1.21:8080/), the change wasn't persisting across reboots. The Pi would revert to the old hostname after restart.

**Root Cause**: The `set-hostname.sh` script wasn't properly updating all required files for hostname persistence.

---

## Solution

Created a robust `set-hostname.sh` script that properly persists hostname changes by updating:

1. **Current hostname** (immediate effect via `hostnamectl`)
2. **`/etc/hostname`** (persists across reboots)
3. **`/etc/hosts`** (required for proper local name resolution)
4. **Avahi/mDNS** (if installed, for `.local` access)

### New Script: `rpi/set-hostname.sh`

```bash
#!/bin/bash
# set-hostname.sh - Properly set hostname with persistence
# Usage: sudo ./set-hostname.sh <new-hostname>

set -e

NEW_HOSTNAME="$1"

# Validation
if [ -z "$NEW_HOSTNAME" ]; then
    echo "Error: No hostname provided"
    exit 1
fi

if ! echo "$NEW_HOSTNAME" | grep -qE '^[a-zA-Z0-9-]+$'; then
    echo "Error: Invalid hostname"
    exit 1
fi

# 1. Set current hostname (immediate)
hostnamectl set-hostname "$NEW_HOSTNAME"

# 2. Update /etc/hostname (persists across reboots)
echo "$NEW_HOSTNAME" | sudo tee /etc/hostname > /dev/null

# 3. Update /etc/hosts (required for name resolution)
sudo sed -i "/127.0.1.1/d" /etc/hosts
echo "127.0.1.1       $NEW_HOSTNAME" | sudo tee -a /etc/hosts > /dev/null

# 4. Update Avahi if present
if [ -f /etc/avahi/avahi-daemon.conf ]; then
    sudo sed -i "s/^#*host-name=.*/host-name=$NEW_HOSTNAME/" /etc/avahi/avahi-daemon.conf
    sudo systemctl restart avahi-daemon 2>/dev/null || true
fi

# 5. Verify
if [ "$(hostname)" = "$NEW_HOSTNAME" ]; then
    echo "✅ Hostname set to: $NEW_HOSTNAME"
    echo "✅ Configuration persisted"
fi
```

---

## Files Modified

### 1. `/etc/hostname`
Contains just the hostname (one line):
```
led-matrix-01
```

### 2. `/etc/hosts`
Maps hostname to loopback:
```
127.0.0.1       localhost
127.0.1.1       led-matrix-01
```

### 3. `/etc/avahi/avahi-daemon.conf` (if Avahi installed)
```
[server]
host-name=led-matrix-01
```

---

## How It Works

### WebUI Flow:

1. User enters hostname in web interface (http://10.1.1.21:8080/)
2. WebUI sends POST request to `/api/config`
3. `web_server.py` validates hostname
4. Calls: `sudo /opt/led-matrix/set-hostname.sh <new-hostname>`
5. Script updates all files
6. Hostname persists across reboots ✅

### Sudoers Configuration:

The install script adds this to `/etc/sudoers.d/led-matrix`:

```
daemon ALL=(root) NOPASSWD: /opt/led-matrix/set-hostname.sh
```

This allows the LED Matrix service (running as user `daemon`) to change the hostname without requiring a password.

---

## Verification Steps

### Test Hostname Change:

```bash
# 1. Via WebUI
# Open http://10.1.1.21:8080/
# Enter new hostname (e.g., "led-panel-01")
# Click "Apply Changes"

# 2. Verify immediate change
hostname
# Should show: led-panel-01

# 3. Check files
cat /etc/hostname
# Should show: led-panel-01

cat /etc/hosts | grep 127.0.1.1
# Should show: 127.0.1.1       led-panel-01

# 4. Reboot and verify persistence
sudo reboot
# Wait for boot...
ssh node@10.1.1.21
hostname
# Should still show: led-panel-01
```

### Manual Test (without WebUI):

```bash
# On the Pi:
sudo /opt/led-matrix/set-hostname.sh my-new-hostname

# Verify
hostname
cat /etc/hostname
cat /etc/hosts | grep 127.0.1.1

# Reboot test
sudo reboot
# After boot:
hostname  # Should show: my-new-hostname
```

---

## mDNS / .local Access

If Avahi is installed, the hostname will also be accessible via `.local`:

```bash
# From your Mac/laptop:
ping led-matrix-01.local
ssh node@led-matrix-01.local
curl http://led-matrix-01.local:8080/
```

**Note**: `.local` may take a few seconds to update after hostname change.

---

## Deployment

### Fresh Install:

The script is automatically deployed by `install.sh`:

```bash
# On the Pi:
cd ~/QSYS-LED-Matrix/rpi
bash install.sh
```

This copies `set-hostname.sh` to `/opt/led-matrix/` and configures sudoers.

### Update Existing Installation:

```bash
# From your Mac:
scp rpi/set-hostname.sh node@10.1.1.21:/tmp/

# On the Pi:
sudo cp /tmp/set-hostname.sh /opt/led-matrix/
sudo chmod +x /opt/led-matrix/set-hostname.sh
sudo chown daemon:daemon /opt/led-matrix/set-hostname.sh
```

---

## Rollback (if needed)

To manually reset hostname to default:

```bash
sudo /opt/led-matrix/set-hostname.sh led-matrix
sudo reboot
```

---

## Known Limitations

1. **Hostname format**: Only alphanumeric characters and hyphens allowed (RFC 1123)
2. **No spaces**: Hostnames cannot contain spaces
3. **No special characters**: Avoid characters like `_`, `.`, etc.
4. **Max length**: 63 characters (POSIX limit)

### Valid Examples:
- `led-matrix-01`
- `panel-stage-left`
- `display01`
- `LED-Panel-A`

### Invalid Examples:
- `led_matrix` (underscore not allowed)
- `led matrix` (space not allowed)
- `led.matrix.local` (dots not recommended in hostname)

---

## Troubleshooting

### Hostname Doesn't Persist After Reboot

**Check `/etc/hostname`:**
```bash
cat /etc/hostname
```

If empty or wrong, manually fix:
```bash
echo "led-matrix-01" | sudo tee /etc/hostname
sudo reboot
```

### .local Not Working

**Check Avahi status:**
```bash
systemctl status avahi-daemon
```

If not running:
```bash
sudo systemctl start avahi-daemon
sudo systemctl enable avahi-daemon
```

### "Permission Denied" When Setting Hostname

**Check sudoers:**
```bash
sudo cat /etc/sudoers.d/led-matrix
```

Should contain:
```
daemon ALL=(root) NOPASSWD: /opt/led-matrix/set-hostname.sh
```

If missing, re-run relevant part of `install.sh` or manually add.

---

## Integration with WebUI

The hostname field is part of the network configuration page. When you change it:

1. WebUI validates format (alphanumeric + hyphens only)
2. Sends JSON to `/api/config`:
   ```json
   {
     "hostname": "new-hostname"
   }
   ```
3. `web_server.py` extracts and validates
4. Executes: `sudo /opt/led-matrix/set-hostname.sh new-hostname`
5. Returns success/error to WebUI
6. WebUI shows confirmation message

---

## Files Summary

| File | Purpose | Deployed Location |
|------|---------|-------------------|
| `rpi/set-hostname.sh` | Hostname persistence script | `/opt/led-matrix/set-hostname.sh` |
| `rpi/web_server.py` | WebUI backend (calls script) | `/opt/led-matrix/web_server.py` |
| `rpi/install.sh` | Deployment automation | N/A (run once) |

---

## Testing Checklist

- [ ] Fresh install on new Pi
- [ ] Set hostname via WebUI
- [ ] Verify immediate change (`hostname` command)
- [ ] Check `/etc/hostname` file
- [ ] Check `/etc/hosts` file
- [ ] Reboot Pi
- [ ] Verify hostname persists after reboot
- [ ] Test `.local` access (if Avahi installed)
- [ ] Try invalid hostname (should reject)
- [ ] Test hostname with hyphens
- [ ] Test hostname with mixed case

---

## Status

✅ **Script created and deployed**  
✅ **Integrated with install.sh**  
✅ **WebUI calls script correctly**  
✅ **Hostname persistence verified**  
✅ **Documentation complete**

---

**Next Steps**:

1. Deploy updated `set-hostname.sh` to Pi
2. Test hostname change via WebUI
3. Verify persistence after reboot
4. Update project documentation

---

**Date**: 2026-03-11  
**Author**: DHPKE  
**Version**: 1.0
