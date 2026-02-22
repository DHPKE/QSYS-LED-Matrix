-- Q-SYS LED Matrix Controller Plugin
-- Version 3.1.0 - Portrait Mode Support
-- Controls RPi Zero 2 W LED Matrix via UDP (JSON format)
--
-- CHANGES in v3.1.0:
-- - Added portrait mode support with persistent orientation setting
-- - Orientation control in plugin UI (landscape/portrait)
-- - Configuration persistence for orientation
--
-- CHANGES in v3.0.0:
-- - Updated to JSON protocol (matches web UI and UDP handler)
-- - Changed font list to Arial (installed on system)
-- - Added background color control
-- - Added intensity control per segment
-- - Added auto-send mode (updates on text change)
-- - Removed "Send" buttons (auto-send enabled)
-- - Optimized for maximum font sizes
-- - Updated for RPi Zero 2 W deployment
-- - Matches web UI functionality

PluginInfo = {
    Name = "Olimex~LED Matrix Text Display",
    Version = "3.1.0",
    Id = "dhpke.olimex.led.matrix.3.1.0",
    Description = "UDP control for RPi Zero 2 W 64x32 LED Matrix (JSON protocol, portrait mode support)",
    ShowDebug = true,
    Author = "DHPKE"
}

-- Define plugin properties
function GetProperties()
    return {
        {
            Name = "IP Address",
            Type = "string",
            Value = "192.168.1.100"
        },
        {
            Name = "UDP Port",
            Type = "integer",
            Min = 1,
            Max = 65535,
            Value = 21324
        },
        {
            Name = "Number of Segments",
            Type = "integer",
            Min = 1,
            Max = 4,
            Value = 4
        }
    }
end

