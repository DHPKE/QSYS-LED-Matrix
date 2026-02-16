-- Q-SYS LED Matrix Controller Plugin
-- Version 2.0.0 - Complete Overhaul
-- Controls Olimex ESP32 Gateway LED Matrix via UDP
--
-- CHANGES in v2.0.0:
-- - Added Effect selector (none, scroll, blink)
-- - Added Alignment selector (L, C, R)
-- - Added Font Size selector (auto, 8-32)
-- - Added proper Font ComboBox
-- - Added UDP error handling
-- - Added command success feedback
-- - Added color input validation
-- - Added port number validation
-- - Added brightness debounce (500ms)
-- - Added segment active indicators
-- - Updated UI colors to match v2.x plugins
-- - Added cleanup function
-- - Improved layout spacing

PluginInfo = {
    Name = "Olimex~LED Matrix Text Display",
    Version = "2.0.0",
    Id = "dhpke.olimex.led.matrix.2.0.0",
    Description = "UDP control for Olimex ESP32 Gateway 64x32 LED Matrix (v1.2.0+ firmware)",
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
        
        -- Font selector
        table.insert(controls, {
            Name = string.format("font_%d", seg),
            ControlType = "Text",
            ControlUnit = "List",
            Choices = {"roboto8", "roboto12", "roboto16", "roboto24"},
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Font size selector
        table.insert(controls, {
            Name = string.format("size_%d", seg),
            ControlType = "Text",
            ControlUnit = "List",
            Choices = {"auto", "8", "12", "16", "24", "32"},
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
        
        -- Effect selector (fade/rainbow not in firmware v1.2.0 yet)
        table.insert(controls, {
            Name = string.format("effect_%d", seg),
            ControlType = "Text",
            ControlUnit = "List",
            Choices = {"none", "scroll", "blink"},
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Send button
        table.insert(controls, {
            Name = string.format("send_%d", seg),
            ControlType = "Button",
            ButtonType = "Trigger",
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
        Name = "clear_all",
        ControlType = "Button",
        ButtonType = "Trigger",
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
            Text = string.format("Segment %d", seg),
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
            Size = {500, 20}
        }
        
        layout[string.format("color_%d", seg)] = {
            PrettyName = "Color (Hex)",
            Style = "Text",
            Position = {555, segmentY + 25},
            Size = {90, 20}
        }
        
        layout[string.format("font_%d", seg)] = {
            PrettyName = "Font",
            Style = "ComboBox",
            Position = {655, segmentY + 25},
            Size = {120, 20}
        }
        
        -- Row 2: Size, Align, Effect
        layout[string.format("size_%d", seg)] = {
            PrettyName = "Size",
            Style = "ComboBox",
            Position = {20, segmentY + 55},
            Size = {80, 20}
        }
        
        layout[string.format("align_%d", seg)] = {
            PrettyName = "Align",
            Style = "ComboBox",
            Position = {110, segmentY + 55},
            Size = {70, 20}
        }
        
        layout[string.format("effect_%d", seg)] = {
            PrettyName = "Effect",
            Style = "ComboBox",
            Position = {190, segmentY + 55},
            Size = {100, 20}
        }
        
        -- Row 3: Buttons
        layout[string.format("send_%d", seg)] = {
            PrettyName = "Send",
            Style = "Button",
            ButtonStyle = "Trigger",
            Legend = "Send to Matrix",
            Color = {0, 180, 80},
            Position = {20, segmentY + 85},
            Size = {140, 35}
        }
        
        layout[string.format("clear_%d", seg)] = {
            PrettyName = "Clear",
            Style = "Button",
            ButtonStyle = "Trigger",
            Legend = "Clear Segment",
            Color = {200, 60, 60},
            Position = {170, segmentY + 85},
            Size = {140, 35}
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
        Size = {250, 50}
    }
    
    layout["clear_all"] = {
        PrettyName = "Clear All",
        Style = "Button",
        ButtonStyle = "Trigger",
        Legend = "Clear All Segments",
        Color = {200, 60, 60},
        Position = {290, yPos + 25},
        Size = {180, 50}
    }
    
    return layout, graphics
end

-- Runtime script
function GetPins(props)
    return GetControls(props)
end

if Controls then
    -- UDP socket
    local socket = nil
    local ip_addr = Properties["IP Address"].Value
    local udp_port = Properties["UDP Port"].Value
    local numSegments = Properties["Number of Segments"].Value
    
    -- Brightness debounce timer
    local brightnessTimer = nil
    local pendingBrightness = nil
    
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
        print("LED Matrix Controller v2.0.0 Initializing")
        print("===========================================")
        
        -- Set default values
        Controls.ip_address.String = ip_addr
        Controls.udp_port.String = tostring(udp_port)
        Controls.brightness.Value = 128
        Controls.last_command.String = "Ready"
        
        -- Set defaults for each segment
        for i = 0, numSegments-1 do
            Controls[string.format("color_%d", i)].String = "FFFFFF"
            Controls[string.format("font_%d", i)].String = "roboto12"
            Controls[string.format("size_%d", i)].String = "auto"
            Controls[string.format("align_%d", i)].String = "C"
            Controls[string.format("effect_%d", i)].String = "none"
            Controls[string.format("active_%d", i)].Boolean = false
        end
        
        -- Open UDP socket with error handling
        local success, err = pcall(function()
            socket = UdpSocket.New()
            socket:Open(ip_addr, udp_port)
        end)
        
        if success then
            Controls.connection_status.Value = 0 -- OK
            print("✓ UDP socket opened: " .. ip_addr .. ":" .. udp_port)
        else
            Controls.connection_status.Value = 2 -- Fault
            print("✗ ERROR opening socket: " .. tostring(err))
        end
        
        print("===========================================")
    end
    
    -- Send UDP command with error handling
    function SendCommand(command)
        if not socket then
            print("✗ ERROR: Socket not initialized")
            Controls.connection_status.Value = 2 -- Fault
            Controls.last_command.String = "ERROR: No socket"
            return false
        end
        
        local success, err = pcall(function()
            socket:Send(command .. "\n")
        end)
        
        if success then
            print("→ Sent: " .. command)
            Controls.last_command.String = command
            Controls.connection_status.Value = 0 -- OK
            return true
        else
            print("✗ Send error: " .. tostring(err))
            Controls.last_command.String = "ERROR: " .. tostring(err)
            Controls.connection_status.Value = 2 -- Fault
            return false
        end
    end
    
    -- Send button handlers
    for i = 0, numSegments-1 do
        Controls[string.format("send_%d", i)].EventHandler = function()
            local text = Controls[string.format("text_%d", i)].String
            local color = Controls[string.format("color_%d", i)].String
            local font = Controls[string.format("font_%d", i)].String
            local size = Controls[string.format("size_%d", i)].String
            local align = Controls[string.format("align_%d", i)].String
            local effect = Controls[string.format("effect_%d", i)].String
            
            -- Validate color
            color = ValidateHexColor(color)
            if not color then
                print("✗ Invalid color format for segment " .. i)
                Controls.last_command.String = "ERROR: Invalid color"
                return
            end
            
            -- Build command: TEXT|segment|content|color|font|size|align|effect
            local command = string.format("TEXT|%d|%s|%s|%s|%s|%s|%s", 
                                        i, text, color, font, size, align, effect)
            
            if SendCommand(command) then
                -- Mark segment as active
                Controls[string.format("active_%d", i)].Boolean = true
            end
        end
    end
    
    -- Clear button handlers
    for i = 0, numSegments-1 do
        Controls[string.format("clear_%d", i)].EventHandler = function()
            local command = string.format("CLEAR|%d", i)
            if SendCommand(command) then
                Controls[string.format("active_%d", i)].Boolean = false
            end
        end
    end
    
    -- Clear all handler
    Controls.clear_all.EventHandler = function()
        if SendCommand("CLEAR_ALL") then
            for i = 0, numSegments-1 do
                Controls[string.format("active_%d", i)].Boolean = false
            end
        end
    end
    
    -- Brightness handler with debounce
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
                local command = string.format("BRIGHTNESS|%d", pendingBrightness)
                SendCommand(command)
                pendingBrightness = nil
            end
        end
        brightnessTimer:Start(0.5)
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
                socket:Open(ip_addr, udp_port)
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
                    socket:Open(ip_addr, udp_port)
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
    end
    
    Initialize()
end
