# Deployment Summary - 128×64 BGR Matrix
**Date**: 2026-03-09 14:43 CET  
**Pi IP**: 10.1.1.26  
**User**: node

## Changes Applied

### 1. Hardware Configuration (config.py)
**File**: `/opt/led-matrix/config.py`

**Changes**:
- `MATRIX_WIDTH`: 64 → **128**
- `MATRIX_HEIGHT`: 32 → **64**
- `MATRIX_LED_RGB_SEQUENCE`: "RGB" → **"BGR"**

**Backup created**: `/opt/led-matrix/config.py.backup-20260309-144302`

### 2. Q-SYS Plugin (local workspace)
**File**: `qsys-plugin/LEDMatrix_v7.1.qplug`

**Changes**:
- Plugin name: "PKE~LED Matrix 128x64 BGR"
- Version: 7.1.0
- ID: `dhpke.olimex.led.matrix.128x64.bgr`
- Added `ConvertRGBtoBGR()` function to swap R↔B channels
- Applied color conversion to all color parameters:
  - Text color
  - Background color
  - Frame color
  - Curtain color

**Color conversion examples**:
- Red `FF0000` → `0000FF` (Blue in BGR)
- Green `00FF00` → `00FF00` (unchanged)
- Blue `0000FF` → `FF0000` (Red in BGR)
- White `FFFFFF` → `FFFFFF` (unchanged)
- Yellow `FFFF00` → `00FFFF` (Cyan in BGR)

### 3. Service Status
```
● led-matrix.service - LED Matrix Display Service
     Active: active (running) since Mon 2026-03-09 14:43:03 CET
   Main PID: 1065
```

**Log confirmation**:
```
14:43:04 [INFO] [RENDER] Canvas resized to 128×64 for rotation=0°
14:43:04 [INFO] [SPLASH] Rendered IP splash at 0° rotation: 10.1.1.26
```

## Testing Checklist

- [x] Config backup created
- [x] Matrix size updated (128×64)
- [x] Color order updated (BGR)
- [x] Service restarted successfully
- [x] Canvas resized confirmed in logs
- [ ] Visual test: Colors display correctly
- [ ] Q-SYS plugin v7.1 deployed to Designer
- [ ] End-to-end test from Q-SYS

## Next Steps

### 1. Deploy Q-SYS Plugin v7.1
Copy `LEDMatrix_v7.1.qplug` to Q-SYS Designer plugins folder:
- **Windows**: `C:\Users\<username>\Documents\QSC\Q-Sys Designer\Plugins\`
- **macOS**: `~/Documents/QSC/Q-Sys Designer/Plugins/`

### 2. Test Color Accuracy
Send test colors from Q-SYS plugin:
- Red text → Should appear as RED on display (converted to BGR internally)
- Blue text → Should appear as BLUE on display
- Green text → Should appear as GREEN on display

### 3. Verify Layouts
Test all layout presets (1-14) to ensure proper scaling for 128×64 resolution.

## Rollback Procedure

If colors are incorrect or display issues occur:

```bash
# SSH to Pi
ssh node@10.1.1.26

# Restore backup
sudo cp /opt/led-matrix/config.py.backup-20260309-144302 /opt/led-matrix/config.py

# Restart service
sudo systemctl restart led-matrix
```

## Technical Notes

### Color Conversion Logic
The Q-SYS plugin now converts all RGB hex values to BGR before sending UDP commands:

```lua
-- RGB to BGR conversion
local function ConvertRGBtoBGR(hexColor)
    -- Input: "RRGGBB"
    -- Output: "BBGGRR"
    local r = hexColor:sub(1,2)
    local g = hexColor:sub(3,4)
    local b = hexColor:sub(5,6)
    return b .. g .. r  -- BGR order
end
```

### Hardware-Side Color Order
The Python firmware reads `MATRIX_LED_RGB_SEQUENCE = "BGR"` and passes it to the rgbmatrix library, which handles pixel-level color ordering. No changes needed in Python code.

### Layout Coordinates
Layout presets in Python code automatically scale based on `MATRIX_WIDTH` and `MATRIX_HEIGHT` constants, so no manual coordinate adjustments needed.

## Files Modified

### On Pi (10.1.1.26)
- `/opt/led-matrix/config.py` (width, height, color order)

### In Workspace
- `qsys-plugin/LEDMatrix_v7.1.qplug` (new file, BGR color conversion)

## Status
✅ **Hardware configuration complete**  
✅ **Service running with 128×64 BGR**  
⏳ **Q-SYS plugin ready for deployment**  
⏳ **Visual testing pending**

---
**Deployed by**: OpenClaw AI Assistant  
**Session**: /Users/user/.openclaw/workspace/QSYS-LED-Matrix/
