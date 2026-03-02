# Curtain Mode v7.0.0 - Usage Guide

## Overview

Curtain Mode creates two 3-pixel wide vertical bars on the left and right edges of the LED matrix display. These bars are group-based (1-8) and can be toggled on/off via boolean input.

## Display Layout

```
64×32 LED Matrix:
┌───┬─────────────────────────────────────────┬───┐
│ L │                                         │ R │
│ E │         Middle Area (58 pixels)         │ I │
│ F │         Segments 1-4 here               │ G │
│ T │         (unchanged positions)           │ H │
│   │                                         │ T │
│ 3 │                                         │ 3 │
│ p │                                         │ p │
│ x │                                         │ x │
└───┴─────────────────────────────────────────┴───┘
  0-2                                      61-63
```

- **Left curtain bar**: pixels 0-2 (3 pixels wide, full height)
- **Right curtain bar**: pixels 61-63 (3 pixels wide, full height)
- **Middle area**: pixels 3-60 (58 pixels wide) - segments stay as configured
- **Z-order**: Curtains render on top of segments, below group indicator

## UDP Commands

### 1. Configure Curtain for a Group

```json
{
  "cmd": "curtain",
  "group": 1,
  "enabled": true,
  "color": "FF0000"
}
```

**Parameters:**
- `group`: Group ID (1-8)
- `enabled`: Whether curtain is enabled for this group (true/false)
- `color`: Hex color code (e.g., "FF0000" for red, "FFFFFF" for white)

**Effect:** Configures the curtain for the specified group. The curtain will only be visible when explicitly triggered with `state:true`.

### 2. Toggle Curtain Visibility (Boolean Trigger)

```json
{
  "cmd": "curtain",
  "group": 1,
  "state": true
}
```

**Parameters:**
- `group`: Group ID (1-8)
- `state`: Visibility state (true = show, false = hide)

**Effect:** Immediately shows or hides the curtain bars for the specified group.

## Configuration Persistence

Curtain configuration is saved to `/var/lib/led-matrix/config.json` and persists across reboots:

```json
{
  "curtains": {
    "1": {
      "enabled": true,
      "visible": false,
      "color": [255, 0, 0]
    },
    "2": {
      "enabled": true,
      "visible": true,
      "color": [0, 255, 0]
    }
  }
}
```

## Behavior

### When Curtain is Active

1. **Configuration**: Set `enabled=true` and specify a color for a group
2. **Trigger**: Send `state=true` to make the curtain visible
3. **Display**: 3-pixel wide bars appear on left and right edges
4. **Segments**: Continue rendering in their configured positions
5. **Z-Order**: Curtains render on top of segments (but below group indicator)

### Group Independence

- Each group (1-8) has independent curtain configuration
- A panel assigned to group 3 only shows curtains configured for group 3
- Curtains from other groups are ignored

### Segment Behavior

**Important:** Segments do NOT automatically adjust when curtain mode is active. They continue using their configured x, y, width, height values.

**Example:**
- If segment 0 is configured as fullscreen (x=0, y=0, w=64, h=32), it will render under the curtain bars
- To reserve space for curtains, manually configure segments with x=3, w=58 when designing your layout

## Example Workflow

### Step 1: Configure Curtain for Group 1

```json
{
  "cmd": "curtain",
  "group": 1,
  "enabled": true,
  "color": "00FF00"
}
```

### Step 2: Assign Panel to Group 1

```json
{
  "cmd": "group",
  "value": 1
}
```

### Step 3: Show Curtain

```json
{
  "cmd": "curtain",
  "group": 1,
  "state": true
}
```

### Step 4: Hide Curtain

```json
{
  "cmd": "curtain",
  "group": 1,
  "state": false
}
```

## Node-RED Example

```javascript
// Configure red curtain for group 1
msg.payload = {
    "cmd": "curtain",
    "group": 1,
    "enabled": true,
    "color": "FF0000"
};

// Toggle curtain visibility based on input
msg.payload = {
    "cmd": "curtain",
    "group": 1,
    "state": msg.trigger  // true/false from input
};
```

## Q-SYS Designer Example

```lua
-- Configure curtain
json_cmd = {
    cmd = "curtain",
    group = 1,
    enabled = true,
    color = "00FF00"
}

-- Toggle curtain
json_cmd = {
    cmd = "curtain",
    group = 1,
    state = true  -- or false
}
```

## Deployment

### Install v7.0.0

```bash
# On the Raspberry Pi
cd /path/to/QSYS-LED-Matrix
git fetch
git checkout v7-curtain-mode
cd rpi
sudo systemctl restart led-matrix
```

### Rollback to v6 (if needed)

```bash
git checkout main
sudo systemctl restart led-matrix
```

## Notes

- **No automatic remapping**: Segments keep their configured positions
- **Design consideration**: When using curtain mode, design layouts with x≥3 and w≤58 to avoid overlap
- **Performance**: Curtain rendering adds minimal overhead (simple rectangle drawing)
- **Compatibility**: All existing v6 commands continue to work unchanged

## Version Info

- **Version**: 7.0.0
- **Branch**: `v7-curtain-mode`
- **Stable Branch**: `main` (v6.x)
- **Status**: Testing/Beta

---

**Tip:** For fullscreen segments with curtain mode, use layout configuration:
```json
{
  "cmd": "config",
  "seg": 0,
  "x": 3,
  "y": 0,
  "w": 58,
  "h": 32
}
```
