# Network Fallback IP - Troubleshooting & Fix

**Date**: 2026-03-11  
**Issue**: Fallback IP (10.10.10.99) not working properly on Raspberry Pi CM4  
**Status**: 🔧 Fixing

---

## Problem Analysis

The fallback IP configuration may fail for several reasons:

### Common Issues:

1. **NetworkManager not properly configured**
   - Connection settings not persisting
   - ipv4.may-fail not set correctly
   - Address priority issues

2. **dhcpcd conflicts**
   - dhcpcd and NetworkManager both trying to manage network
   - Configuration overriding each other

3. **Interface name mismatch**
   - Script looking for eth1, but Pi uses eth0
   - Connection name mismatch

4. **Timing issues**
   - NetworkManager not waiting for fallback to activate
   - DHCP timeout too short

---

## Solution 1: Fix NetworkManager (Recommended)

**Script**: `rpi/fix-fallback-ip.sh`

This script will:
1. Auto-detect ethernet interface (eth0/eth1/enp...)
2. Install NetworkManager if missing
3. Disable dhcpcd (if present)
4. Configure DHCP + static fallback properly
5. Verify configuration
6. Test connectivity

### Run on the Pi:

```bash
cd /opt/led-matrix/
# Copy script to Pi first, then:
sudo bash fix-fallback-ip.sh
```

### What it does:

```bash
# Sets DHCP as primary
nmcli connection modify "Wired connection 1" ipv4.method auto

# Adds static fallback IP (in addition to DHCP)
nmcli connection modify "Wired connection 1" +ipv4.addresses 10.10.10.99/24

# Ensures connection stays up even if DHCP fails
nmcli connection modify "Wired connection 1" ipv4.may-fail no

# Restarts connection
nmcli connection down "Wired connection 1"
nmcli connection up "Wired connection 1"
```

---

## Solution 2: Use systemd-networkd (Alternative)

**Script**: `rpi/fix-fallback-systemd.sh`

More reliable for embedded devices, less overhead.

### Run on the Pi:

```bash
cd /opt/led-matrix/
sudo bash fix-fallback-systemd.sh
```

### What it creates:

**File**: `/etc/systemd/network/10-eth0.network`

```ini
[Match]
Name=eth0

[Network]
DHCP=yes
Address=10.10.10.99/24
Gateway=10.10.10.1
DNS=8.8.8.8

[DHCP]
UseDomains=yes
RouteMetric=100
```

**Behavior**:
- Tries DHCP first
- Always assigns 10.10.10.99 as secondary IP
- Both IPs active when DHCP works
- Falls back to 10.10.10.99 when DHCP fails

---

## Manual Fix (If Scripts Fail)

### Check Current Setup:

```bash
# Show all connections
nmcli connection show

# Show connection details
nmcli connection show "Wired connection 1"

# Show current IPs
ip addr show eth0
```

### Manual Configuration:

```bash
# Get connection name
CONNECTION=$(nmcli -t -f NAME,DEVICE con show | grep ":eth0$" | cut -d: -f1)

# Configure properly
sudo nmcli con mod "$CONNECTION" ipv4.method auto
sudo nmcli con mod "$CONNECTION" ipv4.addresses ""
sudo nmcli con mod "$CONNECTION" +ipv4.addresses 10.10.10.99/24
sudo nmcli con mod "$CONNECTION" ipv4.gateway 10.10.10.1
sudo nmcli con mod "$CONNECTION" ipv4.dns "8.8.8.8 8.8.4.4"
sudo nmcli con mod "$CONNECTION" ipv4.may-fail no

# Restart
sudo nmcli con down "$CONNECTION"
sudo nmcli con up "$CONNECTION"
```

---

## Verification Steps

### 1. Check IPs are assigned:

```bash
ip addr show eth0 | grep "inet "
```

**Expected output**:
```
inet 10.10.10.99/24 brd 10.10.10.255 scope global noprefixroute eth0
inet 10.1.1.26/24 brd 10.1.1.255 scope global dynamic noprefixroute eth0
```

### 2. Test both IPs:

```bash
# Test fallback IP
ping -c 1 10.10.10.99

# Test DHCP IP (if assigned)
ping -c 1 10.1.1.26
```

### 3. Test from another machine:

```bash
# From your laptop/Mac:
ping 10.10.10.99
ssh user@10.10.10.99
curl http://10.10.10.99:8080/
```

### 4. Simulate DHCP failure:

```bash
# Disconnect ethernet, then reconnect
# Or disable DHCP server temporarily
# Fallback IP should remain accessible
```

---

## Persistent Configuration

