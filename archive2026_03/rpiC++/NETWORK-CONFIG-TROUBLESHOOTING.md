# Network Configuration Troubleshooting

## Issue: Permission Denied Error

If you see this error in the WebUI:
```
Error: [Errno 13] Permission denied: '/etc/dhcpcd.conf'
```

Or this error:
```
Error: Failed to configure static IP: sudo: a terminal is required to read the password
```

### Cause
The LED Matrix service needs sudo permissions to run the network configuration script.

### Solution

The service runs as the `daemon` user. Ensure the sudoers entry is correct:

```bash
# Check current sudoers entry
sudo cat /etc/sudoers.d/led-matrix

# Should show:
daemon ALL=(ALL) NOPASSWD: /opt/led-matrix/network-config.sh
```

If it shows `node` instead of `daemon`, update it:

```bash
echo "daemon ALL=(ALL) NOPASSWD: /opt/led-matrix/network-config.sh" | sudo tee /etc/sudoers.d/led-matrix
sudo chmod 0440 /etc/sudoers.d/led-matrix
sudo visudo -c -f /etc/sudoers.d/led-matrix
```

### Verify Service User

Check which user the service is running as:

```bash
ps aux | grep led-matrix | grep python
```

You should see:
```
daemon      1365  ... /usr/bin/python3 /opt/led-matrix/main.py
```

### Test Manually

Test the network configuration script as the daemon user:

```bash
# Test DHCP
sudo -u daemon sudo /opt/led-matrix/network-config.sh dhcp

# Test static IP (won't actually apply without reboot)
sudo -u daemon sudo /opt/led-matrix/network-config.sh static 10.20.30.40 24
```

If these work without asking for a password, the WebUI will work correctly.

---

## Automatic Fix

Re-run the deployment script to fix permissions:

```bash
cd QSYS-LED-Matrix/rpi
./deploy-network-helper.sh 10.1.1.24
```

This will:
1. Install the helper script
2. Set correct permissions
3. Create/update sudoers entry for `daemon` user
4. Restart the service

---

## Security Notes

- The sudoers entry is specific to ONE script only (`/opt/led-matrix/network-config.sh`)
- The script only modifies network configuration (no arbitrary command execution)
- The script is owned by root and cannot be modified by unprivileged users
- Follows the principle of least privilege
