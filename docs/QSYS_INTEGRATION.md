# Q-SYS Integration Guide

## Overview

This guide covers integration of the Olimex LED Matrix Text Display with QSC Q-SYS audio/video control systems using the provided Lua plugin.

## Prerequisites

- Q-SYS Designer software (version 8.0 or later)
- Q-SYS Core processor
- Olimex LED Matrix hardware configured and connected to network
- LED Matrix IP address and UDP port (default: 21324)

## Plugin Installation

### Step 1: Locate the Plugin File

The plugin file is located at:
```
qsys-plugin/led_matrix_controller.lua
```

### Step 2: Install in Q-SYS Designer

**Method 1: User Plugin Folder**
1. Open Q-SYS Designer
2. Go to `Tools` → `Show User Plugin Folder`
3. Copy `led_matrix_controller.lua` to this folder
4. Restart Q-SYS Designer

**Method 2: Drag and Drop**
1. Open your Q-SYS design file (.qsys)
2. Drag `led_matrix_controller.lua` directly into the schematic view
3. Q-SYS will automatically install and instantiate the plugin

### Step 3: Verify Installation

1. In Q-SYS Designer, open the `Schematic` view
2. Click on `Plugins` in the left pane
3. Look for "Olimex~LED Matrix Text Display"
4. Plugin should appear in the list

## Plugin Configuration

### Adding Plugin to Design

1. **Drag plugin from library**:
   - Navigate to Plugins section
   - Find "Olimex~LED Matrix Text Display"
   - Drag onto schematic

2. **Configure properties**:
   - Right-click plugin → Properties
   - Set IP Address (e.g., "192.168.1.100")
   - Set UDP Port (default: 21324)
   - Set Number of Segments (1-4, default: 4)

3. **Save design**:
   - File → Save
   - Name your design file

### Plugin Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| IP Address | String | 192.168.1.100 | IP address of LED Matrix device |
| UDP Port | Integer | 21324 | UDP port for commands |
| Number of Segments | Integer | 4 | Number of text segments to control (1-4) |

## Using the Plugin

### User Interface Layout

The plugin provides controls for each segment:

**Segment Controls (repeated for each segment):**
- **Text** - Text input field for message content
- **Color (Hex)** - Color picker/hex input (RRGGBB format)
- **Font** - Font selection dropdown
- **Send Button** - Sends text to matrix
- **Clear Button** - Clears segment

**Global Controls:**
- **IP Address** - Network address input
- **UDP Port** - Port number input
- **Status Indicator** - Connection status (green=OK, red=fault)
- **Brightness** - Fader for display brightness (0-255)
- **Clear All** - Button to clear all segments

### Basic Operation

#### Displaying Text on Segment 0

1. Click on plugin to open User Control Interface (UCI)
2. In "Segment 0" section:
   - Enter text in "Text" field (e.g., "Hello World")
   - Set color in "Color (Hex)" field (e.g., "FFFFFF" for white)
   - Select font from "Font" dropdown (e.g., "roboto12")
3. Click "Send" button
4. Text should appear on LED matrix within 50ms

#### Changing Display Brightness

1. Move "Brightness" fader
2. Value ranges from 0 (off) to 255 (maximum)
3. Default is 128 (50% brightness)
4. Changes are applied immediately

#### Clearing Display

- Click "Clear" button for specific segment
- Or click "Clear All" to clear entire display

### Advanced Features

#### Multi-Segment Display

Example: Split display into top and bottom sections

1. **Configure Segment 1** (Top):
   ```
   Text: "ROOM A"
   Color: 00FF00 (green)
   Font: roboto12
   ```
   Click "Send"

2. **Configure Segment 2** (Bottom):
   ```
   Text: "AVAILABLE"
   Color: FFFF00 (yellow)
   Font: roboto12
   ```
   Click "Send"

Result: Two lines of text, different colors

#### Using Named Controls

The plugin exposes named controls that can be used in scripts:

