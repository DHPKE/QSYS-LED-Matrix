# Anti-Flicker Configuration - Final

**Date**: 2026-03-09 20:19 CET  
**Pi IP**: 10.1.1.26 (CM4)  
**Status**: ✅ Optimized

## Problem

Visible flicker when passing by the panel, especially on **solid color backgrounds**. Text was fine.

## Root Cause

**PWM flicker** - High PWM_BITS (11) creates slow PWM frequency, visible on large solid areas when in peripheral vision.

## Solution

Reduce PWM_BITS to increase PWM frequency:

### Final Settings

```python
MATRIX_GPIO_SLOWDOWN    = 3           # Stable (~200Hz row scan)
MATRIX_PWM_BITS        = 5           # 32 color levels (maximum PWM speed)
MATRIX_REFRESH_LIMIT    = 0           # Unlimited refresh
MATRIX_MULTIPLEXING     = 0           # Default (modes 1-2 were worse)
MATRIX_SCAN_MODE        = 0           # Progressive
MATRIX_PWM_DITHER_BITS  = 0           # Off (dithering made it worse)
MATRIX_ROW_ADDRESS_TYPE = 0           # Default
```

### Text Padding

Added **1px gap** between text and segment edges:

```python
# text_renderer.py
avail_w = max(1, snap['width'] - 2)   # 1px left + 1px right
avail_h = max(1, snap['height'] - 2)  # 1px top + 1px bottom

# Right alignment: 1px padding from right edge
tx = snap['width'] - tw - 1

# Vertical: 1px top margin
ty = ((snap['height'] - th) // 2 - bbox[1]) + 1
```

## Testing Process

Tried in order:
1. ✗ PWM_BITS 8→6 (same)
2. ✗ REFRESH_LIMIT 300→0 (same)
3. ✗ GPIO_SLOWDOWN 2→1 (worse - too aggressive)
4. ✗ SCAN_MODE 0→1 (same)
5. ✗ ROW_ADDRESS_TYPE 0→1 (bad)
6. ✗ PWM_DITHER_BITS 0→1 (worse)
7. ✓ GPIO_SLOWDOWN = 3 + PWM_BITS = 11 + REFRESH_LIMIT = 500/1000/0 (better trend)
8. ✓ PWM_BITS 11→7→6→5 (progressively better on solid backgrounds)

## Results

**Before**: 
- PWM_BITS = 11 (2048 color levels)
- Visible PWM flicker on solid colors when passing by
- Text looked good but no margins

**After**:
- PWM_BITS = 5 (32 color levels)
- **Much less flicker** on solid backgrounds
- Text still looks good with clean 1px margins
- Slight color banding on gradients (acceptable trade-off)

## Trade-offs

**Color depth**: 2048 levels → 32 levels per channel
- Total colors: 2048³ → 32³ (2B colors → 32K colors)
- Gradients show slight banding
- Solid colors and text look excellent

**CPU usage**: ~40% (plenty of headroom on CM4)

## Backups

- `/opt/led-matrix/config.py.point-zero` - Starting point before testing
- `/opt/led-matrix/text_renderer.py.backup-before-1px-padding`

## Point Zero (Baseline)

If need to revert to pre-optimization state:
```bash
sudo cp /opt/led-matrix/config.py.point-zero /opt/led-matrix/config.py
sudo systemctl restart led-matrix
```

Point Zero settings:
- GPIO_SLOWDOWN = 2
- PWM_BITS = 8
- REFRESH_LIMIT = 300
- No text padding

## Key Insight

**Peripheral vision flicker** on LED matrices is primarily caused by:
1. **PWM frequency** (not row scan rate) for solid colors
2. **Row scanning** is inherent and cannot be eliminated
3. Text with edges masks PWM artifacts naturally

**Solution**: Lower PWM_BITS for faster PWM frequency on solid backgrounds.

---

**Status**: ✅ Optimized for CM4, flicker minimized, text looks great
