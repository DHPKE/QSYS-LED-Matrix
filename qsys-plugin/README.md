# Q-SYS Plugin Files

## ✅ WHICH FILE TO USE

**Use this file:** `LEDMatrix_Complete.qplug`

This is the **complete, production-ready plugin** (v3.11.0) ready to import into Q-SYS Designer.

**Features:**
- **Indexed Layout Control:** Send integers 1-4, 11-14 for instant layout switching
- **8 Layout Modes:** Fullscreen, Vertical/Horizontal Split, QuadView + 4 individual fullscreen modes
- **52 Control Pins:** Fully exposed and grouped (Connection, Layout, Seg1-4, Global)
- **Per-Segment Controls:** Text, colors, font, size, alignment, effects, intensity
- **Auto-Send:** Text changes trigger immediate display update (debounced)
- **Color Invert:** One-click swap of text/background colors per segment
- **Compact UI:** Optimized 600px width with all controls accessible
- **Real-time Status:** Active segment indicators, connection status, last command display

---

## 📁 Files in this Directory

### ✅ LEDMatrix_Complete.qplug
**Version:** 3.11.0  
**Status:** Production-ready  
**Format:** Q-SYS Designer plugin file (.qplug)  
**👉 Use this:** Import directly into Q-SYS Designer

### 📄 Documentation

- **DEVELOPMENT_GUIDE.md** - Complete development guide with Q-SYS learnings and patterns
- **GROUPING_INSTRUCTIONS.md** - How to see grouped control pins in Q-SYS Designer
- **PLUGIN_CHANGELOG.md** - Version history (v1.0 → v2.0 changes)
- **PLUGIN_REVIEW.md** - Legacy issues and fixes

### 📋 Legacy Files

- **OlimexLEDMatrix_v2.qplug** (v2.0.0) - Legacy version, use LEDMatrix_Complete.qplug instead
- **led_matrix_controller_v2.lua** (v2.0.0) - Legacy source
- **led_matrix_controller.lua** (v1.0.0) - Original version

---

## 🚀 Quick Start

1. **Download** `LEDMatrix_Complete.qplug`
2. **Open Q-SYS Designer**
3. **Drag** the .qplug file into your design (shows as "PKE LED Matrix Display")
4. **Configure Connection:**
   - IP address: 10.1.1.24 (or your Raspberry Pi IP)
   - UDP port: 21324
5. **Select Layout:**
   - From control pin: Send integer 1-4 or 11-14
   - From dropdown: Select "1 - Fullscreen", "2 - Vertical Split", etc.
   - Click "Apply Layout" button
6. **Use Segment Controls:**
   - Enter text in segment fields
   - Select colors, fonts, effects
   - Changes auto-send to active segments
   - Use "Display" button for manual send
   - Use "Invert" to swap colors
   - Use "Clear" to erase segment

---

## 🎯 Layout System

### Integer Control Mapping

Send these integers to the **Layout~Preset** pin for instant switching:

**Main Layouts:**
- `1` = Fullscreen (Segment 1 only)
- `2` = Vertical Split (Segments 1 & 2 side-by-side)
- `3` = Horizontal Split (Segments 1 & 2 stacked)
- `4` = QuadView (All 4 segments in 2×2 grid)

**Individual Fullscreen:**
- `11` = Fullscreen Segment 1
- `12` = Fullscreen Segment 2
- `13` = Fullscreen Segment 3
- `14` = Fullscreen Segment 4

### Layout Configurations

| Layout | Active Segments | Display Area |
|--------|----------------|--------------|
| 1 - Fullscreen | Seg 1 | Full 64×32 |
| 2 - Vertical Split | Seg 1, 2 | 32×32 each |
| 3 - Horizontal Split | Seg 1, 2 | 64×16 each |
| 4 - QuadView | Seg 1-4 | 32×16 each |
| 11-14 - Fullscreen SegX | Seg X | Full 64×32 |

---

## 📌 Control Pin Groups

All 52 pins are organized into groups in the Control Pins window:

**Connection** (4 pins)
- Conn~IP, Conn~Port, Conn~Status, Conn~LastCmd

**Layout** (2 pins)
- Layout~Preset, Layout~Apply

**Seg1, Seg2, Seg3, Seg4** (12 pins each)
- Active, Text, Send, Invert, Clear
- TxtColor, BgColor, Font, Size, Align, Effect, Intensity

**Global** (2 pins)
- Global~Bright, Global~ClearAll

### 🔄 To See Grouped Pins:
**Important:** After updating the plugin, you must:
1. Delete the old plugin from your schematic
2. Re-drag the new version from User Plugins
3. Open Properties → Control Pins to see groups

---

## 🎨 Available Options

### Colors
White, Red, Lime, Blue, Yellow, Magenta, Cyan, Orange, Purple, Green, Pink, Gold, Silver, Gray, Black

### Fonts
- Arial (default)
- Verdana
- Digital (7-segment style)
- Mono (monospace)

### Sizes
auto, 8, 12, 16, 24, 32

### Alignment
Left, Center, Right

### Effects
none, scroll, blink, fade

---

## 💡 Tips & Tricks

### Auto-Send Feature
Text fields have auto-send enabled with 500ms debounce. Changes are automatically sent to active segments.

### Color Invert
Quickly swap text and background colors using the "Invert" button per segment.

### Layout Presets
Use integer control (1-4, 11-14) for rapid layout switching from control systems or scripts.

### Offline Editing
Edit segment text/colors while inactive, then apply layout to display changes.

### Master Controls
- Use "Global~Bright" to adjust overall display brightness (0-255)
- Use "Global~ClearAll" to clear all segments at once

---

## 🔧 Technical Details

**Protocol:** JSON over UDP  
**Port:** 21324 (default)  
**Target:** Python LED matrix controller on Raspberry Pi  
**Matrix:** 64×32 pixels, HUB75 interface  

**Sample UDP Command:**
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

---

## 📚 Documentation

For detailed development information, see:
- **DEVELOPMENT_GUIDE.md** - Complete guide with Q-SYS patterns and best practices
- **GROUPING_INSTRUCTIONS.md** - Control pin grouping setup

---

## 🆕 Version History

| Version | Key Features |
|---------|-------------|
| 3.11.0 | Indexed layouts (1-4, 11-14), individual fullscreen modes |
| 3.10.2 | Compact 600px width, optimized spacing |
| 3.10.1 | Shortened pin names for readability |
| 3.9.0-3.10.0 | Pin grouping implementation, all pins always visible |
| 3.5.0-3.8.0 | Auto-send, layout selector, comprehensive controls |
| 2.0.0 | Effects, alignment, validation |
| 1.0.0 | Initial release |

---

**Always use v3.11.0 LEDMatrix_Complete.qplug for new projects!**