```lua
-- Control names for Segment 0:
text_0         -- Text input
color_0        -- Color input
font_0         -- Font selection
send_0         -- Send trigger
clear_0        -- Clear trigger

-- Global controls:
brightness     -- Brightness knob (0-255)
clear_all      -- Clear all trigger
connection_status  -- Status indicator
```

## Control Script Examples

### Example 1: Automatic Room Status Display

```lua
-- Script to update LED matrix based on room status

function UpdateRoomStatus(room_name, is_available)
    -- Set text
    Controls.text_0.String = room_name
    
    -- Set color based on availability
    if is_available then
        Controls.color_0.String = "00FF00"  -- Green
    else
        Controls.color_0.String = "FF0000"  -- Red
    end
    
    -- Set font
    Controls.font_0.String = "roboto16"
    
    -- Trigger send
    Controls.send_0.Boolean = true
    Controls.send_0.Boolean = false
end

-- Usage:
UpdateRoomStatus("Conference Room A", true)   -- Shows green
UpdateRoomStatus("Conference Room A", false)  -- Shows red
```

### Example 2: Display Meeting Timer

```lua
-- Display countdown timer on LED matrix

meeting_time_remaining = 30 * 60  -- 30 minutes in seconds

Timer = Timer.New()
Timer.EventHandler = function()
    meeting_time_remaining = meeting_time_remaining - 1
    
    if meeting_time_remaining <= 0 then
        Timer:Stop()
        Controls.text_0.String = "MEETING ENDED"
        Controls.color_0.String = "FF0000"  -- Red
    else
        -- Format time as MM:SS
        local minutes = math.floor(meeting_time_remaining / 60)
        local seconds = meeting_time_remaining % 60
        Controls.text_0.String = string.format("%02d:%02d", minutes, seconds)
        
        -- Change color based on time remaining
        if meeting_time_remaining > 300 then  -- > 5 min
            Controls.color_0.String = "00FF00"  -- Green
        elseif meeting_time_remaining > 60 then  -- > 1 min
            Controls.color_0.String = "FFFF00"  -- Yellow
        else
            Controls.color_0.String = "FF0000"  -- Red
        end
    end
    
    Controls.send_0.Boolean = true
    Controls.send_0.Boolean = false
end

Timer:Start(1)  -- Update every second
```

### Example 3: Temperature Display

```lua
-- Display temperature from sensor

function DisplayTemperature(temp_f)
    -- Format temperature string
    local temp_str = string.format("%.1f°F", temp_f)
    Controls.text_1.String = temp_str
    
    -- Color based on temperature
    if temp_f < 65 then
        Controls.color_1.String = "00FFFF"  -- Cyan (cold)
    elseif temp_f > 78 then
        Controls.color_1.String = "FF0000"  -- Red (hot)
    else
        Controls.color_1.String = "00FF00"  -- Green (comfortable)
    end
    
    Controls.font_1.String = "digital12"
    Controls.send_1.Boolean = true
    Controls.send_1.Boolean = false
end

-- Connect to temperature sensor control
Controls.room_temp.EventHandler = function(ctl)
    DisplayTemperature(ctl.Value)
end
```

### Example 4: Multi-Source Display

```lua
-- Show different info on multiple segments

function UpdateDisplay()
    -- Segment 0: Room name
    Controls.text_0.String = Properties["Room Name"].Value
    Controls.color_0.String = "FFFFFF"
    Controls.send_0.Boolean = true
    Controls.send_0.Boolean = false
    
    -- Segment 1: Current source
    local source = Controls.source_selector.String
    Controls.text_1.String = "Source: " .. source
    Controls.color_1.String = "00FF00"
    Controls.send_1.Boolean = true
    Controls.send_1.Boolean = false
    
    -- Segment 2: Volume level
    local volume = math.floor(Controls.master_volume.Value)
    Controls.text_2.String = "Vol: " .. volume
    Controls.color_2.String = "00FFFF"
    Controls.send_2.Boolean = true
    Controls.send_2.Boolean = false
end

-- Call when any relevant control changes
Controls.source_selector.EventHandler = UpdateDisplay
Controls.master_volume.EventHandler = UpdateDisplay
```

