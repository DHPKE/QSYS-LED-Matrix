# ListenVision LED Display Controller v2.0.0

QSYS Designer plugin for controlling ListenVision T/E/A/C series LED display cards via SDK protocol.

![Version](https://img.shields.io/badge/version-2.0.0-blue)
![QSYS](https://img.shields.io/badge/QSYS-Designer-green)
![Status](https://img.shields.io/badge/status-stable-brightgreen)

## Features

### 🎯 Display Control
- **4 Text Areas** with instant updates
- **Layout Presets**: Fullscreen, Half, Thirds
- **12+ Visual Effects** (Static default)
- **Frame & Background Colors**
- **Digital Clock** (5 formats)
- **Countdown Timer**
- **Curtain Overlay** (show/hide colored rectangle)

### 🎛️ Advanced Features
- **8 Groups + Broadcast** routing (mutual exclusion)
- **Auto-Send on Change** (no Send button needed)
- **Auto-Connect & Initialize** on startup
- **Display Rotation** (0°/90°/180°/270°)
- **Real-Time Connection Status**
- **Frame Color Linking** (auto-sync with text color)

### 📡 Communication
- TCP/IP (Fixed IP)
- UDP Broadcast
- Serial (RS232/RS485)

### 🖥️ Supported Hardware

**T-Series** (Single/Dual Color)
- T2, T4, T8, T16

**E-Series** (Single/Dual Color)
- E1, E2, E3, E5, E6

**A-Series** (Single/Dual Color)
- A4

**C-Series** (Full Color RGB)
- C2M, C4M, C2W, C4W, C2S, C4A, C8

## Installation

1. Download `LVLEDController_v02.qplug` from the [ListenVision folder](./ListenVision/)
2. Copy to your QSYS Designer plugin directory:
   - Windows: `C:\Users\<username>\Documents\QSC\Q-Sys Designer\Plugins\`
   - macOS: `~/Documents/QSC/Q-Sys Designer/Plugins/`
3. Restart QSYS Designer
4. Find plugin: **LED Display Controller SDK v2**

## Quick Start

1. **Add Plugin** to your design
2. **Configure Properties**:
   - LED Type (T/E/A/C series)
   - IP Address (e.g., 192.168.1.100)
   - Screen Width/Height (e.g., 128x32)
3. **Plugin auto-connects** on load
4. **Type text** → Display updates instantly!

## Usage Examples

### Basic Text Display
1. Enable Text Area 1
2. Type your message
3. Select "Static" effect (default)
4. Display updates immediately ✅

### Layout Presets
1. Select "Half Left" from Preset dropdown
2. Position and size auto-fill
3. Text appears in left half of screen

### Group Routing
1. Click "Group 1" button
2. All commands now route to Group 1
3. Multiple displays controlled independently

### Curtain Overlay
1. Set Curtain position and size
2. Choose color (e.g., Black)
3. Toggle "Show" → Area covered
4. Toggle off → Area revealed

## Configuration

### Screen Parameters
- **Width**: 8-2048 pixels
- **Height**: 8-2048 pixels
- **Color Type**: Mono / Dual / Full RGB

### Text Area Controls
- **Text**: Message content
- **Position**: X, Y coordinates
- **Size**: Width, Height in pixels
- **Layout Preset**: Quick sizing
- **Text Color**: 11 colors + custom
- **BG Color**: 12 colors
- **Font Size**: Adjustable
- **Effect**: 12+ transition styles
- **Speed**: 0-255 (0 = static)
- **Dwell**: Display time (seconds)
- **Frame**: Toggle + color + link

### Effects
- Static (visible, no animation)
- Scroll Left/Right/Up/Down
- Continuous scrolling
- Fade In
- Flash
- Curtain Open/Close
- And more...

## Protocol Details

**Binary Packet Format:**
```
[Header: 0xA5 0x5A] [Command] [Length: 16-bit LE] [Data] [Checksum: XOR]
```

**Commands Implemented:**
- `0x01`: Set Screen Parameters
- `0x02-0x04`: Program Create/Send/Delete
- `0x05`: Clear Screen
- `0x06`: Set Brightness
- `0x07`: Power Control
- `0x08`: Set Rotation
- `0x10`: Add Text Area
- `0x20`: Add Digital Clock
- `0x21`: Add Countdown Timer

**Group Routing:**
- Group byte added to all commands
- 0 = Broadcast (All)
- 1-8 = Specific groups

## Changelog

See [CHANGELOG.md](./CHANGELOG.md) for detailed version history.

### Version 2.0.0 Highlights
- ✅ Auto-send on change (instant updates)
- ✅ Startup initialization (ready immediately)
- ✅ Layout presets (quick sizing)
- ✅ Group routing (8 groups + broadcast)
- ✅ Frame & background colors
- ✅ Curtain overlay feature
- ✅ Static effect default
- ✅ Fixed BG color dropdown

## Troubleshooting

**Plugin doesn't connect:**
- Check IP address is correct
- Verify LED controller is powered on
- Ensure network connectivity
- Check firewall settings

**Text doesn't appear:**
- Enable the text area (blue button)
- Check text color isn't same as background
- Verify screen dimensions are correct
- Ensure brightness is > 0

**Status shows Orange/Red:**
- Orange: Connecting... (wait 1-2 seconds)
- Red: Connection failed (check IP/network)
- Green: Connected ✅

**Changes don't update:**
- Area must be enabled (blue button)
- Check connection status (should be green)
- Verify controller is responding

## SDK Documentation

SDK documentation and examples located in:
```
/Users/user/Desktop/DöDI_QSYS/单双色及门楣全彩开发资料1.0.6_20241209/
```

## Development

**Repository**: https://github.com/DHPKE/QSYS-LED-Matrix/

**Structure:**
```
ListenVision/
├── LVLEDController_v01.qplug  # v1 (bug fixes only)
├── LVLEDController_v02.qplug  # v2 (current release)
└── README.md
```

**Contributing:**
- Report issues on GitHub
- Submit pull requests for improvements
- Follow existing code style

## Credits

- **Plugin Author**: DHPKE
- **SDK**: ListenVision Technology
- **Platform**: QSC Q-SYS Designer

## License

(Specify license if applicable)

## Support

For questions or support:
- GitHub Issues: https://github.com/DHPKE/QSYS-LED-Matrix/issues
- SDK Documentation: See `/001 SDK & Demo/` folder

---

**Version**: 2.0.0  
**Release Date**: 2026-02-25  
**Plugin ID**: `pke.led.controller.sdk.2.0.0`
