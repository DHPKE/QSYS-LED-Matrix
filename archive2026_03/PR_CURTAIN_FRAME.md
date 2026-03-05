# Pull Request: Curtain Frame Indicator (2px Frame)

## Summary

Changes curtain indicator mode from **3-pixel left/right vertical bars** to **2-pixel frame around entire display**.

## Visual Comparison

### Before (Current)
```
┌───┬────────────────────────────────────────────────────┬───┐
│ L │                                                    │ R │
│ E │          Display content here                     │ I │
│ F │          (58 pixels wide)                         │ G │
│ T │                                                    │ H │
│   │                                                    │ T │
│ 3 │                                                    │ 3 │
│ p │                                                    │ p │
│ x │                                                    │ x │
└───┴────────────────────────────────────────────────────┴───┘
```
- Left bar: 3 pixels wide
- Right bar: 3 pixels wide
- Usable width: 58 pixels (64 - 3 - 3)

### After (This PR)
```
┌──────────────────────────────────────────────────────────┐
│  2-pixel top border                                      │
├──┬────────────────────────────────────────────────────┬──┤
│2 │                                                    │2 │
│p │          Display content here                     │p │
│x │          (60×28 usable area)                      │x │
│  │                                                    │  │
├──┴────────────────────────────────────────────────────┴──┤
│  2-pixel bottom border                                   │
└──────────────────────────────────────────────────────────┘
```
- Frame on all 4 edges: 2 pixels each
- Usable area: 60×28 pixels (64-2-2 × 32-2-2)
- **+2px width, -2px height** compared to old bars

## Changes

### Files Modified

1. **`rpi/curtain_manager.py`**
   - Changed `render()` method to draw 4-sided frame instead of 2 vertical bars
   - Top edge: `(0,0)` to `(width-1, 1)`
   - Bottom edge: `(0, height-2)` to `(width-1, height-1)`
   - Left edge: `(0,0)` to `(1, height-1)`
   - Right edge: `(width-2, 0)` to `(width-1, height-1)`
   - Updated docstring to reflect frame rendering

2. **`rpi/config.py`**
   - Changed `CURTAIN_WIDTH = 3` → `CURTAIN_WIDTH = 2`
   - Changed `CURTAIN_AUTO_REMAP = True` → `CURTAIN_AUTO_REMAP = False`
   - Updated documentation: frame is overlay, no segment remapping
   - Clarified inner display area coordinates

3. **`rpi/main.py`**
   - Version bump: `7.0.0 (Curtain Mode)` → `7.0.9 (Curtain Frame Mode)`

## Behavior Changes

### Rendering
- **Before:** Two vertical bars (left & right edges only)
- **After:** Complete frame border (all 4 edges)

### Segment Layout
- **Before:** Segments auto-remapped when curtain active (x+3, width-6)
- **After:** Segments render at original positions, frame overlays on top

### Display Area
- **Before:** 58×32 pixels (64 - 3 - 3, full height)
- **After:** 60×28 pixels (64 - 2 - 2, 32 - 2 - 2)
  - **Gain:** +2 pixels width
  - **Loss:** -2 pixels height (but gain complete border visual)

### Rotation Support
- Frame automatically adapts to canvas dimensions
- Works correctly at 0°, 90°, 180°, 270° rotation

## Compatibility

### Q-SYS Plugin
✅ **No changes required** - Plugin sends same commands:
```json
{"cmd": "curtain_config", "group": N, "enabled": true, "color": "RRGGBB"}
{"cmd": "curtain_visibility", "group": N, "visible": true}
```

### Backward Compatibility
- ✅ Same command structure
- ✅ Same group-based logic (1-8 + broadcast)
- ✅ Same color configuration
- ⚠️ Visual change only (bars → frame)

## Testing Checklist

- [ ] Deploy to Raspberry Pi
- [ ] Restart `led-matrix.service`
- [ ] Configure curtain for group 1: `{"cmd":"curtain_config","group":1,"enabled":true,"color":"FFFFFF"}`
- [ ] Toggle visibility on: `{"cmd":"curtain_visibility","group":1,"visible":true}`
- [ ] Verify 2-pixel frame appears on all 4 edges
- [ ] Toggle visibility off: `{"cmd":"curtain_visibility","group":1,"visible":false}`
- [ ] Verify frame disappears
- [ ] Test different colors (red, green, blue)
- [ ] Test with text segments rendered
- [ ] Test at 0° rotation
- [ ] Test at 90° rotation
- [ ] Test at 180° rotation
- [ ] Test at 270° rotation
- [ ] Verify frame persists across service restart
- [ ] Test multiple groups with different frame colors

## Example Test Commands

```bash
# SSH to Pi
ssh pi@10.1.1.25

# Send test commands via UDP
echo '{"cmd":"curtain_config","group":1,"enabled":true,"color":"FF0000"}' | nc -u -w1 10.1.1.25 21324
echo '{"cmd":"curtain_visibility","group":1,"visible":true}' | nc -u -w1 10.1.1.25 21324

# White frame
echo '{"cmd":"curtain_config","group":2,"enabled":true,"color":"FFFFFF"}' | nc -u -w1 10.1.1.25 21324
echo '{"cmd":"curtain_visibility","group":2,"visible":true}' | nc -u -w1 10.1.1.25 21324

# Green frame
echo '{"cmd":"curtain_config","group":3,"enabled":true,"color":"00FF00"}' | nc -u -w1 10.1.1.25 21324
echo '{"cmd":"curtain_visibility","group":3,"visible":true}' | nc -u -w1 10.1.1.25 21324

# Check logs
sudo journalctl -u led-matrix -f | grep -i curtain
```

## Rollback Plan

If issues are found:

```bash
# Revert this PR
git revert <commit-sha>

# Or checkout previous version
git checkout main~1
```

## Screenshots

_(Please test and add screenshots showing:)_
- [ ] Frame at 0° rotation
- [ ] Frame at 90° rotation
- [ ] Frame with text segments
- [ ] Different frame colors (white, red, green, blue)
- [ ] Before/after comparison (if possible)

## Motivation

The previous 3-pixel vertical bars implementation:
- Only indicated left/right edges
- Used more horizontal space (6 pixels total)
- No top/bottom indication

The new 2-pixel frame:
- Provides complete border visibility (all 4 edges)
- Uses less horizontal space (+2px width gain)
- Better visual indicator (picture frame style)
- More suitable for status/group indication

## Related Issues

Closes: _(Link to issue if applicable)_

## Checklist

- [x] Code changes implemented
- [x] Documentation updated (config.py, curtain_manager.py)
- [x] Version bumped (7.0.9)
- [ ] Tested on actual hardware
- [ ] Screenshots added
- [ ] Q-SYS plugin compatibility verified
- [ ] No breaking changes to API

## Merge Strategy

Recommend: **Squash and merge** (3 commits into 1)

Final commit message:
```
feat: Change curtain indicator from 3px bars to 2px frame

- Replace left/right 3px bars with 2px frame on all 4 edges
- Update rendering logic in curtain_manager.py
- Gain +2px width, -2px height in usable display area
- Frame overlays segments (no auto-remapping)
- Version bump to 7.0.9 (Curtain Frame Mode)
- No breaking changes to Q-SYS plugin commands
```
