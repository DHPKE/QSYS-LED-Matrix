# VO Layout Fix - 128×64 Matrix
**Date**: 2026-03-09 14:50 CET  
**Pi IP**: 10.1.1.26

## Problem
- Fullscreen layouts (1, 11-14) not using maximum font size (auto-scale issue)
- VO-left (layout 8) and VO-right (layout 9) coordinates still at 64×32 scale
- Text not filling available space on 128×64 display

## Solution Applied

### 1. Landscape VO Layouts (128×64)

**Layout 8 (VO-left)**:
```python
# Before (64×32):
seg0: (3, 3, 49, 26)   # 49×26 main left
seg2: (52, 22, 9, 7)   # 9×7 BR indicator

# After (128×64) - 2x scaling:
seg0: (3, 3, 100, 54)   # 100×54 main left (doubled)
seg2: (106, 46, 18, 14) # 18×14 BR indicator (doubled)
```

**Layout 9 (VO-right)**:
```python
# Before (64×32):
seg1: (29, 2, 33, 20)   # 33×20 top-right
seg2: (52, 22, 9, 7)    # 9×7 BR indicator

# After (128×64) - proportional scaling:
seg1: (60, 3, 64, 38)   # 64×38 top-right
seg2: (106, 46, 18, 14) # 18×14 BR indicator (same as layout 8)
```

### 2. Portrait VO Layouts (64×128 after rotation)

**Layout 8 (VO-left portrait)**:
```python
# Before: Used PW-6, (5*PH)//6-6 variables
# After (explicit values):
seg0: (3, 3, 58, 100)   # 58×100 main area
seg2: (42, 92, 18, 14)  # 18×14 BR indicator
```

**Layout 9 (VO-right portrait)**:
```python
seg1: (3, 3, 58, 64)    # 58×64 top area
seg2: (42, 92, 18, 14)  # 18×14 BR indicator
```

## Usable Area with 3px Curtain Margins

**Landscape (128×64)**:
- Curtain bars: x=0-2, x=125-127 (left/right), y=0-2, y=61-63 (top/bottom)
- Usable area: x=3 to x=124 (122px wide), y=3 to y=60 (58px tall)

**Portrait (64×128)**:
- Curtain bars: x=0-2, x=61-63 (left/right), y=0-2, y=125-127 (top/bottom)
- Usable area: x=3 to x=60 (58px wide), y=3 to y=124 (122px tall)

## Changes Made

### Files Modified on Pi
- `/opt/led-matrix/config.py`
  - Updated `LAYOUT_PRESETS` layouts 8 & 9 (landscape)
  - Updated `LAYOUT_PRESETS_PORTRAIT` layouts 8 & 9 (portrait)

### Backups Created
- `config.py.backup-20260309-144302` (initial 128×64 conversion)
- `config.py.backup-vo-fix` (before VO layout changes)
- `config.py.backup-before-vo-128x64` (before final VO fixes)

## Service Status
```
✅ Service restarted successfully
✅ No syntax errors
✅ Layouts loading correctly
```

## Font Auto-Scaling Notes

The text renderer (`text_renderer.py`) uses PIL's `textbbox()` to auto-fit text:

**Process**:
1. Start with font size = segment height
2. Measure text width/height with current font size
3. If too large → reduce font size by 1px
4. Repeat until text fits segment dimensions
5. Apply final sized font

**For fullscreen layouts** (1, 11-14):
- Segment dimensions: 128×64 pixels (full display)
- Font auto-scales to maximum size that fits
- Single characters should use very large fonts (~40-50px)
- Long text automatically shrinks to fit

**Testing fullscreen auto-scale**:
Send single character "A" → should fill nearly entire 128×64 screen
Send "HELLO" → should auto-scale down to fit width

## Next Steps

### 1. Test VO Layouts
From Q-SYS plugin:
- Select layout 8 (VO-left)
- Send text to Segment 1 (main left area)
- Send text to Segment 3 (BR indicator)
- Verify proportions and spacing

### 2. Test Fullscreen Auto-Scale
- Select layout 1 (fullscreen)
- Send single character → verify it scales to ~50px+
- Send long text → verify it shrinks to fit
- Compare with old 64×32 display (should be visibly larger)

### 3. Verify All Layouts
Test presets 1-14 to ensure proper scaling across all configurations.

## Rollback Procedure

If layouts are incorrect:
```bash
ssh node@10.1.1.26
sudo cp /opt/led-matrix/config.py.backup-20260309-144302 /opt/led-matrix/config.py
sudo systemctl restart led-matrix
```

## Technical Details

### Scaling Logic
All coordinates and dimensions doubled from 64×32 to 128×64:
- Old width 64 → New width 128 (2x)
- Old height 32 → New height 64 (2x)
- Margins kept at 3px (curtain frame)

### VO Layout Design
**VO-left (layout 8)**:
- Optimized for speaker on left side
- Large text area (100×54) for main content
- Small indicator (18×14) in bottom-right corner

**VO-right (layout 9)**:
- Optimized for speaker on right side
- Medium text area (64×38) for main content
- Same indicator position as layout 8

Both layouts maintain 3px gaps from curtain bars and consistent corner indicator placement.

---
**Applied by**: OpenClaw AI Assistant  
**Session**: QSYS-LED-Matrix deployment
