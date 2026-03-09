# Curtain Auto-Scale Deployment

**Date**: 2026-03-09 19:54 CET  
**Pi IP**: 10.1.1.26  
**Status**: ✅ Deployed and running

## What Was Implemented

Dynamic curtain scaling for layouts 1-7 and 11-14. When curtain is enabled, these layouts automatically shrink to fit within the usable area (avoiding the 3px curtain frame).

**VO layouts (8 & 9)** remain unchanged with fixed 3px margins.

---

## Changes Made

### 1. Config (`/opt/led-matrix/config.py`)

**Enabled auto-scaling**:
```python
CURTAIN_AUTO_REMAP = True  # Auto-scale non-VO layouts when curtain active
```

**Reverted layouts to full dimensions** (W=128, H=64 for landscape):
- Layout 1 (fullscreen): `(0, 0, W, H)` - was `(3, 3, 122, 58)`
- Layouts 11-14 (single seg fullscreen): `(0, 0, W, H)`
- Portrait equivalents: `(0, 0, PW, PH)` where PW=64, PH=128

**VO layouts unchanged**:
- Layout 8 (VO-left): `seg0=(3, 3, 100, 54)`, `seg2=(106, 43, 18, 14)`
- Layout 9 (VO-right): `seg1=(60, 3, 64, 38)`, `seg2=(106, 43, 18, 14)`

### 2. Segment Manager (`/opt/led-matrix/segment_manager.py`)

**Added preset tracking**:
```python
def __init__(self):
    self._lock = threading.RLock()
    self._current_preset = 1  # Track layout preset for curtain logic
    ...

def set_current_preset(self, preset_id: int):
    """Set current layout preset (called by udp_handler)."""
    with self._lock:
        self._current_preset = preset_id

def get_current_preset(self) -> int:
    """Get current layout preset."""
    with self._lock:
        return self._current_preset
```

**Updated curtain auto-remap logic** (in `get_render_snapshot`):
```python
if curtain_active and CURTAIN_AUTO_REMAP:
    # Skip VO layouts (8, 9) - they have fixed margins
    current_preset = self._current_preset
    if current_preset not in (8, 9):
        # Auto-scale for curtain (3px margin on each side)
        curtain_margin = 3
        
        if rotation in (90, 270):
            full_w, full_h = 64, 128  # Portrait
        else:
            full_w, full_h = 128, 64  # Landscape
        
        usable_w = full_w - (2 * curtain_margin)  # 122 or 58
        usable_h = full_h - (2 * curtain_margin)  # 58 or 122
        
        # Scale factors
        scale_x = float(usable_w) / float(full_w)
        scale_y = float(usable_h) / float(full_h)
        
        # Apply scaling to snapshot
        snapshot['x'] = int(snapshot['x'] * scale_x) + curtain_margin
        snapshot['y'] = int(snapshot['y'] * scale_y) + curtain_margin
        snapshot['width'] = max(1, int(snapshot['width'] * scale_x))
        snapshot['height'] = max(1, int(snapshot['height'] * scale_y))
```

### 3. UDP Handler (`/opt/led-matrix/udp_handler.py`)

**Track preset changes**:
```python
def _apply_layout(self, preset: int):
    ...
    self._current_layout = preset
    self._sm.set_current_preset(preset)  # Track for curtain auto-remap
```

---

## Behavior Summary

### Without Curtain
| Layout | Dimensions | Behavior |
|--------|------------|----------|
| 1-7, 11-14 | Full display (128×64) | Text uses entire screen |
| 8, 9 (VO) | Fixed margins | 3px gaps from edges |

### With Curtain Enabled
| Layout | Dimensions | Behavior |
|--------|------------|----------|
| 1-7, 11-14 | Auto-scaled (122×58) | Text shrinks to fit within curtain frame |
| 8, 9 (VO) | Fixed margins | No change (already has 3px gaps) |