## UCI Integration

### Creating Touch Panel Interface

1. **Open UCI Designer**:
   - In Q-SYS Designer, click "UCI" tab
   - Select or create a new UCI page

2. **Add Plugin Controls**:
   - Drag text input boxes for each segment
   - Add color picker buttons
   - Add send/clear buttons
   - Add brightness slider

3. **Link to Plugin**:
   - Right-click control → "Link to Named Control"
   - Select LED Matrix plugin controls
   - Apply and save

### Example UCI Layout

```
┌─────────────────────────────────┐
│   LED MATRIX CONTROLLER         │
├─────────────────────────────────┤
│                                 │
│  Segment 1: [____________]      │
│  Color: [🎨]  [SEND] [CLEAR]   │
│                                 │
│  Segment 2: [____________]      │
│  Color: [🎨]  [SEND] [CLEAR]   │
│                                 │
│  Brightness: [━━━━━━━━━━━] 50% │
│                                 │
│  [CLEAR ALL SEGMENTS]           │
└─────────────────────────────────┘
```

## Network Configuration

### Static IP Recommendation

For reliable operation, assign a static IP to the LED Matrix:

1. Access your router/DHCP server
2. Find Olimex ESP32 Gateway MAC address
3. Create DHCP reservation
4. Or configure static IP on ESP32 (via web interface)

### Firewall Rules

Ensure Q-SYS Core can reach LED Matrix:

1. Allow outbound UDP on port 21324
2. No inbound ports needed
3. Test connectivity: ping LED Matrix IP from Core

### Network Topology

```
Q-SYS Core
    │
    │ UDP Port 21324
    ├──────────────┐
    │              │
Q-SYS Network    Ethernet
    │            Switch
    │              │
    │         LED Matrix
    │         (Olimex ESP32)
```

## Troubleshooting

### Plugin Not Sending Commands

**Check 1: Network Connectivity**
```lua
-- Add to script
print("Sending to: " .. Controls.ip_address.String .. ":" .. Controls.udp_port.String)
```

**Check 2: UDP Socket Status**
- Verify plugin initialized (check debug log)
- Check Q-SYS Core network settings
- Ping LED Matrix from Core

**Check 3: Firewall**
- Disable firewall temporarily to test
- Add UDP exception for port 21324

### Text Not Displaying

**Check 1: Command Format**
- View Serial Monitor on ESP32
- Verify commands are received
- Check for parsing errors

**Check 2: Segment Selection**
- Verify correct segment ID (0-3)
- Ensure segments don't overlap

**Check 3: LED Matrix**
- Test with web interface
- Verify matrix hardware is functioning

### Status Indicator Shows Fault

**Red Status** indicates:
- UDP socket failed to open
- Network unreachable
- Incorrect IP address/port

**Fix:**
- Verify IP address is correct
- Check network cable/WiFi
- Restart Q-SYS Core
- Reinstantiate plugin

### Commands Delayed

**Possible Causes:**
- Network congestion
- Q-SYS Core CPU load
- Too many commands sent rapidly

**Solutions:**
- Reduce command rate
- Batch updates
- Use timer-based updates (max 10/sec)

## Best Practices

### 1. Error Handling

Always wrap UDP operations in error handlers:

```lua
function SafeSend(segment, text, color)
    local success, error = pcall(function()
        Controls["text_" .. segment].String = text
        Controls["color_" .. segment].String = color
        Controls["send_" .. segment].Boolean = true
        Controls["send_" .. segment].Boolean = false
    end)
    
    if not success then
        print("Error sending to matrix: " .. error)
        Controls.connection_status.Value = 2  -- Fault
    end
end
```

### 2. Debouncing

Avoid sending too many rapid updates:

