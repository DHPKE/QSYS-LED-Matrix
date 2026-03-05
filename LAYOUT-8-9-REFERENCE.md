# VO Layouts - Layout 8 & 9 Reference

## Q-SYS Plugin v7.0.13

### Layout 8 - VO-left
- **Q-SYS Layout:** 8
- **Firmware Preset:** 15
- **Active Segments:** 1, 3
- **Description:** Large content area on left (Segment 1) + small indicator bottom-right (Segment 3)

### Layout 9 - VO-right
- **Q-SYS Layout:** 9
- **Firmware Preset:** 16
- **Active Segments:** 2, 3
- **Description:** Info area top-right (Segment 2) + small indicator bottom-right (Segment 3)

---

## Visual Reference

### Layout 8 (VO-left)
Based on: `examples/Unbenannt-1.png`

```
┌─────────────────────────────────────────────┬─────┐
│                                             │     │
│                                             │     │
│              SEGMENT 1                      │     │
│              (Yellow in reference)          │     │
│              42×26 pixels                   │     │
│              Position: (3, 3)               │     │
│                                             ├─────┤
│                                             │ S3  │
│                                             │Blue │
└─────────────────────────────────────────────┴─────┘
```

**Coordinates (64×32 display with 3px margins):**
- Segment 1: (3, 3, 42×26) - Large left content area
- Segment 3: (48, 19, 13×10) - Small bottom-right indicator

---

### Layout 9 (VO-right)
Based on: `examples/Unbenannt-2.png`

```
┌───────────────────────────────────┬───────────────┐
│                                   │  SEGMENT 2    │
│                                   │  (Green)      │
│                                   │  17×13        │
│                                   │  (44, 3)      │
│                                   ├───────────────┤
│                                   │               │
│                                   ├───────────────┤
│                                   │   S3  │
│                                   │  Blue │
└───────────────────────────────────┴───────┘
```

**Coordinates (64×32 display with 3px margins):**
- Segment 2: (44, 3, 17×13) - Medium top-right info area
- Segment 3: (48, 19, 13×10) - Small bottom-right indicator

---

## Firmware Version

**Current:** v7.0.14 (VO Layout per Images)  
**Deployed:** Pi @ 10.1.1.15

---

## Q-SYS Usage

### Apply Layout 8 (VO-left)
1. In Q-SYS plugin, select **Layout: 8 - VO-left**
2. Enable **Curtain Mode** for your group (optional, shows frame)
3. Send text to **Segment 1** (large left area)
4. Send text to **Segment 3** (small bottom-right indicator)

### Apply Layout 9 (VO-right)
1. In Q-SYS plugin, select **Layout: 9 - VO-right**
2. Enable **Curtain Mode** for your group (optional, shows frame)
3. Send text to **Segment 2** (medium top-right area)
4. Send text to **Segment 3** (small bottom-right indicator)

---

## UDP Commands (Direct)

### Layout 8
```json
{"cmd":"layout","preset":15}
{"cmd":"text","seg":1,"text":"SPEAKER","color":"FFFFFF","bgcolor":"0000FF"}
{"cmd":"text","seg":3,"text":"LIVE","color":"FFFFFF","bgcolor":"FF0000"}
```

### Layout 9
```json
{"cmd":"layout","preset":16}
{"cmd":"text","seg":2,"text":"INFO","color":"FFFFFF","bgcolor":"00FF00"}
{"cmd":"text","seg":3,"text":"REC","color":"FFFFFF","bgcolor":"FF0000"}
```

---

## Curtain Mode

When curtain is enabled:
- **2px frame** on all edges (red in images)
- **1px gap** between frame and segment content
- **Total margin:** 3px from display edges

All segment coordinates already account for the 3px margin.

---

## Testing

Run test script:
```bash
./test-vo-3px-margins.sh
```

This will:
- Apply Layout 8 (VO-left) with color-coded segments
- Apply Layout 9 (VO-right) with color-coded segments
- Toggle curtain on/off to verify gap
- Show visual confirmation of correct positioning

---

## Notes

- Segment indices in firmware are 0-based: {0, 1, 2, 3}
- Q-SYS plugin uses 1-based segment numbering in UI: {1, 2, 3, 4}
- Layout 8/9 added in plugin v7.0.13
- Firmware presets 15/16 added in v7.0.10
- Coordinates corrected in v7.0.14 to match visual reference images