### NetworkManager (Solution 1):

Configuration stored in:
```
/etc/NetworkManager/system-connections/
```

**Backup current config**:
```bash
sudo cp -r /etc/NetworkManager/system-connections/ ~/nm-backup/
```

### systemd-networkd (Solution 2):

Configuration stored in:
```
/etc/systemd/network/10-eth0.network
```

**Backup**:
```bash
sudo cp /etc/systemd/network/10-eth0.network ~/network-backup.network
```

---

## Common Pitfalls

### 1. Both dhcpcd and NetworkManager running

**Check**:
```bash
systemctl status dhcpcd
systemctl status NetworkManager
```

**Fix**: Disable one (prefer NetworkManager):
```bash
sudo systemctl stop dhcpcd
sudo systemctl disable dhcpcd
sudo systemctl enable NetworkManager
sudo systemctl start NetworkManager
```

### 2. Wrong interface name

**Check**:
```bash
ip link show
```

**Fix**: Update scripts/commands with correct interface (eth0/eth1/enp0s3)

### 3. Firewall blocking access

**Check**:
```bash
sudo iptables -L
```

**Fix**:
```bash
# Allow access to fallback IP
sudo iptables -I INPUT -d 10.10.10.99 -j ACCEPT
```

### 4. IPv6 interference

**Disable IPv6** (if causing issues):
```bash
sudo nmcli con mod "$CONNECTION" ipv6.method disabled
sudo nmcli con down "$CONNECTION"
sudo nmcli con up "$CONNECTION"
```

---

## Testing Procedure

### Full Test:

```bash
# 1. Verify configuration
nmcli connection show "Wired connection 1" | grep ipv4

# 2. Check both IPs are assigned
ip addr show eth0 | grep inet

# 3. Test fallback IP locally
ping -c 3 10.10.10.99

# 4. Test web server
curl http://10.10.10.99:8080/

# 5. Test from another machine
# (from laptop/Mac)
ping 10.10.10.99
ssh user@10.10.10.99

# 6. Reboot and re-test
sudo reboot
# Wait for boot, then repeat tests
```

---

## Deployment Script Update

Update `rpi/install.sh` to use the fixed configuration:

**Add to install.sh** (after NetworkManager installation):

```bash
# Configure fallback IP properly
echo "Configuring network fallback IP..."
CONNECTION=$(nmcli -t -f NAME,DEVICE con show | grep ":$INTERFACE$" | cut -d: -f1)
sudo nmcli con mod "$CONNECTION" ipv4.method auto
sudo nmcli con mod "$CONNECTION" ipv4.addresses ""
sudo nmcli con mod "$CONNECTION" +ipv4.addresses 10.10.10.99/24
sudo nmcli con mod "$CONNECTION" ipv4.gateway 10.10.10.1
sudo nmcli con mod "$CONNECTION" ipv4.dns "8.8.8.8 8.8.4.4"
sudo nmcli con mod "$CONNECTION" ipv4.may-fail no
sudo nmcli con down "$CONNECTION" && sudo nmcli con up "$CONNECTION"
```

---

## Expected Behavior

### ✅ With DHCP Server:
- Primary IP: 10.1.1.x (from DHCP)
- Secondary IP: 10.10.10.99 (static fallback)
- Both IPs respond to ping
- Services accessible on both IPs

### ✅ Without DHCP Server:
- Primary IP: 10.10.10.99 (fallback)
- Services accessible on 10.10.10.99
- No DHCP timeout errors

### ✅ After Reboot:
- Configuration persists
- IPs assigned automatically
- No manual intervention needed

---

## Files Created

1. **fix-fallback-ip.sh** - NetworkManager fix (auto-detects interface)
2. **fix-fallback-systemd.sh** - Alternative using systemd-networkd
3. **NETWORK-FALLBACK-FIX.md** - This troubleshooting guide

---

## Next Steps

1. **Deploy fix script to Pi**:
   ```bash
   scp rpi/fix-fallback-ip.sh user@10.1.1.26:/tmp/
   ssh user@10.1.1.26
   sudo bash /tmp/fix-fallback-ip.sh
   ```

2. **Verify it works**:
   ```bash
   ping 10.10.10.99
   ```

3. **Test reboot persistence**:
   ```bash
   sudo reboot
   # Wait 30 seconds
   ping 10.10.10.99
   ```

4. **Update install.sh** with the fix

5. **Document in README.md**

---

**Status**: 🔧 Scripts ready for deployment  
**Priority**: High (network reliability critical)  
**Impact**: Ensures Pi is always accessible at 10.10.10.99
