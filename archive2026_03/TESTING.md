# Q-SYS LED Matrix Plugin v7.1.0 - Testing Guide

## Installation
1. Open Q-SYS Designer
2. Go to Tools → Show Design Resources
3. Navigate to Plugins folder
4. Copy `LEDMatrix_v7.qplug` to the Plugins directory
5. Restart Q-SYS Designer if already running

## Loading the Plugin
1. Create a new design or open existing
2. Open the Schematic view
3. Right-click → Insert → Plugins → User Plugins
4. Select "PKE~LED Matrix Display (v7 Curtain Mode)"
5. Plugin should appear with ID: `dhpke.olimex.led.matrix.7.1.0`

## Testing Tabs

### Expected Behavior
- **5 tabs should be visible** at the top: Connection, Segments, Layout, Global, Curtain
- Clicking each tab should show only controls for that section
- State should persist when switching tabs (text fields retain values)

### Tab 0: Connection
**Should display:**
- IP Address text field
- Port text field
- Reconnect button
- Status indicator
- Last Command text field

**Test:**
- Enter IP address (e.g., 192.168.1.100)
- Change port number
- Click Reconnect - should attempt connection
- Status should show green (connected) or red (disconnected)

### Tab 1: Segments
**Should display:**
- 4 segment rows (Seg1, Seg2, Seg3, Seg4)
- Each row has:
  - Active LED indicator
  - Text field
  - Display, Invert, Clear buttons
  - Text Color, BG Color, Font, Size, Align, Effect dropdowns
  - Frame toggle, Frame Color dropdown, Link button

**Test:**
- Enter text in Seg1
- Select colors/fonts
- Click Display - text should be sent to panel
- LED indicator should light up
- Clear button should turn off LED

### Tab 2: Layout
**Should display:**
- Group routing buttons: All, 1, 2, 3, 4, 5, 6, 7, 8 (colored buttons)
- Layout preset thumbnails (7 main presets + 4 segment-specific)
- Layout preset dropdown
- Apply Layout button

**Test:**
- Click group buttons - only one should be active at a time
- Select a layout preset from dropdown
- Click Apply Layout - should send layout command
- Thumbnails should show visual preview of each layout

### Tab 3: Global
**Should display:**
- Brightness text field (0-255)
- Rotation dropdown (0, 90, 180, 270)
- Apply Rotation button
- Display toggle button
- Test Mode toggle button
- Reboot button
- Clear All button

**Test:**
- Change brightness value - should send command
- Select rotation angle, click Apply
- Toggle Display - should enable/disable panel
- Click Reboot - should send HTTP command

### Tab 4: Curtain
**Should display:**
- Group text field (1-8)
- Color dropdown
- Apply Config button
- Show/Hide toggle button

**Test:**
- Enter group number (1-8)
- Select curtain color
- Click Apply Config - should configure curtain for that group
- Toggle Show/Hide - should control curtain visibility

## Common Issues

### Plugin won't load
- Check Q-SYS Designer version (must support Lua plugins)
- Verify file is in correct Plugins directory
- Look for syntax errors in Debug window

### Tabs not showing
- Ensure `GetPages()` function is present in plugin
- Check that `props["page_index"]` is being read correctly
- Verify no Lua syntax errors

### Controls not working
- Check Debug Output window for error messages
- Verify UDP socket connection (Connection tab status)
- Test with local IP first (127.0.0.1 if running on same machine)

### State not persisting
- This is expected - controls retain their values even when switching tabs
- All controls are defined in `GetControls()` which maintains state
- Only the layout visibility changes per tab

## Verification Checklist

✅ Plugin loads without errors  
✅ All 5 tabs are visible  
✅ Tab switching works smoothly  
✅ Connection tab renders correctly  
✅ Segments tab shows all 4 segment rows  
✅ Layout tab shows group buttons and thumbnails  
✅ Global tab shows all control buttons  
✅ Curtain tab shows group/color/apply/enable  
✅ State persists when switching tabs  
✅ All functionality from v7.0.5 works identically  

## Success Criteria
- No error messages in Debug Output
- UI is clean and organized
- All controls respond to input
- Tab navigation is intuitive
- Functionality matches v7.0.5 exactly

## File Location
Plugin file: `/Users/user/.openclaw/workspace/QSYS-LED-Matrix/qsys-plugin/LEDMatrix_v7.qplug`

## Next Steps
After successful testing:
1. Tag commit as v7.1.0
2. Update documentation
3. Deploy to production Q-SYS systems
