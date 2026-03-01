# Helper Scripts

The LED Matrix service runs as the `daemon` user (rgbmatrix library drops privileges). To perform privileged operations like changing hostname, configuring network, or rebooting, the web server calls these helper scripts via sudo.

## Overview

| Script | Purpose | Called By | Sudoers Rule |
|--------|---------|-----------|--------------|
| `configure-network.sh` | Configure static IP or DHCP via NetworkManager | `web_server.py` | `daemon ALL=(ALL) NOPASSWD` |
| `set-hostname.sh` | Change system hostname | `web_server.py` | `daemon ALL=(root) NOPASSWD` |
| `reboot-device.sh` | Reboot the system | `web_server.py` | `daemon ALL=(root) NOPASSWD` |

## Installation

All scripts are automatically installed by `install.sh`:

1. Copied to `/opt/led-matrix/`
2. Made executable (`chmod +x`)
3. Sudoers rules added to `/etc/sudoers.d/led-matrix`

## Manual Installation

If you need to install or update scripts manually:

```bash
# Copy scripts
sudo cp configure-network.sh /opt/led-matrix/
sudo cp set-hostname.sh /opt/led-matrix/
sudo cp reboot-device.sh /opt/led-matrix/

# Make executable
sudo chmod +x /opt/led-matrix/*.sh

# Configure sudoers (IMPORTANT: use visudo or tee to avoid syntax errors)
cat <<'EOF' | sudo tee /etc/sudoers.d/led-matrix > /dev/null
daemon ALL=(ALL) NOPASSWD: /opt/led-matrix/configure-network.sh
daemon ALL=(root) NOPASSWD: /opt/led-matrix/set-hostname.sh
daemon ALL=(root) NOPASSWD: /opt/led-matrix/reboot-device.sh
EOF
sudo chmod 0440 /etc/sudoers.d/led-matrix

# Restart service
sudo systemctl restart led-matrix
```

## Script Details

### configure-network.sh

Configures network settings via NetworkManager (nmcli).

**Usage:**
```bash
sudo /opt/led-matrix/configure-network.sh dhcp
sudo /opt/led-matrix/configure-network.sh static <ip> <netmask> <gateway> <dns>
```

**Example:**
```bash
# Enable DHCP
sudo /opt/led-matrix/configure-network.sh dhcp

# Set static IP
sudo /opt/led-matrix/configure-network.sh static 10.1.1.25 255.255.255.0 10.1.1.1 10.1.1.1
```

**How it works:**
1. Auto-detects NetworkManager connection name for eth1 device
2. Uses `nmcli con mod` to update connection settings
3. Sets `ipv4.method` to `auto` (DHCP) or `manual` (static)
4. Requires reboot to apply changes

**Web API endpoint:** `POST /api/config`

---

### set-hostname.sh

Changes the system hostname.

**Usage:**
```bash
sudo /opt/led-matrix/set-hostname.sh <new-hostname>
```

**Example:**
```bash
sudo /opt/led-matrix/set-hostname.sh ledmatrixCM4
```

**How it works:**
1. Writes new hostname to `/etc/hostname`
2. Updates running hostname with `/usr/bin/hostname`
3. Changes apply immediately (no reboot needed)

**Web API endpoint:** `POST /api/config`

---

### reboot-device.sh

Reboots the system.

**Usage:**
```bash
sudo /opt/led-matrix/reboot-device.sh
```

**How it works:**
1. Calls `/usr/sbin/reboot` with sudo
2. 1-second delay allows HTTP response to complete before reboot

**Web API endpoint:** `POST /api/reboot`

## Security

### Why Sudoers Rules?

The rgbmatrix library drops privileges to the `daemon` user for security:
- Prevents accidental system damage from LED matrix code
- Follows principle of least privilege
- But blocks legitimate admin operations (hostname, network, reboot)

