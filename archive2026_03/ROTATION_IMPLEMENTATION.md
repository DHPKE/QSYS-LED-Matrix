# Rotation Implementation Summary

## Changes Made

### 1. Pi C++ Code (v6.0.2)
**File:** `rpiC++/udp_handler.cpp`

#### Layout Selection Based on Rotation
- **0° and 180°** → Use `LAYOUT_LANDSCAPE` (64×32 coordinates)
- **90° and 270°** → Use `LAYOUT_PORTRAIT` (32×64 coordinates)
- The rgl-matrix library's `Rotate:X` pixel mapper handles the physical rotation
- Segment boundaries now match the rotated canvas dimensions

#### Deprecated Orientation Command
- `{"cmd":"orientation","value":"portrait"}` → maps to rotation 90°
- `{"cmd":"orientation","value":"landscape"}` → maps to rotation 0°
- Prints deprecation warning in logs
- Maintains backward compatibility with existing Q-SYS designs

### 2. Q-SYS Plugin (v6.0.2-debug)
**File:** `qsys-plugin/LEDMatrix_v6.qplug`

#### Orientation Control Deprecated
- Orientation dropdown still present (backward compatibility)
- Handler now sends rotation commands instead of orientation commands
- Prints deprecation warnings to Q-SYS debug output
- Will be removed in future stable release

#### Updated Documentation
- Header comments reflect rotation as primary control
- Deprecated section added showing orientation → rotation mapping
- Instructions clarified: rotation requires restart

## How It Works

### Rotation Flow
1. User selects rotation (0°, 90°, 180°, 270°) in Q-SYS or Web UI
2. UDP command sent: `{"cmd":"rotation","value":90}`
3. Pi saves to `/var/lib/led-matrix/config.json`
4. **Restart required** to apply (matrix initialization)
5. On startup:
   - Matrix initialized with `pixel_mapper_config = "Rotate:X"`
   - Dimensions change based on rotation (e.g., 32×64 for 90°)
   - Layouts selected accordingly (portrait for 90°/270°)
   - Segments render in correct positions

### Layout Adaptation
```
Rotation 0° (landscape):
┌────────────────┐
│   64 × 32      │  ← LAYOUT_LANDSCAPE
└────────────────┘

Rotation 90° (portrait):
┌──────┐
│      │
│ 32×64│  ← LAYOUT_PORTRAIT
│      │
└──────┘

Rotation 180° (inverted landscape):
┌────────────────┐
│   64 × 32      │  ← LAYOUT_LANDSCAPE (flipped by pixel mapper)
└────────────────┘

Rotation 270° (inverted portrait):
┌──────┐
│      │
│ 32×64│  ← LAYOUT_PORTRAIT (flipped by pixel mapper)
│      │
└──────┘
```

## Migration Guide

### For Existing Q-SYS Designs

**Option 1: Use orientation (deprecated, will work for now)**
```lua
Controls.orientation.String = "portrait"  -- Maps to rotation 90°
```

**Option 2: Switch to rotation (recommended)**
```lua
Controls.rotation.String = "90"
Controls.apply_rotation:Trigger()
```

### Testing Rotation

1. **Set rotation via Q-SYS:**
   ```
   rotation = 90
   Apply Rotation (button)
   ```

2. **Restart Pi via Web UI:**
   - Click "Reboot" button
   - Wait ~60 seconds for reboot

3. **Verify:**
   - Text should appear rotated correctly
   - Segment boundaries should match rotated display
   - Layouts should adapt automatically

## Known Issues / Limitations

1. **Restart Required**
   - Rotation can't be changed at runtime
   - Must restart service or reboot Pi

2. **180° Rotation**
   - Uses landscape layouts (64×32)
   - Pixel mapper handles the flip
   - Text will be upside down but coordinates stay the same

3. **Orientation Control**
   - Still present in Q-SYS plugin for compatibility
   - Will be removed in v7.0 stable release
   - Users should migrate to rotation control

## Version History

- **v6.0.0** - Initial rotation support (buggy - layouts didn't adapt)
- **v6.0.1-debug** - Added comprehensive logging
- **v6.0.2** - **Fixed rotation:** Layouts now adapt to rotation
- **v6.0.2-debug** - Debug build with deprecation warnings

## Files Modified

### Pi Code
- `rpiC++/udp_handler.cpp` - Layout selection logic, orientation deprecation
- `rpiC++/udp_handler.h` - No changes needed

### Q-SYS Plugin
- `qsys-plugin/LEDMatrix_v6.qplug` - Orientation handler maps to rotation

### Documentation
- `rpiC++/TROUBLESHOOTING.md` - UDP stuck state guide
- `qsys-plugin/DEBUG_v6.0.1.md` - Debug build documentation

## Current Status

✅ Rotation fully functional (0°, 90°, 180°, 270°)  
✅ Layouts adapt automatically based on rotation  
✅ Backward compatible with existing orientation commands  
✅ Deployed to Pi at 10.1.1.99  
⚠️ Orientation control deprecated (use rotation instead)  
⚠️ Restart required to apply rotation changes
