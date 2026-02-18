# Rock Pi S - Bus Error Fix Deployment Guide

## Problem
The `rpi-rgb-led-matrix` library crashes with **bus error (signal 7)** on Rock Pi S because it tries to detect Raspberry Pi hardware.

## Solution Applied
Updated `main.py` with three critical fixes:
1. `options.drop_privileges = False` - Disables Pi model detection
2. Explicit GPIO pin assignments (all 13 pins)
3. Hardware pulsing already disabled

## Deployment Methods

### Method 1: Push from Development Machine (RECOMMENDED)

From your local machine in the repository:

```bash
cd /Users/user/.openclaw/workspace/QSYS-LED-Matrix/rockpis
./push-to-rockpi.sh <rockpi-ip-address>
```

Example:
```bash
./push-to-rockpi.sh 192.168.1.100
```

This will automatically:
- Copy all updated Python files
- Restart the service
- Show the service status

### Method 2: Manual SCP (Quick Update)

```bash
cd /Users/user/.openclaw/workspace/QSYS-LED-Matrix/rockpis
scp main.py root@<rockpi-ip>:/opt/led-matrix/
ssh root@<rockpi-ip> "systemctl restart led-matrix"
```

### Method 3: Update on Device (If SSH'd In)

If you're already logged into the Rock Pi S:

```bash
# On the Rock Pi S
cd ~/QSYS-LED-Matrix
git pull origin main
cd rockpis
sudo ./update-on-device.sh
```

## Verify the Fix

After deploying, check the logs:

```bash
ssh root@<rockpi-ip>
sudo journalctl -u led-matrix -f
```

### Expected Output (SUCCESS):
```
20:05:00 [INFO] ==================================================
20:05:00 [INFO] RADXA Rock Pi S — LED Matrix Controller
20:05:00 [INFO] ==================================================
20:05:00 [INFO] Matrix: 64×32, chain=1
20:05:00 [INFO] UDP port: 21324,  Web port: 80
20:05:00 [INFO] HW pulsing disabled: True  (expected True on RK3308)
20:05:00 [INFO] ✓ LED matrix initialised
20:05:00 [INFO]   ↳ Hardware pulsing disabled (bit-bang OE- mode for RK3308)
20:05:00 [INFO]   ↳ Privilege drop disabled (non-Pi hardware mode)
20:05:00 [INFO]   ↳ GPIO pins: R1=16 G1=17 B1=18 CLK=22
20:05:00 [INFO] [UDP] Listening on 0.0.0.0:21324
20:05:00 [INFO] [WEB] HTTP server started on port 80
20:05:00 [INFO] ==================================================
20:05:00 [INFO] System ready — press Ctrl+C to stop
20:05:00 [INFO] ==================================================
```

### Old Output (FAILURE - before fix):
```
20:03:43 [INFO] HW pulsing disabled: True  (expected True on RK3308)
non-existent Revision: Could not determine Pi model
Failed to read revision from /proc/device-tree/system/linux,revision
Unknown Revision: Could not determine Pi model
systemd[1]: led-matrix.service: Main process exited, code=killed, status=7/BUS
```

## Troubleshooting

### Still getting bus error after update?

1. **Verify the file was actually copied:**
   ```bash
   ssh root@<rockpi-ip> "grep 'drop_privileges' /opt/led-matrix/main.py"
   ```
   Should return: `options.drop_privileges = False`

2. **Check file timestamp:**
   ```bash
   ssh root@<rockpi-ip> "ls -la /opt/led-matrix/main.py"
   ```

3. **Force service restart:**
   ```bash
   ssh root@<rockpi-ip> "systemctl stop led-matrix && systemctl start led-matrix"
   ```

4. **Try manual run to see full error:**
   ```bash
   ssh root@<rockpi-ip>
   systemctl stop led-matrix
   cd /opt/led-matrix
   sudo python3 main.py
   ```

## Quick Commands

```bash
# Check service status
ssh root@<rockpi-ip> "systemctl status led-matrix"

# View live logs
ssh root@<rockpi-ip> "journalctl -u led-matrix -f"

# Restart service
ssh root@<rockpi-ip> "systemctl restart led-matrix"

# Stop service
ssh root@<rockpi-ip> "systemctl stop led-matrix"
```

## Files Updated
- `rockpis/main.py` - Core fix applied
- `rockpis/README.md` - Documentation updated
- `rockpis/update-on-device.sh` - New deployment helper
- `rockpis/push-to-rockpi.sh` - New remote deployment helper
