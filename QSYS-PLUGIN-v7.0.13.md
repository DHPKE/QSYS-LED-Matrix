# v7.0.13 - Q-SYS Plugin VO Layouts

## Summary

Added Layout 8 & 9 to Q-SYS plugin for voice-over video production layouts.

**Version:** 7.0.13  
**Date:** 2026-03-04  
**Plugin File:** `LEDMatrix_v7.qplug`

---

## New Layouts

### Layout 8: VO-left
**Maps to firmware preset 15**

- **Segment 1:** 5/6 width aligned left (53×32 pixels)
  - Use for: Speaker name, large text, main content
  
- **Segment 3:** Quarter bottom-right (32×16 pixels)
  - Use for: LIVE indicator, timestamp, logo

**Active segments:** 1, 3 (segments 0 & 2 hidden)

### Layout 9: VO-right
**Maps to firmware preset 16**

- **Segment 2:** 1/3 width aligned top-right (21×32 pixels)
  - Use for: Info sidebar, subtitles, scrolling text
  
- **Segment 3:** Quarter bottom-right (32×16 pixels)
  - Use for: REC indicator, timestamp, logo

**Active segments:** 2, 3 (segments 0 & 1 hidden)

---

## Usage in Q-SYS Designer

### 1. Select Layout
Open the plugin and select from Layout dropdown:
- `8 - VO-left` (for left-side main content)
- `9 - VO-right` (for right-side sidebar)

### 2. Configure Segments

#### Layout 8 Example (VO-left)
```
Segment 1 (5/6 left):
  Text: "Dr. Jane Smith"
  Color: White
  Background: Blue
  Align: Center

Segment 3 (quarter BR):
  Text: "LIVE"
  Color: White
  Background: Red
  Align: Center
```

#### Layout 9 Example (VO-right)
```
Segment 2 (1/3 right):
  Text: "Recording"
  Color: White
  Background: Green
  Align: Center

Segment 3 (quarter BR):
  Text: "REC"
  Color: White
  Background: Red
  Align: Center
```

### 3. Apply Layout
Click "Apply Layout" button to send preset to panel.

Segments automatically show/hide based on layout selection.

---

## Preset Mapping

| Q-SYS Layout | Firmware Preset | Description |
|--------------|-----------------|-------------|
| 1 | 1 | Fullscreen |
| 2 | 2 | Top / Bottom |
| 3 | 3 | Left / Right |
| 4 | 4 | Triple Left |
| 5 | 5 | Triple Right |
| 6 | 6 | Thirds |
| 7 | 7 | Quad View |
| **8** | **15** | **VO-left** |
| **9** | **16** | **VO-right** |
| 11 | 11 | Fullscreen Seg 1 |
| 12 | 12 | Fullscreen Seg 2 |
| 13 | 13 | Fullscreen Seg 3 |
| 14 | 14 | Fullscreen Seg 4 |

---

## Compatibility

**Firmware Required:** v7.0.10 or later  
**Plugin Version:** v7.0.13  
**Q-SYS Core:** All models with UDP support

**Note:** Layouts 8 & 9 require firmware v7.0.10+ which includes preset 15 & 16 support.

---

## Installation

1. Open Q-SYS Designer
2. File → Import Component...
3. Select `LEDMatrix_v7.qplug`
4. Drag component to design canvas
5. Configure IP address and UDP port
6. Select Layout 8 or 9 from dropdown

---

## Visual Reference

### Layout 8: VO-left (Landscape 64×32)
```
┌──────────────────────────────────────────────┬──────────┐
│                                              │          │
│          Segment 1 (53×32)                   │          │
│          "Dr. Jane Smith"                    │          │
│          5/6 width left                      │          │
│                                              │          │
│                                              ├──────────┤
│                                              │  Seg 3   │
│                                              │  LIVE    │
└──────────────────────────────────────────────┴──────────┘
              53 pixels                            32×16
```

### Layout 9: VO-right (Landscape 64×32)
```
┌────────────────────────────────────────┬──────────────┐
│                                        │  Segment 2   │
│                                        │  "Recording" │
│                                        │  1/3 width   │
│                                        │              │
│                                        │              │
│                                        ├──────────────┤
│                                        │   Seg 3      │
│                                        │   REC        │
└────────────────────────────────────────┴──────────────┘
              43 pixels                      21px  32×16
```

---

## Use Cases

### Layout 8 (VO-left)
- Video podcast speaker identification
- Conference presenter names
- Tutorial instructor credits
- Live stream host information
- Interview participant labels

### Layout 9 (VO-right)
- News ticker sidebar
- Social media handles (@username)
- Sports statistics sidebar
- Translation/subtitle column
- Live chat display

---

## Example Q-SYS Lua Control

```lua
-- Select Layout 8 (VO-left)
Controls.layout_preset.String = "8 - VO-left"
Controls.layout_apply.Boolean = true

-- Configure Segment 1 (speaker name)
Controls.seg1_text.String = "Dr. Jane Smith"
Controls.seg1_color.String = "FFFFFF"
Controls.seg1_bgcolor.String = "0066CC"
Controls.seg1_send.Boolean = true

-- Configure Segment 3 (LIVE indicator)
Controls.seg3_text.String = "LIVE"
Controls.seg3_color.String = "FFFFFF"
Controls.seg3_bgcolor.String = "FF0000"
Controls.seg3_send.Boolean = true
```

---

## Testing

After installing the plugin:

1. Set IP address to your LED panel (e.g., 10.1.1.15)
2. Select "8 - VO-left"
3. Click "Apply Layout"
4. Configure Segment 1 text
5. Click "Send" for Segment 1
6. Configure Segment 3 text
7. Click "Send" for Segment 3
8. Verify layout on physical panel

Repeat for Layout 9 (VO-right) using Segment 2 & 3.

---

## Changelog

**v7.0.13** (2026-03-04)
- Added Layout 8 - VO-left (maps to preset 15)
- Added Layout 9 - VO-right (maps to preset 16)
- Updated LayoutPresets table with active segment definitions
- Updated LayoutChoices dropdown list
- Compatible with firmware v7.0.10+

---

## Files Modified

- `qsys-plugin/LEDMatrix_v7.qplug`
  - Updated PluginInfo to v7.0.13
  - Added presets 8 & 9 to LayoutPresets table
  - Added "8 - VO-left" and "9 - VO-right" to LayoutChoices
  - Updated LAYOUT PRESETS comment section

---

## Support

For issues or questions:
- Check firmware version (must be v7.0.10+)
- Verify UDP connectivity between Q-SYS and panel
- Check panel logs via SSH: `sudo journalctl -u led-matrix -f`
- Test layouts manually via UDP before using plugin

---

## Related Documentation

- `LAYOUTS-15-16-VO.md` - Detailed layout documentation
- `test-vo-layouts.sh` - Command-line test script
- `PR_CURTAIN_FRAME.md` - Pull request documentation
- Firmware v7.0.10 release notes
