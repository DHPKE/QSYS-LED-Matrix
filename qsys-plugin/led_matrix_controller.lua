-- Q-SYS LED Matrix Controller Plugin
-- Version 1.0.0
-- Controls Olimex ESP32 Gateway LED Matrix via UDP

PluginInfo = {
    Name = "Olimex~LED Matrix Text Display",
    Version = "1.0.0",
    Id = "dhpke.olimex.led.matrix.1.0.0",
    Description = "UDP control for Olimex ESP32 Gateway 64x32 LED Matrix",
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
    
    -- Segment controls
    for i = 1, numSegments do
        -- Text input
        table.insert(controls, {
            Name = string.format("text_%d", i-1),
            ControlType = "Text",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Color input (hex)
        table.insert(controls, {
            Name = string.format("color_%d", i-1),
            ControlType = "Text",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Font selector
        table.insert(controls, {
            Name = string.format("font_%d", i-1),
            ControlType = "Text",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Send button
        table.insert(controls, {
            Name = string.format("send_%d", i-1),
            ControlType = "Button",
            ButtonType = "Trigger",
            Count = 1,
            UserPin = true,
            PinStyle = "Input"
        })
        
        -- Clear button
        table.insert(controls, {
            Name = string.format("clear_%d", i-1),
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
    
    -- Title
    table.insert(graphics, {
        Type = "GroupBox",
        Fill = {74, 74, 74},
        StrokeWidth = 2,
        CornerRadius = 8,
        Position = {0, 0},
        Size = {600, 50}
    })
    
    table.insert(graphics, {
        Type = "Text",
        Text = "Olimex LED Matrix Controller",
        Font = "Roboto",
        FontSize = 18,
        FontStyle = "Bold",
        HTextAlign = "Center",
        Color = {0, 255, 136},
        Position = {0, 15},
        Size = {600, 30}
    })
    
    -- Connection section
    local yPos = 60
    table.insert(graphics, {
        Type = "GroupBox",
        Text = "Connection",
        Fill = {30, 30, 30},
        StrokeWidth = 1,
        CornerRadius = 4,
        Position = {10, yPos},
        Size = {580, 80}
    })
    
    layout["ip_address"] = {
        PrettyName = "IP Address",
        Style = "Text",
        Position = {20, yPos + 25},
        Size = {200, 20}
    }
    
    layout["udp_port"] = {
        PrettyName = "UDP Port",
        Style = "Text",
        Position = {230, yPos + 25},
        Size = {100, 20}
    }
    
    layout["connection_status"] = {
        PrettyName = "Status",
        Style = "Indicator",
        Position = {340, yPos + 25},
        Size = {40, 20}
    }
    
    -- Segment controls
    yPos = 150
    for i = 1, numSegments do
        local segmentY = yPos + (i-1) * 120
        
        table.insert(graphics, {
            Type = "GroupBox",
            Text = string.format("Segment %d", i-1),
            Fill = {50, 50, 50},
            StrokeWidth = 1,
            CornerRadius = 4,
            Position = {10, segmentY},
            Size = {580, 110}
        })
        
        layout[string.format("text_%d", i-1)] = {
            PrettyName = "Text",
            Style = "Text",
            Position = {20, segmentY + 25},
            Size = {400, 20}
        }
        
        layout[string.format("color_%d", i-1)] = {
            PrettyName = "Color (Hex)",
            Style = "Text",
            Position = {430, segmentY + 25},
            Size = {80, 20}
        }
        
        layout[string.format("font_%d", i-1)] = {
            PrettyName = "Font",
            Style = "ComboBox",
            Position = {520, segmentY + 25},
            Size = {60, 20}
        }
        
        layout[string.format("send_%d", i-1)] = {
            PrettyName = "Send",
            Style = "Button",
            ButtonStyle = "Trigger",
            Legend = "Send",
            Color = {0, 200, 100},
            Position = {20, segmentY + 55},
            Size = {100, 30}
        }
        
        layout[string.format("clear_%d", i-1)] = {
            PrettyName = "Clear",
            Style = "Button",
            ButtonStyle = "Trigger",
            Legend = "Clear",
            Color = {200, 0, 0},
            Position = {130, segmentY + 55},
            Size = {100, 30}
        }
    end
    
    -- Global controls
    yPos = 150 + (numSegments * 120) + 10
    table.insert(graphics, {
        Type = "GroupBox",
        Text = "Global Settings",
        Fill = {50, 50, 50},
        StrokeWidth = 1,
        CornerRadius = 4,
        Position = {10, yPos},
        Size = {580, 80}
    })
    
    layout["brightness"] = {
        PrettyName = "Brightness",
        Style = "Fader",
        Position = {20, yPos + 25},
        Size = {200, 40}
    }
    
    layout["clear_all"] = {
        PrettyName = "Clear All",
        Style = "Button",
        ButtonStyle = "Trigger",
        Legend = "Clear All Segments",
        Color = {200, 0, 0},
        Position = {230, yPos + 25},
        Size = {150, 40}
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
    
    -- Initialize
    function Initialize()
        -- Set default values
        Controls.ip_address.String = ip_addr
        Controls.udp_port.String = tostring(udp_port)
        Controls.brightness.Value = 128
        
        -- Set default colors
        for i = 0, numSegments-1 do
            Controls[string.format("color_%d", i)].String = "FFFFFF"
            Controls[string.format("font_%d", i)].String = "roboto12"
        end
        
        -- Open UDP socket
        socket = UdpSocket.New()
        socket:Open(ip_addr, udp_port)
        
        Controls.connection_status.Value = 0 -- OK
        print("LED Matrix Controller initialized")
        print("Target: " .. ip_addr .. ":" .. udp_port)
    end
    
    -- Send UDP command
    function SendCommand(command)
        if socket then
            socket:Send(command .. "\n")
            print("Sent: " .. command)
        else
            print("Error: Socket not initialized")
            Controls.connection_status.Value = 2 -- Fault
        end
    end
    
    -- Send button handlers
    for i = 0, numSegments-1 do
        Controls[string.format("send_%d", i)].EventHandler = function()
            local text = Controls[string.format("text_%d", i)].String
            local color = Controls[string.format("color_%d", i)].String
            local font = Controls[string.format("font_%d", i)].String
            
            -- Remove # if present
            color = color:gsub("#", "")
            
            -- Build command: TEXT|segment|content|color|font|size|align|effect
            local command = string.format("TEXT|%d|%s|%s|%s|auto|C|none", 
                                        i, text, color, font)
            SendCommand(command)
        end
    end
    
    -- Clear button handlers
    for i = 0, numSegments-1 do
        Controls[string.format("clear_%d", i)].EventHandler = function()
            local command = string.format("CLEAR|%d", i)
            SendCommand(command)
        end
    end
    
    -- Clear all handler
    Controls.clear_all.EventHandler = function()
        SendCommand("CLEAR_ALL")
    end
    
    -- Brightness handler
    Controls.brightness.EventHandler = function(ctl)
        local value = math.floor(ctl.Value)
        local command = string.format("BRIGHTNESS|%d", value)
        SendCommand(command)
    end
    
    -- IP/Port change handlers
    Controls.ip_address.EventHandler = function(ctl)
        ip_addr = ctl.String
        if socket then
            socket:Close()
            socket:Open(ip_addr, udp_port)
        end
    end
    
    Controls.udp_port.EventHandler = function(ctl)
        udp_port = tonumber(ctl.String) or 21324
        if socket then
            socket:Close()
            socket:Open(ip_addr, udp_port)
        end
    end
    
    Initialize()
end
