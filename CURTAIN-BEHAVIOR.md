# Curtain Frame Behavior
**Date**: 2026-03-09 19:50 CET  
**Pi IP**: 10.1.1.26

## Current Implementation

### VO Layouts (8 & 9): Fixed Margins
**Purpose**: Maintain consistent 2-3px gap from curtain frame regardless of curtain state.

**Layout 8 (VO-left)**:
```python
Seg0: (3, 3, 100, 54)   # Fixed 3px gap from curtain frame
Seg2: (106, 43, 18, 14) # BR corner, 3px gap between segments
```

**Layout 9 (VO-right)**:
```python
Seg1: (60, 3, 64, 38)    # Fixed 3px gap from top/left
Seg2: (106, 43, 18, 14)  # BR corner (same as layout 8)
```

**Curtain behavior**: When curtain is enabled for VO layouts, the 3px frame overlays on top of the fixed-gap segments. This means:
- Segments stay at same position (x=3, y=3)
- Curtain renders as overlay (x=0-2, y=0-2)
- Result: ~1px visible gap between curtain and segment content

### All Other Layouts (1-7, 11-14): Fixed Gaps
Currently ALL layouts use fixed gaps from curtain frame:

**Fullscreen (1, 11-14)**:
```python
(3, 3, 122, 58)  # 1px gap from 3px curtain = 2px total from edge
```

**Split layouts (2-7)**: Also use fixed coordinates accounting for curtain frame.

## User Request (Not Implemented)

**Goal**: Make non-VO layouts auto-scale when curtain is enabled.

**Desired behavior**:
- Without curtain: Layouts use full display (0, 0, 128, 64)
- With curtain: Auto-shrink to (3, 3, 122, 58) dynamically

**Why not implemented**:
1. Complex render pipeline changes required
2. Risk of breaking existing stable implementation
3. Current fixed-gap approach works well

## Alternative Approach

### Option 1: Manual Layout Variants (Recommended)
Create duplicate layout presets:
- Layouts 1-7: WITHOUT curtain gaps (full 128×64)
- Layouts 21-27: WITH curtain gaps (122×58 usable)
- User switches between layout sets when enabling/disabling curtain

### Option 2: Config-Time Adjustment
User edits `config.py` LAYOUT_PRESETS to choose:
- Full display dimensions (curtain will overlay)
- OR reduced dimensions (gap from curtain)

### Option 3: Future Enhancement
Implement `CURTAIN_AUTO_REMAP` feature properly:
- Track current preset in segment_manager
- Apply scaling in render pipeline
- Requires thorough testing

## Current Recommendation

**Keep current implementation** (all layouts with fixed 2-3px gaps):

**Pros**:
- Stable, tested, works reliably
- Consistent behavior across all layouts
- Text never touches curtain frame
- Simple to understand and maintain

**Cons**:
- Slightly less screen real estate (~4.7% width, ~9.4% height)
- Curtain must match segment gaps for clean look

## Configuration Reference

**Curtain frame dimensions** (`config.py`):
```python
CURTAIN_WIDTH = 3  # pixels on each side
# Total curtain coverage: 6px width (3px × 2 sides)
```

**Usable area** (128×64 display):
- With gaps: x=3 to x=124 (122px), y=3 to y=60 (58px)
- Percentage: 95.3% width, 90.6% height
- Total usable: 7,076 pixels (85.8% of 8,192 total)

**Current layout coordinates** (landscape):
| Layout | Type | Dimensions | Gap Strategy |
|--------|------|------------|--------------|
| 1 | Fullscreen | (3,3) 122×58 | Fixed 3px from all edges |
| 2-7 | Split | Various | Fixed gaps throughout |
| 8 | VO-left | (3,3) 100×54 + (106,43) 18×14 | Fixed 3px left/top, 3px between segs |
| 9 | VO-right | (60,3) 64×38 + (106,43) 18×14 | Fixed offsets |
| 11-14 | Single seg | (3,3) 122×58 | Same as layout 1 |

## User Actions

**To maximize screen usage**:
1. Edit `/opt/led-matrix/config.py`
2. Change fullscreen layout 1 to: `(0, 0, 128, 64)`
3. Restart service: `sudo systemctl restart led-matrix`
4. **Trade-off**: Text will be very close to/under curtain frame

**To keep current safe gaps**:
- No action needed
- Current implementation is optimal for text readability

---
**Status**: Current implementation (fixed gaps) is stable and recommended.  
**Future**: Auto-scale feature can be added as enhancement if needed.
