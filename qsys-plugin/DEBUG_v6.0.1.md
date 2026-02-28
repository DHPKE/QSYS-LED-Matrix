# Q-SYS Plugin Debug Build v6.0.1-debug

## Changes from v6.0.0

### Comprehensive Logging Added
- **Command tracking**: Each command gets unique ID (#1, #2, #3...)
- **Socket state monitoring**: Logs socket open/close/status
- **Error tracking**: Counts errors and tracks last error message
- **Group selection logging**: Detailed group button event tracking
- **Stats reporting**: Periodic summary every 10 seconds

### Auto-Recovery Features
- **Auto-reconnect on error**: Reconnects socket after 2+ consecutive errors
- **Health check timer**: Runs every 10 seconds, forces reconnect if 5+ errors
- **Error counter**: Tracks consecutive errors, resets on success

### Logging Output Format
```
[INIT] Target IP: 10.1.1.99
[INIT] Target Port: 21324
[SOCKET] Opening UDP socket...
✓ [SOCKET] UDP socket ready -> 10.1.1.99:21324
[CMD #1] Preparing to send: {"cmd":"text","seg":0,...}
[CMD #1] Socket state: OK
[CMD #1] Calling socket:Send()...
[CMD #1] socket:Send() completed
✓ [CMD #1 SUCCESS] Sent to 10.1.1.99:21324
[GROUP] Button 1 event: Boolean=true, updatingGroups=false
[GROUP] Activating group 1
[STATS] Commands: 50 | Errors: 0 | Last Error: none
```

### Error Output Format
```
✗ [CMD #5 ERROR #1] attempt to call a nil value
[CMD #5] Multiple errors detected, forcing socket reconnect...
[SOCKET] Reopening socket (IP: 10.1.1.99, Port: 21324)
[HEALTH CHECK] Error count high (5), forcing reconnect...
```

## How to Use

1. **Replace existing plugin** in Q-SYS Designer with v6.0.1-debug
2. **Open Debug Output** window in Q-SYS Designer (View → Debug Output)
3. **Test commands** - watch for patterns in errors
4. **Share logs** - copy relevant error messages for troubleshooting

## What to Look For

### If commands stop working after N commands:
- Look for error pattern (e.g., "ERROR #3", "ERROR #4")
- Check if socket state changes from "OK" to something else
- See if auto-reconnect attempts succeed

### If socket closes unexpectedly:
- Look for "[SOCKET] Closing existing socket..."
- Check if "[SOCKET ERROR]" appears
- Watch for reconnection attempts

### If group buttons cause issues:
- Look for "[GROUP]" log entries
- Check if "updatingGroups" flag gets stuck
- See if group commands succeed

## Known Issues to Watch

1. **Q-SYS UDP socket limits**: Q-SYS may have internal socket pooling limits
2. **Rapid command bursts**: Sending too many commands too fast may overflow buffers
3. **Socket lifecycle**: Q-SYS might garbage-collect idle sockets

## Reverting to Stable

If debug logging causes performance issues, revert to v6.0.0 (non-debug).
