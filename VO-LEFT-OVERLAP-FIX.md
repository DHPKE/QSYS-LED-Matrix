# VO-Left Overlap Fix
**Date**: 2026-03-09 14:56 CET  
**Pi IP**: 10.1.1.26

## Problem
Segment 0 (main text area) was overlapping with Segment 2 (BR corner indicator) by 16 pixels.

## Before (Broken)
```
Layout 8 (VO-left):
  Seg0: x=3, y=3, w=116, h=54
    → ends at x=119
  
  Seg2: x=103, y=43, w=18, h=14
    → starts at x=103
  
  ❌ OVERLAP: x=103 to x=119 (16px conflict!)
```

**Visual (128×64 display)**:
```
0     3              103   119  124 127
┌─────┬────────────────┼─────┼────┬───┐
│Curt.│   Segment 0    ┊█████│    │C. │ ← y=3-43
│  3px│                ┊█Seg2│    │3px│
│     │                ┊█████│    │   │
│     │                XXXXX│    │   │ ← y=43-57 (overlap!)
└─────┴────────────────┴─────┴────┴───┘
      ↑                ↑     ↑
    start=3         seg2=103 end=119
    
    X = Overlap region (16px)
```

## After (Fixed)
```
Layout 8 (VO-left):
  Seg0: x=3, y=3, w=100, h=54
    → ends at x=103
  
  (gap: 3px)
  
  Seg2: x=106, y=43, w=18, h=14
    → starts at x=106
  
  ✓ NO OVERLAP: 3px gap between segments
```

**Visual (128×64 display)**:
```
0     3           103 106  124 127
┌─────┬─────────────┬─┬────┬───┐
│Curt.│  Segment 0  │ │Seg2│C. │ ← y=3-42
│  3px│             │ │    │3px│
│     │             │ │    │   │
│     │             │ │████│   │ ← y=43-57
└─────┴─────────────┴─┴────┴───┘
      ↑             ↑ ↑    ↑
    start=3      end gap  seg2
                 103 106  124
    
    Gap = 3px clear space
```

## Layout 8 Dimensions

### Landscape (128×64)
```python
Segment 0 (main left area):
  Position: (3, 3)
  Size: 100×54
  Occupies: x=3 to x=103, y=3 to y=57
  
Gap: 3px

Segment 2 (BR indicator):
  Position: (106, 43)
  Size: 18×14
  Occupies: x=106 to x=124, y=43 to y=57
```

### Coordinate Breakdown
| Element | X Start | X End | Width | Y Start | Y End | Height |
|---------|---------|-------|-------|---------|-------|--------|
| Curtain left | 0 | 2 | 3px | 0 | 63 | 64px |
| **Seg0** | **3** | **102** | **100px** | **3** | **56** | **54px** |
| Gap | 103 | 105 | 3px | - | - | - |
| **Seg2** | **106** | **123** | **18px** | **43** | **56** | **14px** |
| Gap | 124 | 124 | 1px | - | - | - |
| Curtain right | 125 | 127 | 3px | 0 | 63 | 64px |

## Changes Applied

### File Modified
`/opt/led-matrix/config.py` - Layout 8 (VO-left)

### Before
```python
8: [(3,        3,         116,   54   ),  # seg0 - TOO WIDE
    (0,        0,         1,     1    ),  # seg1 hidden
    (103,      43,        18,    14   ),  # seg2 - OVERLAPPED
    (0,        0,         1,     1    )], # seg3 hidden
```

### After
```python
8: [(3,        3,         100,   54   ),  # seg0 - ends at x=103
    (0,        0,         1,     1    ),  # seg1 hidden
    (106,      43,        18,    14   ),  # seg2 - starts at x=106
    (0,        0,         1,     1    )], # seg3 hidden
```

## Usage in Q-SYS

**Layout 8 (VO-left)** is designed for speaker on the left side:

1. **Segment 1** (QSYS) = Segment 0 (firmware)
   - Main text area (100×54)
   - Use for: Speaker name, camera label, main content
   - Auto-scales font to fit

2. **Segment 3** (QSYS) = Segment 2 (firmware)
   - Corner indicator (18×14)
   - Use for: Status icon, group ID, small label
   - Positioned in bottom-right usable area

**Example**:
```
Segment 1 text: "CAMERA 1"      → Large, fills left area
Segment 3 text: "LIVE"          → Small, bottom-right corner
```

## Verification

Service restarted successfully:
```
✅ No overlap
✅ 3px gap between seg0 and seg2
✅ Both segments fit within usable area (x=3 to x=124)
```

## Backup
`config.py.backup-before-overlap-fix`

## Rollback (if needed)
```bash
ssh node@10.1.1.26
sudo cp /opt/led-matrix/config.py.backup-before-overlap-fix /opt/led-matrix/config.py
sudo systemctl restart led-matrix
```

---
**Fixed by**: OpenClaw AI Assistant  
**Issue**: Segment overlap in VO-left layout  
**Status**: ✅ Resolved
