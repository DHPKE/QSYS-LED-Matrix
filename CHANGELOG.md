# QSYS LED Display Controller - Changelog

## Version 2.0.0 (2026-02-25)

### 🎉 Major Release - Complete Rewrite with Advanced Features

#### New Features

**Group Routing System**
- 9 group buttons: All (broadcast) + Groups 1-8
- Color-coded buttons for visual distinction
- Mutual exclusion (only one group active at a time)
- Active group display indicator
- Group routing byte added to all command packets

**Frame & Background Colors**
- Frame toggle for each text area
- Frame color dropdown with full color palette
- Background color dropdown (12 colors)
- Frame link toggle: auto-syncs frame color with text color
- Frame controls integrated into top row of text area UI

**Curtain Overlay**
- Boolean show/hide control (toggle button)
- Custom position (X, Y) and size (Width, Height)
- Background color selection
- Perfect for transitions, blackouts, or masking areas
- Uses special area index 254 in protocol

**Layout Presets**
- 7 preset options for quick sizing:
  - Custom (manual control)
  - Fullscreen (100% width × 100% height)
  - Half Left (50% width × 100% height)
  - Half Right (50% width × 100% height)
  - Third 1 (33% width × 100% height, left)
  - Third 2 (33% width × 100% height, middle)
  - Third 3 (33% width × 100% height, right)
- Automatically calculates and applies X, Y, Width, Height values
- Based on configured screen dimensions

**Auto-Send on Change**
- Text changes update display immediately
- All parameter changes trigger instant updates
- No manual Send button press required
- Event handlers on all text area controls:
  - Text content
  - Position (X, Y) and Size (Width, Height)
  - Colors (text, background, frame)
  - Font size
  - Effects, speed, dwell time
  - Frame settings

**Startup Initialization**
- Auto-connects on plugin load (0.5s delay)
- Sends all configuration automatically (1.0s after connection)
- Staggered command sequence:
  1. Screen parameters
  2. Brightness
  3. Rotation (if not 0°)
  4. All enabled text areas (0.1s intervals)
  5. Clock (if enabled)
  6. Timer (if enabled)
  7. Curtain (if enabled)
- Display ready immediately - no manual intervention needed

**Display Rotation**
- Four rotation options: 0°, 90°, 180°, 270°
- Rotates entire display (not individual text areas)
- Auto-applies on selection change

#### UI Improvements

**Text Area Layout**
- Reorganized for better workflow:
  - Row 1: Text input | Enable | Frame | Frame Color | Link
  - Row 2: X | Y | Width | Height | Preset | Text Color | BG Color | Font
  - Row 3: Effect | Speed | Dwell
- Text input width optimized (W-462)
- Frame controls on right side for easy access
- Proper spacing throughout - no overlapping elements
- Text area height: 158px (compact and efficient)

**Connection Status**
- Real-time feedback with color coding:
  - 🔴 Red (Fault): Not connected / error
  - 🟠 Orange (Connecting): Connection in progress
  - 🟢 Green (OK): TCP connected
  - 🟡 Yellow (Compromised): UDP (connectionless)
- TCP event handlers (Connected, Reconnected, Closed, Error, Timeout)
- Accurate status display

**Group Routing UI**
- Dedicated section with 9 buttons
- Color-coded for visual identification:
  - Gray (All/Broadcast)
  - White, Yellow, Orange, Red, Magenta, Blue, Cyan, Green (Groups 1-8)
- Active group display indicator

**Curtain UI**
- Single compact row (68px height)
- Show/Hide toggle (orange)
- Position/Size controls
- Color dropdown on right

#### Default Values Changed

- **Effect**: Static (was Scroll Left/Immediate)
  - Text visible immediately with no animation
- **Speed**: 0 (was 50)
  - No movement by default
- **Background Color**: Black
- **Frame Color**: White

#### Removed Features

- QR Code functionality (not needed for this use case)
  - Removed QR controls, functions, and UI section
  - Simplified interface

#### Bug Fixes

**Syntax Errors**
- Fixed orphaned `end` statement (line 804)
- Removed duplicate function definitions
- Fixed function closure issues from sed insertions
- Plugin now imports cleanly without errors

**UI Overlapping**
- Fixed Connection & Screen Parameters section (84px → 110px)
- Fixed Display Rotation section (60px → 74px)
- Fixed text area spacing (gaps increased to 6px)
- Fixed Effect/Speed/Dwell row positioning
- No more overlapping with group borders

**Dropdown Initialization**
- Background color dropdown now shows all choices
- Frame color dropdown properly initialized
- Color palettes consistent throughout

#### Technical Changes

**Protocol Enhancements**
- Group routing byte added to text area packets
- Curtain uses area index 254
- Frame data fields added to text area command
- Background color field added to text area command

**Event System**
- Auto-send event handlers on all controls
- Frame link toggle with color sync
- Layout preset calculation and application
- Group mutual exclusion with Timer.CallAfter
- Connection check (`if socket then`) before sending

**Initialization**
- Auto-connect on startup
- Comprehensive settings transmission
- Staggered sends to avoid overwhelming controller
- Default values for all controls

#### Files

- `LVLEDController_v01.qplug`: Latest v1 with all bug fixes
- `LVLEDController_v02.qplug`: New v2.0.0 release with all features

---

## Version 1.0.0 (2026-02-24)

### Initial Release

#### Features

- TCP/UDP/Serial communication support
- 4 text areas with customizable position and size
- 16+ transition effects (scroll, fade, curtain, etc.)
- Digital clock with 5 time/date formats
- Countdown timer
- Multiple color support (single/dual/full RGB)
- Brightness control (0-255)
- Power toggle
- Program management (create, send, delete)
- Display rotation (0°/90°/180°/270°)

#### Supported Hardware

- T-series: T2, T4, T8, T16 (single/dual color)
- E-series: E1, E2, E3, E5, E6 (single/dual color)
- A-series: A4 (single/dual color)
- C-series: C2M, C4M, C2W, C4W, C2S, C4A, C8 (full color RGB)

#### Protocol

- Binary packet format
- Header: 0xA5 0x5A
- Commands: Screen params, program management, text areas, clock, timer
- XOR checksum validation

---

## Development History

### Key Commits

```
6c4100b Add layout presets and fix BG color dropdown
1bed194 Fix: Set Static effect as default for all areas
0d4f858 v2.0.0: Add auto-send on change and startup initialization
c76cfe9 Release v2.0.0 with all new features
1672ef8 Fix Curtain layout: move Color dropdown to right
c2ab838 Add Curtain overlay area with boolean control
d931b60 Reorganize text area layout: frame controls on right
3a6669e Clean QR code removal (properly)
757cb6c Fix syntax error: remove orphaned 'end' and QR function
f656edf Complete runtime implementation: groups, frames, bg colors
c2d79f5 Major update: Add grouping, frame/bg colors, remove QR
a59faad Fix connection status feedback to show real connection state
8fa0b98 Fix UI element overlapping issues
fa547ec Add display rotation and blue enable buttons
31504e1 Fix overlapping fields in Connection & Screen Parameters
cd1ce7c Initial plugin creation
```

### Total Development Time
- Started: 2026-02-24
- Released v2.0.0: 2026-02-25
- Duration: ~2 days
- Commits: 15+

---

**Repository**: https://github.com/DHPKE/QSYS-LED-Matrix/
**Plugin Path**: `/ListenVision/LVLEDController_v02.qplug`
**Author**: DHPKE
**License**: (specify if applicable)
