# Pi Troubleshooting Guide

## Issue: UDP Commands Not Working

### Symptoms
- Q-SYS plugin shows commands sent but no response
- Pi service running but not responding
- Large UDP receive buffer (`ss -ulnp` shows high UNCONN value)

### Diagnosis
```bash
# Check if service is running
sudo systemctl status led-matrix

# Check UDP port and buffer
sudo ss -ulnp | grep 21324

# If UNCONN buffer is >100KB, service is likely stuck
```

### Fix
```bash
# Kill and restart the service
sudo kill -9 $(pgrep led-matrix)
sudo systemctl start led-matrix

# Verify it's running
ps aux | grep led-matrix
```

### Root Cause
The UDP receive loop can get stuck if:
1. Too many commands sent too quickly
2. JSON parsing exception not properly handled
3. Main loop blocked during intensive operations

### Prevention
- Avoid sending >10 commands per second
- Add delays between rapid command sequences in Q-SYS
- Monitor UDP buffer: large buffer = service needs restart

## Testing Commands
```bash
# Test brightness
echo '{"cmd":"brightness","value":100}' | nc -u 10.1.1.99 21324

# Test text
echo '{"cmd":"text","seg":0,"text":"TEST","color":"FFFFFF","bgcolor":"000000"}' | nc -u 10.1.1.99 21324

# Check logs
sudo journalctl -u led-matrix --since '1 minute ago' --no-pager | tail -20
```

## Current Status
✅ Service working after restart (PID 1386)
✅ UDP commands being received and processed
✅ All features functional (rotation, layouts, text, brightness)