Sudoers rules allow specific scripts to run with elevated privileges:
- `NOPASSWD` = no password prompt needed
- Limited to specific script paths (can't run arbitrary commands)
- Daemon user can't modify the scripts themselves (owned by root)

### Validation

Each script includes input validation:
- **configure-network.sh:** Checks IP format, validates CIDR
- **set-hostname.sh:** Sanitizes hostname (alphanumeric + hyphen only)
- **reboot-device.sh:** No user input

### D-Bus Authentication

Why not use `systemctl` or `hostnamectl`?

These commands require D-Bus authentication even from the daemon user. Direct file writes + helper scripts are more reliable:
- `/etc/hostname` + `/usr/bin/hostname` (instead of `hostnamectl`)
- `/usr/sbin/reboot` (instead of `systemctl reboot`)
- `nmcli` (instead of manual `/etc/network/interfaces`)

## Troubleshooting

### "command not allowed" in logs

**Symptom:** Sudoers rule not working, logs show "daemon : command not allowed"

**Fix:**
```bash
# Check sudoers file exists
ls -la /etc/sudoers.d/led-matrix

# Check permissions (must be 0440)
sudo chmod 0440 /etc/sudoers.d/led-matrix

# Verify syntax (should print file contents if valid)
sudo visudo -cf /etc/sudoers.d/led-matrix

# Recreate if corrupted
cat <<'EOF' | sudo tee /etc/sudoers.d/led-matrix > /dev/null
daemon ALL=(ALL) NOPASSWD: /opt/led-matrix/configure-network.sh
daemon ALL=(root) NOPASSWD: /opt/led-matrix/set-hostname.sh
daemon ALL=(root) NOPASSWD: /opt/led-matrix/reboot-device.sh
EOF
sudo chmod 0440 /etc/sudoers.d/led-matrix
```

### Network config not persisting

**Symptom:** Static IP reverts to DHCP after reboot

**Cause:** System uses NetworkManager, not `/etc/network/interfaces`

**Fix:**
```bash
# Check NetworkManager is active
systemctl status NetworkManager

# Find connection name for eth1
nmcli con show | grep eth1

# Manually configure static IP
nmcli con mod "CONNECTION_NAME" ipv4.method manual
nmcli con mod "CONNECTION_NAME" ipv4.addresses "10.1.1.25/24"
nmcli con mod "CONNECTION_NAME" ipv4.gateway "10.1.1.1"
nmcli con mod "CONNECTION_NAME" ipv4.dns "10.1.1.1"

# Reboot
sudo reboot
```

### Hostname not changing

**Symptom:** WebUI shows success but hostname unchanged

**Cause:** Script not executable or sudoers rule missing

**Fix:**
```bash
# Make script executable
sudo chmod +x /opt/led-matrix/set-hostname.sh

# Test manually
sudo /opt/led-matrix/set-hostname.sh TestHostname
hostname  # Should show TestHostname

# Check service logs
sudo journalctl -u led-matrix -n 50
```

### Reboot button not working

**Symptom:** Button shows success but Pi doesn't reboot

**Cause:** Script not executable or sudoers rule missing

**Fix:**
```bash
# Make script executable
sudo chmod +x /opt/led-matrix/reboot-device.sh

# Test manually
sudo /opt/led-matrix/reboot-device.sh
# Pi should reboot immediately
```

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-03-01 | Initial helper scripts documentation |
| 1.1 | 2026-03-01 | Added configure-network.sh (NetworkManager) |
| 1.2 | 2026-03-01 | All scripts tested and working |

## Related Files

- `/etc/sudoers.d/led-matrix` - Sudoers rules
- `/opt/led-matrix/main.py` - Main service (calls scripts)
- `/opt/led-matrix/web_server.py` - Web API (calls scripts)
- `/etc/systemd/system/led-matrix.service` - Service configuration
- `/var/lib/led-matrix/config.json` - LED matrix config (rotation, group)

## API Endpoints

| Endpoint | Method | Body | Helper Script |
|----------|--------|------|---------------|
| `/api/config` | POST | `{"hostname":"...","mode":"dhcp\|static",...}` | `set-hostname.sh`, `configure-network.sh` |
| `/api/reboot` | POST | Empty | `reboot-device.sh` |
| `/api/testmode` | POST | Empty | None (sets `/tmp/led-matrix-testmode`) |

## See Also

- `FRESH-INSTALL-GUIDE.md` - Full installation instructions
- `FEATURE-PLAN.md` - Planned features and roadmap
- `install.sh` - Automated installation script