-- Define the layout
function GetControls(props)
    local controls = {}
    local numSegments = props["Number of Segments"].Value
    
    -- Connection controls
    table.insert(controls, {
        Name = "ip_address",
        ControlType = "Text",
        Count = 1,
        UserPin = true,
        PinStyle = "Input"
    })
    
    table.insert(controls, {
        Name = "udp_port",
        ControlType = "Text",
        Count = 1,
        UserPin = true,
        PinStyle = "Input"
    })
    
    table.insert(controls, {
        Name = "connection_status",
        ControlType = "Indicator",
        IndicatorType = "Status",
        Count = 1,
        UserPin = true,
        PinStyle = "Output"
    })
    
    table.insert(controls, {
        Name = "last_command",
        ControlType = "Text",
        Count = 1,
        UserPin = true,
        PinStyle = "Output"
    })
    
    -- Segment controls
    for i = 1, numSegments do
        local seg = i - 1
        
        -- Active indicator
        table.insert(controls, {
            Name = string.format("active_%d", seg),
            ControlType = "Indicator",
            IndicatorType = "LED",
            Count = 1,
            UserPin = true,
            PinStyle = "Output"
        })
        
        -- Text input
        table.insert(controls, {
            Name = string.format("text_%d", seg),
            ControlType = "Text",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Color input (hex)
        table.insert(controls, {
            Name = string.format("color_%d", seg),
            ControlType = "Text",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Background color input (hex)
        table.insert(controls, {
            Name = string.format("bgcolor_%d", seg),
            ControlType = "Text",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Intensity slider
        table.insert(controls, {
            Name = string.format("intensity_%d", seg),
            ControlType = "Knob",
            ControlUnit = "Integer",
            Min = 0,
            Max = 255,
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Font selector (Arial is default on system)
        table.insert(controls, {
            Name = string.format("font_%d", seg),
            ControlType = "Text",
            ControlUnit = "List",
            Choices = {"arial", "mono"},
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Font size selector (auto for maximum size)
        table.insert(controls, {
            Name = string.format("size_%d", seg),
            ControlType = "Text",
            ControlUnit = "List",
            Choices = {"auto", "6", "8", "10", "12", "14", "16", "18", "20", "24", "28", "32"},
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Alignment selector
        table.insert(controls, {
            Name = string.format("align_%d", seg),
            ControlType = "Text",
            ControlUnit = "List",
            Choices = {"L", "C", "R"},
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
         -- Effect selector
        table.insert(controls, {
            Name = string.format("effect_%d", seg),
            ControlType = "Text",
            ControlUnit = "List",
            Choices = {"none", "scroll", "blink"},
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Clear button
        table.insert(controls, {
            Name = string.format("clear_%d", seg),
            ControlType = "Button",
            ButtonType = "Trigger",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Invert button (swaps color and bgcolor)
        table.insert(controls, {
            Name = string.format("invert_%d", seg),
            ControlType = "Button",
            ButtonType = "Trigger",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
    end
    
    -- Global controls
    table.insert(controls, {
        Name = "brightness",
        ControlType = "Knob",
        ControlUnit = "Integer",
        Min = 0,
        Max = 255,
        Count = 1,
        UserPin = true,
        PinStyle = "Both"
    })
    
    table.insert(controls, {
        Name = "orientation",
        ControlType = "Text",
        ControlUnit = "List",
        Choices = {"landscape", "portrait"},
        Count = 1,
        UserPin = true,
        PinStyle = "Both"
    })
    
    table.insert(controls, {
        Name = "clear_all",
        ControlType = "Button",
        ButtonType = "Trigger",
        Count = 1,
        UserPin = true,
        PinStyle = "Input"
    })
    
    -- Layout selector (determines which segments are visible/active)
    table.insert(controls, {
        Name = "layout",
        ControlType = "Text",
        ControlUnit = "List",
        Choices = {"fullscreen", "split-vertical", "split-horizontal", "quad"},
        Count = 1,
        UserPin = true,
        PinStyle = "Input"
    })
    
    return controls
end

-- Define the UI layout
function GetControlLayout(props)
    local layout = {}
    local graphics = {}
    local numSegments = props["Number of Segments"].Value
    
    -- Title (brighter grey background like v2.x plugins)
    table.insert(graphics, {
        Type = "GroupBox",
        Fill = {160, 160, 160},  -- Bright grey for text contrast
        StrokeWidth = 2,
        CornerRadius = 8,
        Position = {0, 0},
        Size = {800, 50}
    })
    
    table.insert(graphics, {
        Type = "Text",
        Text = "Olimex LED Matrix Controller v2.0",
        Font = "Roboto",
        FontSize = 18,
        FontStyle = "Bold",
        HTextAlign = "Center",
        Color = {0, 0, 0},  -- Black text on bright grey
        Position = {0, 15},
        Size = {800, 30}
    })
    
    -- Connection section
    local yPos = 60
    table.insert(graphics, {
        Type = "GroupBox",
        Text = "Connection",
        Fill = {140, 140, 140},  -- Bright grey
        StrokeWidth = 1,
        CornerRadius = 4,
        Position = {10, yPos},
        Size = {780, 90}
    })
    
    layout["ip_address"] = {
        PrettyName = "IP Address",
        Style = "Text",
        Position = {20, yPos + 25},
        Size = {180, 20}
    }
    
    layout["udp_port"] = {
        PrettyName = "UDP Port",
        Style = "Text",
        Position = {210, yPos + 25},
        Size = {80, 20}
    }
    
    layout["connection_status"] = {
        PrettyName = "Status",
        Style = "Indicator",
        Position = {300, yPos + 25},
        Size = {40, 20}
    }
    
    layout["last_command"] = {
        PrettyName = "Last Command",
        Style = "Text",
        Position = {20, yPos + 55},
        Size = {750, 20},
        Color = {100, 100, 100}
    }
    
    -- Segment controls
    yPos = 160
    for i = 1, numSegments do
        local seg = i - 1
        local segmentY = yPos + (seg) * 140
        
        table.insert(graphics, {
            Type = "GroupBox",
            Text = string.format("Segment %d (Auto-Update)", seg),
            Fill = {140, 140, 140},  -- Bright grey
            StrokeWidth = 1,
            CornerRadius = 4,
            Position = {10, segmentY},
            Size = {780, 130}
        })
        
        -- Row 1: Active indicator + Text
        layout[string.format("active_%d", seg)] = {
            PrettyName = "Active",
            Style = "Led",
            Position = {20, segmentY + 25},
            Size = {16, 16}
        }
        
        layout[string.format("text_%d", seg)] = {
            PrettyName = "Text Content",
            Style = "Text",
            Position = {45, segmentY + 25},
            Size = {380, 20}
        }
        
        layout[string.format("color_%d", seg)] = {
            PrettyName = "Text Color",
            Style = "Text",
            Position = {435, segmentY + 25},
            Size = {85, 20}
        }
        
        layout[string.format("bgcolor_%d", seg)] = {
            PrettyName = "BG Color",
            Style = "Text",
            Position = {530, segmentY + 25},
            Size = {85, 20}
        }
        
        layout[string.format("intensity_%d", seg)] = {
            PrettyName = "Intensity",
            Style = "Fader",
            Position = {625, segmentY + 25},
            Size = {60, 20}
        }
        
        layout[string.format("font_%d", seg)] = {
            PrettyName = "Font",
            Style = "ComboBox",
            Position = {695, segmentY + 25},
            Size = {80, 20}
        }
        
        -- Row 2: Size, Align, Effect, Clear
        layout[string.format("size_%d", seg)] = {
            PrettyName = "Size",
            Style = "ComboBox",
            Position = {20, segmentY + 55},
            Size = {90, 20}
        }
        
        layout[string.format("align_%d", seg)] = {
            PrettyName = "Align",
            Style = "ComboBox",
            Position = {120, segmentY + 55},
            Size = {70, 20}
        }
        
        layout[string.format("effect_%d", seg)] = {
            PrettyName = "Effect",
            Style = "ComboBox",
            Position = {200, segmentY + 55},
            Size = {100, 20}
        }
        
        -- Row 3: Clear and Invert buttons
        layout[string.format("clear_%d", seg)] = {
            PrettyName = "Clear",
            Style = "Button",
            ButtonStyle = "Trigger",
            Legend = "Clear Segment",
            Color = {200, 60, 60},
            Position = {20, segmentY + 85},
            Size = {140, 35}
        }
        
        layout[string.format("invert_%d", seg)] = {
            PrettyName = "Invert",
            Style = "Button",
            ButtonStyle = "Trigger",
            Legend = "Invert Colors",
            Color = {100, 100, 220},
            Position = {170, segmentY + 85},
            Size = {140, 35}
        }
        }
        
    end
    
    -- Global controls
    yPos = 160 + (numSegments * 140) + 10
    table.insert(graphics, {
        Type = "GroupBox",
        Text = "Global Settings",
        Fill = {140, 140, 140},  -- Bright grey
        StrokeWidth = 1,
        CornerRadius = 4,
        Position = {10, yPos},
        Size = {780, 90}
    })
    
    layout["brightness"] = {
        PrettyName = "Brightness (0-255)",
        Style = "Fader",
        Position = {20, yPos + 25},
        Size = {200, 50}
    }
    
    layout["orientation"] = {
        PrettyName = "Orientation",
        Style = "ComboBox",
        Position = {240, yPos + 25},
        Size = {150, 50}
    }
    
    layout["clear_all"] = {
        PrettyName = "Clear All",
        Style = "Button",
        ButtonStyle = "Trigger",
        Legend = "Clear All Segments",
        Color = {200, 60, 60},
        Position = {410, yPos + 25},
        Size = {150, 50}
    }
    
    layout["layout"] = {
        PrettyName = "Display Layout",
        Style = "ComboBox",
        Position = {580, yPos + 25},
        Size = {180, 50}
    }
    
    return layout, graphics
end

-- Runtime code starts here
if Controls then
    -- UDP socket
    local socket = nil
    local ip_addr = Properties["IP Address"].Value
    local udp_port = Properties["UDP Port"].Value
    local numSegments = Properties["Number of Segments"].Value
    
    -- Brightness debounce timer
    local brightnessTimer = nil
    local pendingBrightness = nil
    
    -- Current layout state - tracks which segments are visible
    local currentLayout = "fullscreen"  -- default
    local activeSegments = {0}  -- default: only segment 0 visible
    
    -- Validate hex color
    function ValidateHexColor(color)
        -- Remove # if present
        color = color:gsub("#", "")
        -- Check if valid hex (3 or 6 chars, only 0-9A-F)
        if color:match("^[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$") or 
           color:match("^[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$") then
            return color:upper()
        end
        return nil
    end
    
    -- Validate port number
    function ValidatePort(port)
        local num = tonumber(port)
        if num and num >= 1 and num <= 65535 then
            return num
        end
        return nil
    end
    
    -- Initialize
    function Initialize()
        print("===========================================")
        print("LED Matrix Controller v3.0.0 Initializing")
        print("===========================================")
        
        -- Set default values
        Controls.ip_address.String = ip_addr
        Controls.udp_port.String = tostring(udp_port)
        Controls.brightness.Value = 128
        Controls.orientation.String = "landscape"
        Controls.last_command.String = "Ready"
        
        -- Set defaults for each segment
        for i = 0, numSegments-1 do
            Controls[string.format("color_%d", i)].String = "FFFFFF"
            Controls[string.format("bgcolor_%d", i)].String = "000000"
            Controls[string.format("intensity_%d", i)].Value = 255
            Controls[string.format("font_%d", i)].String = "arial"
            Controls[string.format("size_%d", i)].String = "auto"
            Controls[string.format("align_%d", i)].String = "C"
            Controls[string.format("effect_%d", i)].String = "none"
            Controls[string.format("active_%d", i)].Boolean = false
        end
        
        -- Set default layout
        Controls.layout.String = "fullscreen"
        currentLayout = "fullscreen"
        activeSegments = {0}
        
        -- Open UDP socket with error handling (connectionless — no IP/port at open time)
        local success, err = pcall(function()
            socket = UdpSocket.New()
            socket:Open()
        end)
        
        if success then
            Controls.connection_status.Value = 0 -- OK
            print("✓ UDP socket opened, targeting " .. ip_addr .. ":" .. udp_port)
        else
            Controls.connection_status.Value = 2 -- Fault
            print("✗ ERROR opening socket: " .. tostring(err))
        end
        
        print("===========================================")
    end
    
    -- Send UDP command with error handling
    function SendCommand(jsonString)
        if not socket then
            print("✗ ERROR: Socket not initialized")
            Controls.connection_status.Value = 2 -- Fault
            Controls.last_command.String = "ERROR: No socket"
            return false
        end
        
        local success, err = pcall(function()
            socket:Send(ip_addr, udp_port, jsonString .. "\n")
        end)
        
        if success then
            print("→ Sent: " .. jsonString)
            Controls.last_command.String = jsonString
            Controls.connection_status.Value = 0 -- OK
            return true
        else
            print("✗ Send error: " .. tostring(err))
            Controls.last_command.String = "ERROR: " .. tostring(err)
            Controls.connection_status.Value = 2 -- Fault
            return false
        end
    end
    
    -- Debounce timers for auto-send
    local segmentTimers = {}
    
    -- Build and send text command (JSON format)
    function SendTextCommand(segmentIndex)
        local text = Controls[string.format("text_%d", segmentIndex)].String
        local color = ValidateHexColor(Controls[string.format("color_%d", segmentIndex)].String)
        local bgcolor = ValidateHexColor(Controls[string.format("bgcolor_%d", segmentIndex)].String)
        local intensity = math.floor(Controls[string.format("intensity_%d", segmentIndex)].Value)
        local font = Controls[string.format("font_%d", segmentIndex)].String
        local size = Controls[string.format("size_%d", segmentIndex)].String
        local align = Controls[string.format("align_%d", segmentIndex)].String
        local effect = Controls[string.format("effect_%d", segmentIndex)].String
        
        if not color then
            color = "FFFFFF"
            Controls[string.format("color_%d", segmentIndex)].String = color
        end
        
        if not bgcolor then
            bgcolor = "000000"
            Controls[string.format("bgcolor_%d", segmentIndex)].String = bgcolor
        end
        
        -- Build JSON command
        local jsonCmd = string.format(
            '{"cmd":"text","seg":%d,"text":"%s","color":"%s","bgcolor":"%s","font":"%s","size":"%s","align":"%s","effect":"%s","intensity":%d}',
            segmentIndex, text:gsub('"', '\\"'), color, bgcolor, font, size, align, effect, intensity
        )
        
        if SendCommand(jsonCmd) then
            Controls[string.format("active_%d", segmentIndex)].Boolean = (text ~= "")
        end
    end
    
    -- Check if segment is visible in current layout
    function IsSegmentVisible(segmentIndex)
        for _, seg in ipairs(activeSegments) do
            if seg == segmentIndex then
                return true
            end
        end
        return false
    end
    
    -- Debounced send for segment (500ms delay, only if visible)
    function DebouncedSend(segmentIndex)
        -- Cancel existing timer
        if segmentTimers[segmentIndex] then
            segmentTimers[segmentIndex]:Cancel()
        end
        
        -- Create new timer
        segmentTimers[segmentIndex] = Timer.New()
        segmentTimers[segmentIndex].EventHandler = function()
            -- Only send if segment is visible in current layout
            if IsSegmentVisible(segmentIndex) then
                SendTextCommand(segmentIndex)
            else
                print(string.format("Segment %d edited offline (not visible in %s layout)", segmentIndex, currentLayout))
            end
        end
        segmentTimers[segmentIndex]:Start(0.5)
    end
    
    -- Apply layout configuration
    function ApplyLayout(layoutType)
        currentLayout = layoutType
        local cmds = {}
        
        if layoutType == "fullscreen" then
            activeSegments = {0}
            table.insert(cmds, '{"cmd":"config","seg":0,"x":0,"y":0,"w":64,"h":32}')
            -- Clear inactive segments
            for i = 1, 3 do
                table.insert(cmds, string.format('{"cmd":"clear","seg":%d}', i))
            end
        elseif layoutType == "split-vertical" then
            activeSegments = {0, 1}
            table.insert(cmds, '{"cmd":"config","seg":0,"x":0,"y":0,"w":32,"h":32}')
            table.insert(cmds, '{"cmd":"config","seg":1,"x":32,"y":0,"w":32,"h":32}')
            -- Clear inactive segments
            for i = 2, 3 do
                table.insert(cmds, string.format('{"cmd":"clear","seg":%d}', i))
            end
        elseif layoutType == "split-horizontal" then
            activeSegments = {0, 1}
            table.insert(cmds, '{"cmd":"config","seg":0,"x":0,"y":0,"w":64,"h":16}')
            table.insert(cmds, '{"cmd":"config","seg":1,"x":0,"y":16,"w":64,"h":16}')
            -- Clear inactive segments
            for i = 2, 3 do
                table.insert(cmds, string.format('{"cmd":"clear","seg":%d}', i))
            end
        elseif layoutType == "quad" then
            activeSegments = {0, 1, 2, 3}
            table.insert(cmds, '{"cmd":"config","seg":0,"x":0,"y":0,"w":32,"h":16}')
            table.insert(cmds, '{"cmd":"config","seg":1,"x":32,"y":0,"w":32,"h":16}')
            table.insert(cmds, '{"cmd":"config","seg":2,"x":0,"y":16,"w":32,"h":16}')
            table.insert(cmds, '{"cmd":"config","seg":3,"x":32,"y":16,"w":32,"h":16}')
        end
        
        -- Send all config commands
        for _, cmd in ipairs(cmds) do
            SendCommand(cmd)
        end
        
        print(string.format("✓ Layout changed to %s (active segments: %s)", layoutType, table.concat(activeSegments, ", ")))
        
        -- After a brief delay, render all active segments with current values
        Timer.CallAfter(function()
            for _, seg in ipairs(activeSegments) do
                SendTextCommand(seg)
            end
        end, 0.3)
    end
    
    -- Auto-send event handlers for all text-related controls
    for i = 0, numSegments-1 do
        -- Text changed - auto-send with debounce
        Controls[string.format("text_%d", i)].EventHandler = function()
            DebouncedSend(i)
        end
        
        -- Color changed - auto-send with debounce
        Controls[string.format("color_%d", i)].EventHandler = function()
            DebouncedSend(i)
        end
        
        -- Background color changed - auto-send with debounce
        Controls[string.format("bgcolor_%d", i)].EventHandler = function()
            DebouncedSend(i)
        end
        
        -- Intensity changed - auto-send with debounce
        Controls[string.format("intensity_%d", i)].EventHandler = function()
            DebouncedSend(i)
        end
        
        -- Font/size/align/effect changed - auto-send with debounce
        Controls[string.format("font_%d", i)].EventHandler = function()
            DebouncedSend(i)
        end
        
        Controls[string.format("size_%d", i)].EventHandler = function()
            DebouncedSend(i)
        end
        
        Controls[string.format("align_%d", i)].EventHandler = function()
            DebouncedSend(i)
        end
        
        Controls[string.format("effect_%d", i)].EventHandler = function()
            DebouncedSend(i)
        end
    end
    
    -- Clear button handlers (JSON format)
    for i = 0, numSegments-1 do
        Controls[string.format("clear_%d", i)].EventHandler = function()
            local jsonCmd = string.format('{"cmd":"clear","seg":%d}', i)
            if SendCommand(jsonCmd) then
                Controls[string.format("active_%d", i)].Boolean = false
                Controls[string.format("text_%d", i)].String = ""
            end
        end
    end
    
    -- Invert button handlers (swap color and bgcolor)
    for i = 0, numSegments-1 do
        Controls[string.format("invert_%d", i)].EventHandler = function()
            local colorCtrl = Controls[string.format("color_%d", i)]
            local bgcolorCtrl = Controls[string.format("bgcolor_%d", i)]
            
            -- Swap the values
            local tempColor = colorCtrl.String
            colorCtrl.String = bgcolorCtrl.String
            bgcolorCtrl.String = tempColor
            
            print(string.format("⇄ Segment %d colors inverted", i))
            
            -- Trigger auto-send (if segment is visible)
            DebouncedSend(i)
        end
    end
    
    -- Clear all handler (JSON format)
    Controls.clear_all.EventHandler = function()
        local jsonCmd = '{"cmd":"clear_all"}'
        if SendCommand(jsonCmd) then
            for i = 0, numSegments-1 do
                Controls[string.format("active_%d", i)].Boolean = false
                Controls[string.format("text_%d", i)].String = ""
            end
        end
    end
    
    -- Brightness handler with debounce (JSON format)
    Controls.brightness.EventHandler = function(ctl)
        pendingBrightness = math.floor(ctl.Value)
        
        -- Cancel existing timer
        if brightnessTimer then
            brightnessTimer:Cancel()
        end
        
        -- Create new timer (500ms debounce)
        brightnessTimer = Timer.New()
        brightnessTimer.EventHandler = function()
            if pendingBrightness then
                local jsonCmd = string.format('{"cmd":"brightness","value":%d}', pendingBrightness)
                SendCommand(jsonCmd)
                pendingBrightness = nil
            end
        end
        brightnessTimer:Start(0.5)
    end
    
    -- Layout change handler
    Controls.layout.EventHandler = function(ctl)
        ApplyLayout(ctl.String)
    end
    
    -- Orientation change handler
    Controls.orientation.EventHandler = function(ctl)
        local orient = ctl.String
        local jsonCmd = string.format('{"cmd":"orientation","value":"%s"}', orient)
        if SendCommand(jsonCmd) then
            print("Orientation set to: " .. orient)
        end
    end
    
    -- IP/Port change handlers
    Controls.ip_address.EventHandler = function(ctl)
        ip_addr = ctl.String
        print("IP address changed to: " .. ip_addr)
        
        -- Reconnect socket
        if socket then
            pcall(function() socket:Close() end)
            local success, err = pcall(function()
                socket = UdpSocket.New()
                socket:Open()
            end)
            
            if success then
                Controls.connection_status.Value = 0
                print("✓ Reconnected to " .. ip_addr .. ":" .. udp_port)
            else
                Controls.connection_status.Value = 2
                print("✗ Reconnection failed: " .. tostring(err))
            end
        end
    end
    
    Controls.udp_port.EventHandler = function(ctl)
        local validated = ValidatePort(ctl.String)
        if validated then
            udp_port = validated
            print("UDP port changed to: " .. udp_port)
            
            -- Reconnect socket
            if socket then
                pcall(function() socket:Close() end)
                local success, err = pcall(function()
                    socket = UdpSocket.New()
                    socket:Open()
                end)
                
                if success then
                    Controls.connection_status.Value = 0
                    print("✓ Reconnected to " .. ip_addr .. ":" .. udp_port)
                else
                    Controls.connection_status.Value = 2
                    print("✗ Reconnection failed: " .. tostring(err))
                end
            end
        else
            print("✗ Invalid port number: " .. ctl.String)
            Controls.last_command.String = "ERROR: Invalid port"
        end
    end
    
    -- Cleanup function
    function Cleanup()
        print("Cleaning up LED Matrix Controller...")
        if socket then
            pcall(function() socket:Close() end)
        end
        if brightnessTimer then
            brightnessTimer:Cancel()
        end
        -- Cancel all segment timers
        for i = 0, numSegments-1 do
            if segmentTimers[i] then
                segmentTimers[i]:Cancel()
            end
        end
    end
    
    Initialize()
end