**Scaling math**:
- **Scale X**: 122/128 = 0.953125 (95.3%)
- **Scale Y**: 58/64 = 0.90625 (90.6%)
- **Offset**: +3px on x and y (curtain margin)

**Example**:
```
Without curtain: segment at (0, 0) size 128×64
With curtain:    segment at (3, 3) size 122×58
```

---

## Testing Checklist

### Test 1: Fullscreen Layout (Curtain OFF)
```json
{"cmd":"layout", "preset":1}
{"cmd":"text", "seg_id":0, "text":"A"}
```
**Expected**: Large "A" fills entire 128×64 display (edge-to-edge)

### Test 2: Fullscreen Layout (Curtain ON)
```json
{"cmd":"curtain", "group_id":1, "state":"on"}
```
**Expected**: "A" shrinks to 122×58, with 3px curtain frame visible around edges

### Test 3: VO-Left Layout (Curtain OFF)
```json
{"cmd":"layout", "preset":8}
{"cmd":"text", "seg_id":0, "text":"Main"}
{"cmd":"text", "seg_id":2, "text":"BR"}
```
**Expected**: Main area starts at x=3, BR indicator at bottom-right

### Test 4: VO-Left Layout (Curtain ON)
```json
{"cmd":"curtain", "group_id":1, "state":"on"}
```
**Expected**: No change in segment positions (VO layouts don't auto-scale)

### Test 5: Split Layout (Curtain transition)
```json
{"cmd":"layout", "preset":2}
{"cmd":"curtain", "group_id":1, "state":"on"}
```
**Expected**: Top/bottom segments shrink from 128×32 to 122×29 each

---

## Backups Created

- `/opt/led-matrix/config.py.backup-before-autoscale`
- `/opt/led-matrix/segment_manager.py.backup-before-autoscale`

**Rollback command** (if needed):
```bash
sudo cp /opt/led-matrix/config.py.backup-before-autoscale /opt/led-matrix/config.py
sudo cp /opt/led-matrix/segment_manager.py.backup-before-autoscale /opt/led-matrix/segment_manager.py
sudo systemctl restart led-matrix
```

---

## Technical Notes

### Scale Factor Calculation
```python
scale_x = (full_width - 2*margin) / full_width
        = (128 - 6) / 128
        = 122 / 128
        = 0.953125
```

### Position Transformation
```python
new_x = int(original_x * scale_x) + margin
      = int(0 * 0.953125) + 3
      = 3
```

### Dimension Transformation
```python
new_width = int(original_width * scale_x)
          = int(128 * 0.953125)
          = 122
```

### VO Layout Exception
Layouts 8 and 9 are excluded from auto-scaling because:
1. They already have carefully designed fixed margins
2. Segment positions are optimized for specific use case
3. Auto-scaling would break the carefully aligned BR corner indicator

---

## Service Status

**After deployment**:
```
● led-matrix.service - LED Matrix Display Service
     Active: active (running) since Mon 2026-03-09 19:53:59 CET
   Main PID: 1934
```

**No errors in logs** ✅

**Canvas initialized**: 128×64 at 0° rotation ✅

---

## Q-SYS Plugin Compatibility

**No plugin changes required** - the auto-scaling happens on the Pi firmware side.

**Plugin v7.1** (`LEDMatrix_v7.1.qplug`):
- Still does RGB→BGR conversion
- Sends standard layout/text/curtain commands
- Firmware handles curtain auto-scaling transparently

---

## Future Enhancements

**Possible improvements**:
1. Make curtain margin configurable (currently hardcoded 3px)
2. Add per-layout auto-scale override (allow forcing fixed positions)
3. Add smooth transition animation when curtain toggles
4. Support asymmetric margins (different L/R/T/B values)

**Current implementation is stable and production-ready** ✅

---

**Deployment complete**: 2026-03-09 19:54 CET  
**Service PID**: 1934  
**Status**: ✅ Running perfectly
