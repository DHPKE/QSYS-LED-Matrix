# Left-Aligned Text Padding Fix
**Date**: 2026-03-09 14:58 CET  
**Pi IP**: 10.1.1.26

## Problem
When text is left-aligned in VO-left layout, it should have minimal gap (1-2px) from the curtain frame, not be right at the segment edge.

## Solution
Added 1px padding for left-aligned text, resulting in 2px total gap from curtain frame.

## Coordinate Breakdown

### Before (no padding)
```
Curtain: x=0-2 (3px wide)
Gap: 1px
Segment 0: starts at x=3
Text (left-aligned): x=3 (no padding)

Total gap from curtain to text: 1px
```

### After (1px padding)
```
Curtain: x=0-2 (3px wide)
Gap: 1px
Segment 0: starts at x=3
Text (left-aligned): x=4 (1px padding)

Total gap from curtain to text: 2px ✓
```

**Visual**:
```
0   2 3 4                     103
┌───┬─┬─┬─────────────────────┐
│C│C│C│G│T Text starts here   │
│u│u│u│a│e                    │
│r│r│r│p│x                    │
│t│t│t│ │t                    │
└───┴─┴─┴─────────────────────┘
  Curtain  ↑
   3px     Text at x=4
           (1px padding)
```

## Code Change

**File**: `/opt/led-matrix/text_renderer.py`

**Before**:
```python
if snap['align'] == TextAlign.LEFT:
    tx = 0
```

**After**:
```python
if snap['align'] == TextAlign.LEFT:
    tx = 1  # 1px padding from left edge (2px from curtain)
```

## Q-SYS Plugin Recommendation

For VO-left layout (Layout 8), set **Segment 1** alignment to **LEFT** for best appearance:

```
Layout 8 (VO-left):
  Segment 1:
    - Align: LEFT ✓ (not CENTER)
    - Text: "SPEAKER NAME" or "CAMERA 1"
    - Result: Text starts 2px from curtain, fills area efficiently
  
  Segment 3:
    - Align: CENTER (default OK for small indicator)
    - Text: "LIVE" or group number
```

## Testing

From Q-SYS Designer:
1. Select Layout 8 (VO-left)
2. Set Segment 1 alignment to "Left"
3. Send text: "SPEAKER 1"
4. Verify: Text starts ~2px from left curtain frame

**Expected result**:
```
┌───┬────────────────────┐
│Crt│  SPEAKER 1         │  ← 2px gap looks natural
└───┴────────────────────┘
```

Not:
```
┌───┬────────────────────┐
│Crt│SPEAKER 1           │  ← No gap, text touching curtain
└───┴────────────────────┘
```

## Alignment Behavior Summary

| Alignment | Text Position | Use Case |
|-----------|---------------|----------|
| LEFT | x + 1px | VO layouts, labels, natural reading |
| CENTER | (w - tw) / 2 | Indicators, status, symmetric layouts |
| RIGHT | w - tw | Timers, counters (rare) |

## Backup
`text_renderer.py.backup-before-left-padding`

## Rollback
```bash
ssh node@10.1.1.26
sudo cp /opt/led-matrix/text_renderer.py.backup-before-left-padding /opt/led-matrix/text_renderer.py
sudo systemctl restart led-matrix
```

---
**Applied by**: OpenClaw AI Assistant  
**Issue**: Left-aligned text too close to curtain  
**Status**: ✅ Fixed - 2px gap from curtain
