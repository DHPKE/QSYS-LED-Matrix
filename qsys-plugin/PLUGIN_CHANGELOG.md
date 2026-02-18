# Q-SYS Plugin Changelog

## Version 3.5.0 (2026-02-18) - Auto-Send & Inactive Segment Protection

### ✅ NEW FEATURES

#### 1. **Text Auto-Send**
- Text input now automatically sends to display when changed
- No need to press "Display" button after typing
- Updates happen in real-time as you type
- Only sends if segment is active (respects layout)

### 🔧 IMPROVEMENTS

#### 2. **Inactive Segment Protection**
- Editing inactive segments no longer affects the display
- When in Fullscreen layout, changing segments 2-4 has no effect
- All property changes (text, color, font, etc.) respect active/inactive state
- Prevents accidental rendering of hidden segments

#### 3. **Enhanced Comments**
- Clarified that AutoSend only affects active segments
- Added documentation for manual Display/Clear button behavior
- Better code organization and readability

---

## Version 2.0.0 (2026-02-17) - Complete Overhaul

### ✅ NEW FEATURES

#### 1. **Effect Control**
- Added Effect selector for each segment
- Choices: none, scroll, blink
- Note: fade/rainbow declared in firmware but not implemented yet

#### 2. **Alignment Control**
- Added Alignment selector for each segment
- Choices: L (Left), C (Center), R (Right)
- Previously hardcoded to Center

#### 3. **Font Size Control**
- Added Size selector for each segment
- Choices: auto, 8, 12, 16, 24, 32 pixels
- Previously hardcoded to "auto"

#### 4. **Proper Font ComboBox**
- Fixed font control (was Text, now proper ComboBox)
- Choices: roboto8, roboto12, roboto16, roboto24
- Matches firmware font options

#### 5. **Segment Active Indicators**
- Added LED indicator for each segment
- Shows green when segment has active text
- Turns off when segment cleared

#### 6. **Last Command Display**
- Shows last sent UDP command
- Helps debugging and confirms commands sent
- Shows errors if command fails

---

### 🔧 IMPROVEMENTS

#### 7. **UDP Error Handling**
- Added pcall wrapping for socket operations
- Connection status updates on errors
- Clear error messages in log

#### 8. **Color Validation**
- Validates hex colors before sending
- Removes # if present
- Accepts 3 or 6 character hex codes
- Shows error if invalid

#### 9. **Port Number Validation**
- Validates port range (1-65535)
- Shows error if invalid
- Prevents crash from bad port values

#### 10. **Brightness Debounce**
- 500ms debounce on brightness slider
- Prevents UDP flood while dragging
- Only sends when slider stops moving

#### 11. **Socket Cleanup**
- Added Cleanup() function
- Properly closes socket on plugin unload
- Cancels pending timers

#### 12. **Better Logging**
- Clear startup banner
- ✓/✗ symbols for success/error
- → symbol for sent commands
- More informative messages

---

### 🎨 UI IMPROVEMENTS

#### 13. **Updated Colors**
- Bright grey backgrounds ({140-160}) for text contrast
- Matches QSYS-WagoTCP750 v2.3.6 and QSYS-Group-Matrix v2.3.0
- Better readability in Q-SYS Designer dark theme

#### 14. **Improved Layout**
- Wider controls for better visibility
- Better organized rows
- Active LED indicator placement
- 800px width (was 600px)

#### 15. **Better Button Colors**
- Send: Green ({0, 180, 80})
- Clear: Red ({200, 60, 60})
- More visually distinct

---

## Comparison: v1.0.0 → v2.0.0

### Controls Added:
| Control | v1.0 | v2.0 |
|---------|------|------|
| **Effect selector** | ❌ Hardcoded "none" | ✅ ComboBox (none/scroll/blink) |
| **Align selector** | ❌ Hardcoded "C" | ✅ ComboBox (L/C/R) |
| **Size selector** | ❌ Hardcoded "auto" | ✅ ComboBox (auto/8/12/16/24/32) |
| **Font selector** | ⚠️ Text field | ✅ Proper ComboBox |
| **Active indicator** | ❌ None | ✅ LED per segment |
| **Last command** | ❌ None | ✅ Text display |

### Features Added:
| Feature | v1.0 | v2.0 |
|---------|------|------|
| **UDP error handling** | ❌ No | ✅ Yes (pcall) |
| **Color validation** | ❌ No | ✅ Yes (hex check) |
| **Port validation** | ❌ No | ✅ Yes (1-65535) |
| **Brightness debounce** | ❌ No (floods UDP) | ✅ Yes (500ms) |
| **Socket cleanup** | ❌ No | ✅ Yes (Cleanup function) |
| **Status feedback** | ⚠️ Minimal | ✅ Comprehensive |

### UI Updates:
| Aspect | v1.0 | v2.0 |
|--------|------|------|
| **Background colors** | Dark grey | Bright grey (contrast) |
| **Width** | 600px | 800px |
| **Layout** | Basic | Organized rows |
| **Segment height** | 110px | 130px (more space) |
| **Logging** | Basic | ✓/✗ symbols |

---

## Files

**v1.0.0:** `led_matrix_controller.lua` (10KB, 340 lines)  
**v2.0.0:** `led_matrix_controller_v2.lua` (19KB, 620 lines)

---

## Firmware Compatibility

**v1.0.0:** Works with firmware v1.0.0+  
**v2.0.0:** Optimized for firmware v1.2.0+

### Command Format:
```
TEXT|segment|content|color|font|size|align|effect
```

**v1.0 sent:**
```
TEXT|0|Hello|FFFFFF|roboto12|auto|C|none
```

**v2.0 sends (with user control):**
```
TEXT|0|Hello|FF0000|roboto16|24|R|scroll
```

---

## Upgrade Guide

### From v1.0.0 to v2.0.0:

1. **Replace plugin file:**
   - Old: `led_matrix_controller.lua`
   - New: `led_matrix_controller_v2.lua`

2. **Update plugin ID:**
   - Old: `dhpke.olimex.led.matrix.1.0.0`
   - New: `dhpke.olimex.led.matrix.2.0.0`

3. **Re-wire any existing connections** (control names unchanged)

4. **New controls available:**
   - Effect selectors (per segment)
   - Alignment selectors (per segment)
   - Size selectors (per segment)
   - Active indicators (per segment)
   - Last command display

5. **Test UDP connection:**
   - Check connection status indicator
   - Watch "Last Command" field
   - Monitor Q-SYS debug output

---

## Known Limitations

1. **Fade/Rainbow effects** - Declared in firmware but not implemented (v1.2.0)
2. **No command queue** - Rapid commands might overrun UDP buffer
3. **No segment preview** - Can't see what's on matrix without looking at it
4. **No persistent config** - Firmware doesn't save segment state (defined but unused)

---

## Next Version Ideas (v2.1.0)

- [ ] Add command queue with rate limiting
- [ ] Add segment preview images
- [ ] Add preset text templates
- [ ] Add bulk send (all segments at once)
- [ ] Add fade/rainbow effects when firmware supports them
- [ ] Add SAVE/RECALL presets

---

**Status:** v2.0.0 Ready for Testing  
**Recommended:** Use with QSYS-LED-Matrix firmware v1.2.0+
