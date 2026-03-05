# LED Matrix v6.1.0 - Full Rotation Support

## 🎉 Complete Implementation

**Version:** 6.1.0  
**Date:** March 1, 2026  
**Status:** ✅ Production Ready

## Features

### 4-Way Rotation Support
- **0°** - Normal landscape orientation
- **90°** - Portrait (clockwise)
- **180°** - Inverted landscape (upside-down)
- **270°** - Portrait (counter-clockwise)

### Automatic Layout Adaptation
- Layouts automatically switch based on rotation
- 0°/180° use 64×32 coordinate system
- 90°/270° use 32×64 coordinate system
- Autofont recalculates for rotated dimensions

### Q-SYS Plugin v6.1.0
- Clean, simple rotation control
- Dropdown: 0°, 90°, 180°, 270°
- "Apply Rotation" button
- Removed deprecated orientation control
- User prompted to restart panel via Web UI

## How to Use

### From Q-SYS Plugin

1. Select desired rotation from dropdown (0°, 90°, 180°, 270°)
2. Click "Apply Rotation" button
3. Restart panel using Web UI reboot button (http://PANEL_IP:8080)
4. Rotation takes effect on restart

### UDP Command

```json
{"cmd":"rotation","value":90}
```

Values: 0, 90, 180, 270

**Note:** Restart required for rotation to take effect.

## Technical Details

### Matrix Initialization
```cpp
// main.cpp - Rotation applied at init
if (config_rotation == 90) {
    matrix_options.pixel_mapper_config = "Rotate:90";
} else if (config_rotation == 180) {
    matrix_options.pixel_mapper_config = "Rotate:180";
} else if (config_rotation == 270) {
    matrix_options.pixel_mapper_config = "Rotate:270";
}
```

### Layout Selection
```cpp
// udp_handler.cpp - applyLayout()
bool use_portrait_layout = (rotation_ == ROTATION_90 || rotation_ == ROTATION_270);

if (use_portrait_layout) {
    zones = &LAYOUT_PORTRAIT[preset];  // 32×64
} else {
    zones = &LAYOUT_LANDSCAPE[preset]; // 64×32
}
```

### Persistence
- Config stored in: `/var/lib/led-matrix/config.json`
- Survives reboots
- Example:
```json
{
  "brightness": 128,
  "group_id": 0,
  "rotation": 90
}
```

## Migration from Orientation

### Old Command (Deprecated)
```json
{"cmd":"orientation","value":"landscape"}  // 0°
{"cmd":"orientation","value":"portrait"}   // 90°
```

### New Command
```json
{"cmd":"rotation","value":0}    // landscape
{"cmd":"rotation","value":90}   // portrait
{"cmd":"rotation","value":180}  // inverted landscape
{"cmd":"rotation","value":270}  // inverted portrait
```

## Testing Results

All rotations tested and verified on Pi at 10.1.1.99:

| Rotation | Matrix Dims | Layout Type | Status |
|----------|-------------|-------------|--------|
| 0°       | 64×32       | LANDSCAPE   | ✅ Pass |
| 90°      | 32×64       | PORTRAIT    | ✅ Pass |
| 180°     | 64×32       | LANDSCAPE   | ✅ Pass |
| 270°     | 32×64       | PORTRAIT    | ✅ Pass |

**Test Cases:**
- Layout presets 1-7, 11-14
- Text rendering with autofont
- Segment boundaries
- Group indicators
- Web UI controls

## Deployment

### Pi Firmware
```bash
# On Pi (10.1.1.99)
cd ~/rpiC++
git pull origin main
make clean && make
sudo systemctl stop led-matrix
sudo cp led-matrix /usr/local/bin/
sudo systemctl start led-matrix
```

### Q-SYS Plugin
1. Download `LEDMatrix_v6.qplug` from GitHub
2. Import into Q-SYS Designer
3. Replace old plugin instances
4. Update designs to use new rotation control

## Known Limitations

1. **Restart Required** - Rotation changes need service restart (not runtime switchable)
2. **180° Display** - Shows content upside-down (expected behavior)
3. **Group Indicator** - Stays in bottom-left corner (not rotated with display)

## Support

- **GitHub:** https://github.com/DHPKE/QSYS-LED-Matrix
- **Pi IP:** 10.1.1.99
- **Web UI:** http://10.1.1.99:8080
- **UDP Port:** 21324

## Version History

- **v6.1.0** (2026-03-01) - Full rotation support, removed orientation control
- **v6.0.2** (2026-02-28) - Orientation deprecated, rotation layout fixes
- **v6.0.0** (2026-02-28) - Initial rotation support (buggy)
- **v5.0.0** - Group support
- **v4.0.0** - Layout presets
