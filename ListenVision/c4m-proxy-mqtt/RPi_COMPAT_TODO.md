# C4M Proxy MQTT - RPi Feature Compatibility Update

## Missing Commands to Add

Based on RPi UDP implementation, these commands need to be added to the MQTT proxy:

### 1. `frame` command - Segment borders
```javascript
// Topic: led/display/42/cmd/frame
// Payload: { "seg": 0, "enabled": true, "color": "FFFFFF", "width": 2 }
```

### 2. `curtain` command - Edge bars (3px wide on left/right edges)
```javascript
// Topic: led/display/42/cmd/curtain  
// Payload: { "group": 1, "enabled": true, "color": "FF0000" }
```

### 3. `group` command - Assign device to group
```javascript
// Topic: led/display/42/cmd/group
// Payload: { "value": 3 }
```

### 4. `display` command - Enable/disable display  
```javascript
// Topic: led/display/42/cmd/display
// Payload: { "enabled": true }
```

### 5. `orientation` command - Landscape/portrait
```javascript
// Topic: led/display/42/cmd/orientation
// Payload: { "value": "landscape" } // or "portrait"
```

### 6. `config` command - Manual segment positioning
```javascript
// Topic: led/display/42/cmd/config
// Payload: { "seg": 0, "x": 0, "y": 0, "w": 64, "h": 16 }
```

### 7. Enhanced `segment` command - Add missing fields
```javascript
// Current payload: { seg, enabled, text, color, effect, align }
// Add: bgcolor, font, size, intensity
{
  "seg": 0,
  "enabled": true,
  "text": "Hello",
  "color": "FF0000",
  "bgcolor": "000000",  // NEW
  "font": "arial",      // NEW
  "size": "auto",       // NEW (auto, small, medium, large)
  "intensity": 255,     // NEW (0-255)
  "effect": 1,
  "align": 1
}
```

## Implementation Notes

### Curtain Mode
- 3-pixel wide vertical bars on left (x=0-2) and right (x=61-63) edges
- Per-group configuration (groups 1-8)
- Tracks both configuration (color) and visibility (enabled)
- When `enabled=false`, hides bars but keeps config
- When `enabled=true`, shows bars with configured color

### Group Routing
- Groups 0-8 (0 = broadcast to all)
- Devices can be assigned to specific groups via `group` command
- Commands with `group` field filter to matching devices
- Group assignment should persist in config.json

### Frame Mode  
- Optional border around segments
- Configurable color and width (1-3 pixels)
- Per-segment enable/disable

### Display Enable/Disable
- Blanks entire display when disabled
- Preserves state (re-renders when re-enabled)
- Useful for "lights out" mode without disconnecting

## Q-SYS Plugin Updates Needed

The plugin also needs these commands added:

1. **Frame controls** (per segment):
   - Frame enable toggle
   - Frame color selector
   - Frame width (1-3px)

2. **Curtain controls** (global):
   - Curtain group (1-8)
   - Curtain color
   - Curtain enable toggle
   - Curtain apply button

3. **Enhanced segment properties**:
   - Background color (bgcolor)
   - Font selection (arial, digital)
   - Size mode (auto, small, medium, large)
   - Intensity (0-255)

4. **Display enable** toggle (global)

5. **Orientation** selector (landscape/portrait)

## Testing Required

After implementing, test these scenarios:

1. ✅ Set segment text with all properties
2. ✅ Apply layout preset
3. ✅ Set brightness
4. ✅ Rotate display
5. ✅ Clear segment / clear all
6. ✅ Test pattern
7. ⚠️ **Curtain mode** - configure and toggle per group
8. ⚠️ **Frame mode** - borders around segments  
9. ⚠️ **Group assignment** - device joins group  
10. ⚠️ **Display on/off** - blank without disconnect
11. ⚠️ **Orientation** - switch landscape/portrait
12. ⚠️ **Manual config** - custom segment positioning

Items marked ⚠️ are NOT YET IMPLEMENTED.

## Compatibility

After these updates, the MQTT proxy will support 100% of RPi UDP protocol features, ensuring drop-in compatibility with existing Q-SYS designs.
