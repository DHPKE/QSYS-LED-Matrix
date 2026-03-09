# Network Fallback IP Configuration

**Date**: 2026-03-09 20:23 CET  
**Pi IP**: 10.1.1.26 (DHCP) + 10.10.10.99/24 (fallback static)  
**Status**: ✅ Configured

## Configuration

**Network Manager** (active on Raspberry Pi CM4)

### Connection Details
```
Connection: "Wired connection 1"
Interface: eth0
Method: auto (DHCP with static fallback)
```

### IP Addresses
- **Primary**: DHCP (currently 10.1.1.26/24)
- **Fallback**: 10.10.10.99/24 (static)

### How It Works
1. **DHCP available**: Gets IP from DHCP server (10.1.1.x)
2. **DHCP fails**: Falls back to 10.10.10.99/24
3. **Both IPs active**: When DHCP works, both IPs respond

### Configuration Commands

**Set fallback IP**:
```bash
sudo nmcli connection modify "Wired connection 1" ipv4.method auto
sudo nmcli connection modify "Wired connection 1" +ipv4.addresses 10.10.10.99/24
sudo nmcli connection modify "Wired connection 1" ipv4.may-fail no
sudo nmcli connection down "Wired connection 1"
sudo nmcli connection up "Wired connection 1"
```

**Remove fallback IP**:
```bash
sudo nmcli connection modify "Wired connection 1" -ipv4.addresses 10.10.10.99/24
sudo nmcli connection down "Wired connection 1"
sudo nmcli connection up "Wired connection 1"
```

**Change fallback IP**:
```bash
# Remove old
sudo nmcli connection modify "Wired connection 1" -ipv4.addresses <old-ip>/24
# Add new
sudo nmcli connection modify "Wired connection 1" +ipv4.addresses <new-ip>/24
# Restart connection
sudo nmcli connection down "Wired connection 1" && sudo nmcli connection up "Wired connection 1"
```

### Verification

**Check current IPs**:
```bash
ip addr show eth0 | grep "inet "
```

**Expected output**:
```
inet 10.10.10.99/24 brd 10.10.10.255 scope global noprefixroute eth0
inet 10.1.1.26/24 brd 10.1.1.255 scope global dynamic noprefixroute eth0
```

**Test connectivity**:
```bash
ping 10.10.10.99    # Should always work
ping 10.1.1.26      # Works when DHCP active
```

### Q-SYS Plugin Integration

**Default IP in plugin**: 192.168.1.100 (original)

**Recommended**: Update plugin default to 10.10.10.99 for automatic fallback.

**Or**: Users can manually enter 10.10.10.99 in the plugin IP field.

### Use Cases

1. **Production network with DHCP**: Uses 10.1.1.26 (dynamic)
2. **Direct connection (no DHCP)**: Uses 10.10.10.99 (static)
3. **Troubleshooting**: Always accessible at 10.10.10.99
4. **Network migration**: Fallback IP remains constant

### Persistence

NetworkManager stores configuration in:
```
/etc/NetworkManager/system-connections/
```

Configuration survives reboots and updates.

---

**Status**: ✅ Fallback IP 10.10.10.99/24 configured and tested
