# Q-SYS LED Matrix Plugin - Development Guide & Learnings

## Project Overview

**Plugin Name:** PKE LED Matrix Display  
**Current Version:** 3.11.0  
**Purpose:** Complete JSON UDP control for Olimex ESP32 Gateway 64x32 LED Matrix  
**Target Hardware:** Raspberry Pi Zero 2 W (10.1.1.24) with Waveshare RGB-Matrix-P4-64x32

---

## Development Journey & Key Learnings

### Critical Discovery: Q-SYS Plugin Architecture

From the [official Q-SYS plugin development guide](https://github.com/q-sys-community/q-sys-plugin-guide):

#### The Setup vs Runtime Division

**Setup Section** (compiled when plugin is dragged):
- `PluginInfo` - metadata and version info
- `GetProperties()` - plugin configuration properties
- `GetControls()` - control definitions (Name, Type, PinStyle, UserPin)
- `GetControlLayout()` - UI layout and **PrettyName for pin grouping**
- `GetPages()` - optional tabbed UI

**Runtime Section** (executed on every compile):
- Everything inside `if Controls then` block
- Event handlers, timers, UDP communication
- Can be updated without re-dragging plugin

#### Critical Rule
> "When a plugin is added to a schematic, the above areas are compiled into your program at that point in time. Any changes will not be reflected until either **deletion of the control**, or possibly a re-load of designer."

**Action Required:** Delete and re-drag plugin after changing GetControlLayout, GetControls, or PluginInfo.

### Pin Grouping in Control Pins Window

**Correct Method:**
```lua
function GetControlLayout(props)
    layout["control_name"] = {
        PrettyName = "GroupName~ControlName",  -- Tilde separator!
        Style = "Text",
        Position = {x, y},
        Size = {w, h}
    }
end
```

**Key Insights:**
- Use tilde (`~`) separator: `"Group~Item"`
- Set PrettyName in **GetControlLayout()**, NOT GetControls()
- Keep names short for Control Pins window readability (truncation issue)
- Final naming: `Conn~IP`, `Seg1~Text`, `Global~Bright` (≤15 chars total)

### Control Pin Visibility

```lua
{
    Name = "control_name",
    ControlType = "Text",
    UserPin = false,  -- false = always visible, true = user-selectable
    PinStyle = "Both"  -- "Input" | "Output" | "Both"
}
```

**Decision:** Set `UserPin = false` for all 52 pins to ensure always visible/connectable.

### Plugin Sizing Strategy

**Evolution:**
- v3.9.1: 1200px (too wide)
- v3.10.0: 1400px (even wider for pin names)
- v3.10.1: Shortened pin names instead
- v3.10.2: **600px final** (minimum for 6 combo boxes in row)

**Calculation for minimum width:**
```
Row 2 has 6 combo boxes with spacing:
114 + 6 + 114 + 6 + 84 + 6 + 68 + 6 + 78 + 6 + 96 = 584px
Add margins: ~590-600px minimum
```

---

## Current Plugin Architecture

### Control Pin Groups (52 total)

#### Connection Group (4 pins)
- `Conn~IP` - IP address input
- `Conn~Port` - UDP port input
- `Conn~Status` - Connection indicator LED
- `Conn~LastCmd` - Last command sent (status text)

#### Layout Group (2 pins)
- `Layout~Preset` - Layout selector with indexed options
- `Layout~Apply` - Manual apply button

#### Segment Groups (48 pins = 12 per segment × 4)
Each segment (Seg1, Seg2, Seg3, Seg4) has:
- `Active` - LED indicator (active/inactive)
- `Text` - Text input field
- `Send` - Manual display trigger
- `Invert` - Swap text/background colors
- `Clear` - Clear segment
- `TxtColor` - Text color selector
- `BgColor` - Background color selector
- `Font` - Font selector (Arial, Verdana, Digital, Mono)
- `Size` - Size selector (auto, 8, 12, 16, 24, 32)
- `Align` - Alignment (Left, Center, Right)
- `Effect` - Effect selector (none, scroll, blink, fade)
- `Intensity` - Brightness fader (0-255)

#### Global Group (2 pins)
- `Global~Bright` - Master brightness fader
- `Global~ClearAll` - Clear all segments button

### Indexed Layout System (v3.11.0)

**Integer Control Mapping:**
```
Main Layouts:
  1 → "1 - Fullscreen"
  2 → "2 - Vertical Split"
  3 → "3 - Horizontal Split"
  4 → "4 - QuadView"

Individual Fullscreen:
  11 → "11 - Fullscreen Segment 1"
  12 → "12 - Fullscreen Segment 2"
  13 → "13 - Fullscreen Segment 3"
  14 → "14 - Fullscreen Segment 4"
```

**Implementation Pattern:**
```lua
-- Map table for integer → full name
local LayoutIndexMap = {
    ["1"] = "1 - Fullscreen",
    ["2"] = "2 - Vertical Split",
    -- ... etc
}

-- Control handler accepts integer or full string
Controls.layout.EventHandler = function(ctl)
    local input = ctl.String
    local layoutName = LayoutIndexMap[input] or input
    if LayoutConfigs[layoutName] then
        Controls.layout.String = layoutName
        ApplyLayout(layoutName)
    end
end
```

**Usage:**
- From control system: Send integer `2` → instantly switches to Vertical Split
- From dropdown: Select "2 - Vertical Split" → applies immediately
- Auto-send enabled: Layout changes trigger segment reconfigurations

### Auto-Send with Debouncing

**Pattern for preventing rapid-fire commands:**
```lua
local debounceTimers = {}

local function Debounce(key, delay, fn)
    if debounceTimers[key] then debounceTimers[key]:Stop() end
    if not debounceTimers[key] then debounceTimers[key] = Timer.New() end
    local t = debounceTimers[key]
    t.EventHandler = function() t:Stop(); fn() end
    t:Start(delay)
end

-- Usage
Controls.seg1_text.EventHandler = function()
    if activeLayoutSegments[0] then
        Debounce("seg1_text", 0.5, function()
            local _, cmd = BuildTextCommand(0)
            SendCommand(cmd)
        end)
    end
end
```

### Color Invert Feature

**Implementation:**
```lua
Controls.seg1_invert.EventHandler = function()
    local textColorCtrl = Controls.seg1_text_color
    local bgColorCtrl = Controls.seg1_bg_color
    
    local temp = textColorCtrl.String
    textColorCtrl.String = bgColorCtrl.String
    bgColorCtrl.String = temp
    
    -- Auto-send if segment active
    if activeLayoutSegments[0] then
        local _, cmd = BuildTextCommand(0)
        SendCommand(cmd)
    end
end
```

---

## Best Practices Learned

### 1. Development Workflow
```bash
# Work directory
cd /Users/user/.openclaw/workspace/QSYS-LED-Matrix/qsys-plugin/

# Plugin location for testing
# macOS: ~/Documents/QSC/Q-Sys Designer/Plugins/
# Copy .qplug file to this directory

# After Setup changes (PluginInfo, GetControls, GetControlLayout):
# 1. Delete plugin from schematic
# 2. Re-drag from User Plugins
# 3. Test changes

# After Runtime changes (event handlers, logic):
# 1. Just re-compile (no re-drag needed)
```

### 2. Commit Message Format
```
<type>: <description in 50 chars>

Types: fix, feat, docs, style, refactor, chore
Examples:
  feat: Add indexed layout system with integer control
  fix: Correct text field width calculation
  docs: Add pin grouping instructions
```

### 3. Version Numbering
Pattern: `MAJOR.MINOR.PATCH`
- MAJOR: Breaking changes, complete rewrites
- MINOR: New features, layout changes
- PATCH: Bug fixes, small improvements

### 4. Testing Checklist
- [ ] Delete and re-drag plugin after Setup changes
- [ ] Test all 52 control pins for functionality
- [ ] Verify Control Pins window shows grouped names
- [ ] Test integer layout switching (1-4, 11-14)
- [ ] Test dropdown layout switching
- [ ] Verify auto-send with debouncing
- [ ] Test color invert on all segments
- [ ] Test manual Send/Clear buttons
- [ ] Verify UDP communication logs
- [ ] Test segment active/inactive states

---

## UDP Protocol Reference

**JSON Command Format:**
```json
{
  "cmd": "text",
  "seg": 0,
  "text": "Hello",
  "color": "FFFFFF",
  "bgcolor": "000000",
  "font": "arial",
  "size": "auto",
  "align": "C",
  "effect": "none",
  "intensity": 255
}
```

**Other Commands:**
```json
{"cmd": "clear", "seg": 0}
{"cmd": "clear_all"}
{"cmd": "brightness", "value": 200}
{"cmd": "config", "seg": 0, "x": 0, "y": 0, "w": 64, "h": 32}
```

---

## Known Issues & Solutions

### Issue: Control Pins not grouping
**Solution:** Delete plugin from schematic, re-drag fresh copy.

### Issue: Pin names truncated in Control Pins window
**Solution:** Keep PrettyName strings ≤15 chars total including tilde separator.

### Issue: Layout doesn't apply immediately
**Solution:** Use EventHandler on layout control itself, not just Apply button.

### Issue: Plugin width too narrow/wide
**Solution:** Calculate minimum based on widest row (600px for 6 combo boxes).

### Issue: Text field overlaps buttons
**Solution:** Use formula `W-332` for text width to ensure 300px for buttons + spacing.

---

## Future Enhancement Ideas

### Potential Features
- [ ] Preset manager (save/recall text + color combinations)
- [ ] Animation sequencer (timed layout transitions)
- [ ] Color picker with RGB hex input
- [ ] Font preview in dropdown
- [ ] Segment templates/macros
- [ ] Layout duplication tool
- [ ] Multi-segment text synchronization
- [ ] External data binding (Named Controls)
- [ ] Snapshot/restore functionality
- [ ] Layout transition effects

### Performance Optimizations
- [ ] Batch UDP commands (send multiple segments at once)
- [ ] Cache last-sent values to skip redundant commands
- [ ] Optimize Timer callbacks (reuse timer objects)
- [ ] Add connection pooling for UDP socket

---

## Resources

### Official Documentation
- [Q-SYS Plugin Development Guide](https://github.com/q-sys-community/q-sys-plugin-guide)
- [Q-SYS Help - Plugins](https://q-syshelp.qsc.com/)
- [Lua 5.3 Reference Manual](https://www.lua.org/manual/5.3/)

### Development Tools
- **IDE:** Visual Studio Code with extensions:
  - `vscode-lua` (trixnz.vscode-lua)
  - `markdownlint` (davidanson.vscode-markdownlint)
- **File Association:** Add to VS Code settings.json:
  ```json
  {
    "files.associations": {
      "*.qplug": "lua"
    }
  }
  ```
- **Formatting:** Shift-Alt-F before every commit

### Plugin Location
- **macOS:** `~/Documents/QSC/Q-Sys Designer/Plugins/`
- **Windows:** `%userprofile%\Documents\QSC\Q-Sys Designer\Plugins`

---

## Version History Summary

| Version | Key Changes |
|---------|-------------|
| 3.11.0 | Indexed layout system with integer control (1-4, 11-14) |
| 3.10.2 | Reduced width to 600px, optimized spacing |
| 3.10.1 | Shortened PrettyName strings for readability |
| 3.10.0 | Increased width to 1400px, verified grouping |
| 3.9.1 | Width to 1200px |
| 3.9.0 | All pins set to UserPin=false (always visible) |
| 3.8.0 | PrettyName grouping in GetControlLayout() |
| 3.7.0 | Clear naming without grouping attempt |
| 3.6.0 | First tilde grouping attempt |
| 3.5.0 | Auto-send on text change |

---

## Contact & Support

**Repository:** https://github.com/DHPKE/QSYS-LED-Matrix  
**Plugin Author:** DHPKE  
**License:** MIT

---

*Last Updated: February 19, 2026*  
*Plugin Version: 3.11.0*