```lua
local update_timer = Timer.New()
local pending_update = false

function QueueUpdate()
    pending_update = true
end

update_timer.EventHandler = function()
    if pending_update then
        UpdateDisplay()
        pending_update = false
    end
end

update_timer:Start(0.5)  -- Update at most twice per second
```

### 3. State Management

Keep track of display state:

```lua
local display_state = {
    segment_0_text = "",
    segment_0_color = "FFFFFF",
    brightness = 128
}

function UpdateSegment(segment, text, color)
    if display_state["segment_" .. segment .. "_text"] ~= text or
       display_state["segment_" .. segment .. "_color"] ~= color then
        -- Only send if changed
        Controls["text_" .. segment].String = text
        Controls["color_" .. segment].String = color
        Controls["send_" .. segment].Boolean = true
        Controls["send_" .. segment].Boolean = false
        
        -- Update state
        display_state["segment_" .. segment .. "_text"] = text
        display_state["segment_" .. segment .. "_color"] = color
    end
end
```

### 4. Initialization

Always initialize plugin on design start:

```lua
function Initialize()
    -- Set default values
    Controls.ip_address.String = "192.168.1.100"
    Controls.udp_port.String = "21324"
    Controls.brightness.Value = 128
    
    -- Clear all segments
    Controls.clear_all.Boolean = true
    Controls.clear_all.Boolean = false
    
    print("LED Matrix plugin initialized")
end

Initialize()
```

## Performance Optimization

### Reduce Network Traffic

```lua
-- Bad: Sends 3 packets
Controls.text_0.String = "Hello"
Controls.send_0.Boolean = true
Controls.color_0.String = "FFFFFF"
Controls.send_0.Boolean = true
Controls.font_0.String = "roboto12"
Controls.send_0.Boolean = true

-- Good: Sends 1 packet
Controls.text_0.String = "Hello"
Controls.color_0.String = "FFFFFF"
Controls.font_0.String = "roboto12"
Controls.send_0.Boolean = true
Controls.send_0.Boolean = false
```

### Use Timers for Periodic Updates

```lua
-- Update every 5 seconds instead of on every change
local update_timer = Timer.New()
update_timer.EventHandler = function()
    UpdateDisplay()
end
update_timer:Start(5)
```

## Support and Resources

- **LED Matrix Documentation**: See README.md
- **UDP Protocol**: See UDP_PROTOCOL.md
- **Q-SYS Help**: Q-SYS Designer → Help → User Documentation
- **Forum**: Q-SYS Community Forums
- **GitHub Issues**: Report bugs at repository

## Appendix: Complete Example Design

```lua
-- Complete Q-SYS design example for conference room

-- Properties
room_name = Properties["Room Name"].Value

-- Initialize
function Initialize()
    Controls.ip_address.String = "192.168.1.100"
    Controls.brightness.Value = 128
    UpdateRoomStatus("Initializing...")
end

-- Update room status display
function UpdateRoomStatus(status)
    Controls.text_0.String = room_name
    Controls.color_0.String = "FFFFFF"
    Controls.font_0.String = "roboto16"
    Controls.send_0.Boolean = true
    Controls.send_0.Boolean = false
    
    Controls.text_1.String = status
    if status == "AVAILABLE" then
        Controls.color_1.String = "00FF00"
    else
        Controls.color_1.String = "FF0000"
    end
    Controls.send_1.Boolean = true
    Controls.send_1.Boolean = false
end

-- Connect to room scheduling system
Controls.room_busy.EventHandler = function(ctl)
    if ctl.Boolean then
        UpdateRoomStatus("IN USE")
    else
        UpdateRoomStatus("AVAILABLE")
    end
end

Initialize()
```

---

**Plugin Version**: 1.0.0  
**Q-SYS Compatibility**: 8.0+  
**Last Updated**: 2026-02-16

Ready to integrate! See [UDP Protocol Documentation](UDP_PROTOCOL.md) for advanced commands.
