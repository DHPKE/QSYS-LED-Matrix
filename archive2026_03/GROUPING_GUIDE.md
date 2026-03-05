# LED Matrix Group Configuration Guide

## Overview

The LED Matrix system now supports grouping multiple panels for organized control. Each panel can be assigned to one of 8 groups, allowing you to send commands to specific panels or broadcast to all panels simultaneously.

## Visual Indicator

Each panel displays a small 4×4 pixel colored square in the **bottom-left corner** to identify its group assignment:

| Group | Color   | RGB           |
|-------|---------|---------------|
| 0     | None    | (invisible)   |
| 1     | White   | 255, 255, 255 |
| 2     | Yellow  | 255, 255, 0   |
| 3     | Orange  | 255, 165, 0   |
| 4     | Red     | 255, 0, 0     |
| 5     | Magenta | 255, 0, 255   |
| 6     | Blue    | 0, 0, 255     |
| 7     | Cyan    | 0, 255, 255   |
| 8     | Green   | 0, 255, 0     |

## Configuration

### RPi Panel Configuration

#### Method 1: Web UI

1. Open the panel's web interface (http://panel-ip:8080)
2. Scroll to "Display Settings"
3. Click the desired group button (None, 1-8)
4. The group indicator will appear immediately in the bottom-left corner

#### Method 2: UDP Command

Send a JSON command to the panel:

```json
{"cmd":"group","value":3}
```

This assigns the panel to group 3 (Orange indicator).

#### Method 3: Edit config.py

Edit `/rpi/config.py` and set:

```python
GROUP_ID = 3  # 0 = no grouping, 1-8 = assigned group
```

Then restart the LED matrix service.

### Configuration File Location

Group assignments are persisted in:
```
/var/lib/led-matrix/config.json
```

## Q-SYS Plugin (v5.0.0)

The new LEDMatrix_v5.qplug includes group routing controls.

### Features

1. **Group Selection Buttons**: Nine toggle buttons (All, 1-8) to select the target group
2. **Active Group Display**: Shows which group is currently selected
3. **Automatic Routing**: All commands are automatically routed to the selected group

### Usage

1. **Load the Plugin**: Use `LEDMatrix_v5.qplug` in your Q-SYS design
2. **Select Target Group**: Click one of the group buttons (All, 1, 2, ..., 8)
3. **Send Commands**: All text, brightness, layout, and orientation commands will only affect panels in the selected group
4. **Broadcast Mode**: Select "All" to send commands to all panels regardless of their group assignment

### Command Protocol

All UDP commands now include an optional `group` field:

```json
{
  "cmd": "text",
  "seg": 0,
  "group": 3,
  "text": "Hello Group 3",
  "color": "FFFFFF",
  "bgcolor": "000000",
  "font": "arial",
  "size": "auto",
  "align": "C",
  "effect": "none",
  "intensity": 255
}
```

**Group field values:**
- `0` or omitted = Broadcast to all panels
- `1-8` = Only panels assigned to that specific group will execute the command

## Example Use Cases

### Scenario 1: Lobby Display System

- **Groups 1-4**: Four zones (North, South, East, West)
- **QSYS Setup**: 
  - Select Group 1 → Update North zone displays
  - Select Group 2 → Update South zone displays
  - Select "All" → Update all lobby displays simultaneously

### Scenario 2: Multi-Room Conference System

- **Group 1**: Main auditorium displays
- **Group 2**: Breakout Room A displays
- **Group 3**: Breakout Room B displays
- **Group 4**: Lobby/Common area displays
- **QSYS Setup**: Route appropriate messages to each room's group

### Scenario 3: Stage/Production

- **Groups 1-8**: Different stage positions or talent monitor groups
- **QSYS Setup**: Update cue displays for specific performers or stage areas

## Technical Details

### Filtering Logic

Panels filter incoming commands based on:

```
IF command.group == 0 THEN execute (broadcast)
ELSE IF command.group == panel.group_id THEN execute
ELSE ignore
```

### Performance

- Group filtering happens before command processing (minimal overhead)
- Visual indicator is rendered on every frame (no performance impact)
- Group assignment persists across reboots

## Troubleshooting

### Indicator Not Visible

- Check that `GROUP_ID` is set to 1-8 (0 disables the indicator)
- Verify the group color isn't the same as your background
- Confirm `GROUP_INDICATOR_SIZE` is set to at least 3 in config.py

### Commands Not Reaching Panel

- Verify panel group assignment matches the command's group field
- Check UDP port and IP address configuration
- Use group 0 (broadcast) for testing

### Multiple Panels Responding

- Each panel should have a unique IP address
- Verify each panel has the correct group assignment
- Use Q-SYS plugin's group selector to target specific groups

## Migration from v4.0

Existing designs using LEDMatrix_v4.qplug will continue to work (commands without a group field are broadcast to all panels). To adopt grouping:

1. Configure each panel's group ID (Web UI or config.py)
2. Replace v4 plugin with v5 in your Q-SYS design
3. Use group selection buttons to target specific panels

## Files Modified

**RPi Code:**
- `/rpi/config.py` - Added GROUP_ID and GROUP_COLORS
- `/rpi/udp_handler.py` - Added group filtering and get/set functions
- `/rpi/text_renderer.py` - Added group indicator rendering
- `/rpi/web_server.py` - Added /api/group endpoints

**Q-SYS Plugin:**
- `/qsys-plugin/LEDMatrix_v5.qplug` - New plugin with group controls

## Summary

The grouping function enables centralized control of multiple LED panels through a single Q-SYS plugin. Each panel displays its group assignment, and commands are automatically routed to the appropriate panels based on the selected group.
