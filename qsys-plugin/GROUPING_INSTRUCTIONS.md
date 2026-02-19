# Q-SYS Plugin Control Pin Grouping Instructions

## v3.10.0 - Properly Grouped Control Pins

Your LED Matrix plugin now has **properly grouped control pins** according to the official Q-SYS plugin development guidelines.

## Pin Groups

All 52 control pins are organized into these groups in the Q-SYS Designer Control Pins window:

### Connection Group (4 pins)
- Connection~IP Address
- Connection~Port  
- Connection~Status
- Connection~Last Command

### Layout Group (2 pins)
- Layout~Preset
- Layout~Apply

### Segment 1 Group (12 pins)
- Segment 1~Active
- Segment 1~Text
- Segment 1~Send
- Segment 1~Invert
- Segment 1~Clear
- Segment 1~Text Color
- Segment 1~BG Color
- Segment 1~Font
- Segment 1~Size
- Segment 1~Align
- Segment 1~Effect
- Segment 1~Intensity

### Segment 2, 3, 4 Groups (12 pins each)
- Same structure as Segment 1, repeated for each segment

### Global Group (2 pins)
- Global~Brightness
- Global~Clear All

## ⚠️ CRITICAL: How to See Grouped Pins

According to the [official Q-SYS plugin development guide](https://github.com/q-sys-community/q-sys-plugin-guide):

> "When a plugin is added to a schematic, the setup sections (PluginInfo, GetProperties, GetControls, GetControlLayout) are **compiled into your program at that point in time**. Any changes will not be reflected until either **deletion of the control**, or possibly a re-load of designer."

### To Apply the Grouped Pins:

1. **Open your Q-SYS Design file**
2. **Delete the old LED Matrix plugin** from your schematic (the one you placed before this update)
3. **Drag the plugin again** from the User Plugins section
4. **Open the Properties window** → Control Pins section
5. You should now see collapsible groups: Connection, Layout, Segment 1-4, Global

### If Groups Still Don't Appear:

1. Close Q-SYS Designer completely
2. Reopen Q-SYS Designer
3. Delete and re-drag the plugin again

## Technical Implementation

The grouping uses the `PrettyName` property with tilde (`~`) separator in `GetControlLayout()`:

```lua
layout["seg1_text"] = {
    PrettyName = "Segment 1~Text",  -- Group Name ~ Control Name
    Style = "Text",
    Position = {x, y},
    Size = {w, h}
}
```

This is the official Q-SYS method for organizing control pins into collapsible groups.

## Plugin Dimensions

- **Width**: 1400px (increased from 1200px for better label visibility)
- **Height**: Dynamic based on segments and controls

All pins are set to `UserPin = false`, meaning they are always visible and available for connection (not user-selectable).
