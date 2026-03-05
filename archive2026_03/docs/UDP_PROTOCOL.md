# UDP Protocol Specification

## Overview

The LED Matrix Text Display uses a simple ASCII-based UDP protocol for receiving text commands. This document provides the complete specification for all supported commands.

## Connection Details

- **Protocol**: UDP (User Datagram Protocol)
- **Default Port**: 21324
- **Encoding**: ASCII text
- **Terminator**: Newline character (`\n`)
- **Maximum packet size**: 512 bytes
- **No authentication**: Open protocol (use on trusted networks only)

## General Format

All commands follow this structure:

```
COMMAND|parameter1|parameter2|...|parameterN\n
```

- Commands are case-sensitive
- Parameters are separated by pipe character (`|`)
- Each command must end with newline (`\n`)
- Invalid commands are silently ignored

## Supported Commands

### 1. TEXT - Display Text

Displays text on a specified segment with formatting options.

**Syntax:**
```
TEXT|segment|content|color|font|size|align|effect
```

**Parameters:**

| Parameter | Type | Description | Valid Values | Required |
|-----------|------|-------------|--------------|----------|
| segment | int | Segment ID | 0-3 | Yes |
| content | string | Text to display | Any ASCII text (URL encode special chars) | Yes |
| color | hex | Text color | 6-digit hex RRGGBB (no #) | Yes |
| font | string | Font name | roboto6, roboto8, roboto12, roboto16, roboto24, digital12, digital24, mono9, mono12 | Yes |
| size | mixed | Font size | 6-32 (fixed) or "auto" | Yes |
| align | char | Text alignment | L (left), C (center), R (right) | Yes |
| effect | string | Text effect | none, scroll, blink, fade, rainbow | Yes |

**Examples:**

```
TEXT|0|Hello World|FFFFFF|roboto12|auto|C|none
TEXT|1|Temperature: 72.5|00FF00|digital12|16|L|none
TEXT|2|ALERT|FF0000|roboto24|auto|C|blink
TEXT|3|Scrolling Text|00FFFF|roboto16|auto|L|scroll
```

**Color Examples:**
- `FF0000` - Red
- `00FF00` - Green
- `0000FF` - Blue
- `FFFF00` - Yellow
- `FF00FF` - Magenta
- `00FFFF` - Cyan
- `FFFFFF` - White
- `FFA500` - Orange
- `800080` - Purple

**Font Size Guidelines:**
- `auto` - Automatically scales to fit segment (recommended)
- `6-9` - Very small text (fits 10-12 chars on full width)
- `12` - Small text (fits 6-8 chars)
- `16-18` - Medium text (fits 4-5 chars)
- `24` - Large text (fits 2-3 chars)
- `32` - Very large text (1-2 chars)

**Effects:**
- `none` - Static text, no effects
- `scroll` - Horizontal scrolling (right to left)
- `blink` - Blinking text (500ms intervals)
- `fade` - Fade in/out effect (not yet implemented)
- `rainbow` - Rainbow color cycling (not yet implemented)

### 2. CLEAR - Clear Segment

Clears a specific segment, removing all text.

**Syntax:**
```
CLEAR|segment
```

**Parameters:**

| Parameter | Type | Description | Valid Values | Required |
|-----------|------|-------------|--------------|----------|
| segment | int | Segment ID to clear | 0-3 | Yes |

**Examples:**

```
CLEAR|0
CLEAR|1
CLEAR|2
```

**Behavior:**
- Removes text from specified segment
- Sets segment to inactive state
- Fills segment area with black (background color)
- Other segments remain unchanged

### 3. CLEAR_ALL - Clear All Segments

Clears all segments simultaneously.

**Syntax:**
```
CLEAR_ALL
```

**Parameters:** None

**Examples:**

```
CLEAR_ALL
```

**Behavior:**
- Clears all 4 segments
- Entire display becomes black
- All segments set to inactive

### 4. BRIGHTNESS - Set Display Brightness

Adjusts the overall brightness of the LED matrix.

**Syntax:**
```
BRIGHTNESS|value
```

**Parameters:**

| Parameter | Type | Description | Valid Values | Required |
|-----------|------|-------------|--------------|----------|
| value | int | Brightness level | 0-255 (0=off, 255=max) | Yes |

**Examples:**

```
BRIGHTNESS|128
BRIGHTNESS|64
BRIGHTNESS|255
```

**Brightness Guidelines:**
- `0-50` - Very dim (suitable for dark rooms, night mode)
- `51-128` - Medium brightness (default, good for most conditions)
- `129-200` - Bright (suitable for well-lit rooms)
- `201-255` - Maximum brightness (outdoor use, high power consumption)

**Note:** Lower brightness extends LED lifetime and reduces power consumption.

### 5. CONFIG - Configuration Commands

General configuration command for various settings.

**Syntax:**
```
CONFIG|setting|value
```

**Parameters:**

| Parameter | Type | Description | Valid Values | Required |
|-----------|------|-------------|--------------|----------|
| setting | string | Setting name | brightness, udp_port | Yes |
| value | mixed | Setting value | Depends on setting | Yes |

**Supported Settings:**

**brightness:**
```
CONFIG|brightness|128
```
Same as BRIGHTNESS command.

**Examples:**

```
CONFIG|brightness|100
```

## Segment Layout

The default layout includes 4 segments:

### Segment 0 - Full Screen
- Position: (0, 0)
- Size: 64×32 pixels
- Use: Full screen messages, large text

### Segment 1 - Top Half
- Position: (0, 0)
- Size: 64×16 pixels
- Use: Header, title text

### Segment 2 - Bottom Half
- Position: (0, 16)
- Size: 64×16 pixels
- Use: Status text, footer

### Segment 3 - Custom
- Position: (0, 0)
- Size: 32×16 pixels
- Use: Small info boxes, icons

**Important:** Only one segment should be active at a time in overlapping areas, or visual conflicts will occur.

## Command Sequencing

### Display Text on Segment
```
1. CLEAR_ALL                           # Clear display
2. TEXT|0|Hello|FFFFFF|roboto16|auto|C|none  # Show text
```

### Switch Between Segments
```
1. CLEAR|0                             # Clear segment 0
2. TEXT|1|Top Text|00FF00|roboto12|auto|C|none
3. TEXT|2|Bottom Text|FF0000|roboto12|auto|C|none
```

### Update Text
```
1. TEXT|0|Old Text|FFFFFF|roboto12|auto|C|none
2. TEXT|0|New Text|FFFFFF|roboto12|auto|C|none  # Overwrites
```

## Error Handling

### Invalid Commands
- Silently ignored
- No error response sent
- Check Serial Monitor for debug messages

### Invalid Parameters
- Command ignored if required parameter missing
- Invalid colors default to white
- Invalid fonts default to roboto12
- Invalid segments ignored (0-3 only valid)

### Packet Loss
- UDP is unreliable protocol
- No delivery confirmation
- Sender should implement retries if critical
- Consider sending same command 2-3 times for reliability

## Testing Commands

### Command Line Tools

**Linux/Mac (using netcat):**
```bash
echo "TEXT|0|Test Message|FF0000|roboto16|auto|C|none" | nc -u -w1 192.168.1.100 21324
```

**Windows (PowerShell):**
```powershell
$udpClient = New-Object System.Net.Sockets.UdpClient
$bytes = [System.Text.Encoding]::ASCII.GetBytes("TEXT|0|Test|FFFFFF|roboto12|auto|C|none`n")
$udpClient.Send($bytes, $bytes.Length, "192.168.1.100", 21324)
$udpClient.Close()
```

**Python:**
```python
import socket

def send_udp_command(ip, port, command):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto((command + "\n").encode(), (ip, port))
    sock.close()

# Example usage
send_udp_command("192.168.1.100", 21324, "TEXT|0|Python Test|00FFFF|roboto12|auto|C|none")
```

**Node.js:**
```javascript
const dgram = require('dgram');

function sendCommand(ip, port, command) {
    const client = dgram.createSocket('udp4');
    const message = Buffer.from(command + '\n');
    
    client.send(message, port, ip, (err) => {
        client.close();
        if (err) console.error(err);
    });
}

// Example usage
sendCommand('192.168.1.100', 21324, 'TEXT|0|Node Test|FF00FF|roboto12|auto|C|none');
```

## Integration Examples

### Q-SYS Control Script
```lua
-- Send text to matrix
function DisplayText(segment, text, color)
    local command = string.format("TEXT|%d|%s|%s|roboto12|auto|C|none", segment, text, color)
    UdpSocket:Send(command .. "\n")
end

-- Usage
DisplayText(0, "Room A", "00FF00")
DisplayText(1, "Available", "FFFF00")
```

### Crestron SIMPL+
```c
// UDP Send function
String_Parameter command[255];
UDP_SOCKET socket;

PUSH send_btn
{
    MakeString(command, "TEXT|0|%s|FFFFFF|roboto12|auto|C|none\n", text_input);
    UDPSend(socket, command);
}
```

### Web API (JavaScript)
```javascript
async function sendMatrixCommand(command) {
    const response = await fetch('http://192.168.1.100/api/test', {
        method: 'POST',
        headers: {'Content-Type': 'text/plain'},
        body: command
    });
    return response.text();
}

// Usage
await sendMatrixCommand('TEXT|0|Hello Web|FFFFFF|roboto16|auto|C|none');
```

## Performance Considerations

### Latency
- Network latency: <10ms (local network)
- Processing latency: <20ms
- Rendering latency: <20ms
- **Total latency: ~50ms** from send to display

### Throughput
- Maximum commands per second: ~100
- Recommended rate: 10 commands/second
- For animations, limit to 20 updates/second

### Text Length
- Maximum characters: 128 per segment
- Recommended for readability:
  - 64px wide: 10-12 characters max
  - 32px wide: 5-6 characters max
  - Use scrolling for longer text

## Best Practices

### Command Design
1. **Use auto sizing** - Let firmware calculate optimal size
2. **Center align** - Looks best on matrix displays
3. **High contrast** - Bright text on black background
4. **Short messages** - 3-5 words maximum
5. **Clear before update** - Avoid ghosting

### Network Optimization
1. **Local network only** - Don't route UDP over internet
2. **Static IP** - Assign fixed IP to ESP32
3. **QoS priority** - If supported by network
4. **Test connectivity** - Ping ESP32 before sending

### Color Selection
1. **Avoid pure blue** - Can appear dim on some panels
2. **Use green** - Typically brightest color
3. **Red for alerts** - Universal warning color
4. **White sparingly** - High power consumption

### Effect Usage
1. **Scrolling** - Good for long text
2. **Blinking** - Use for urgent alerts only
3. **Static** - Default for most content
4. **Avoid rapid changes** - Can cause flicker

## Troubleshooting

### Command Not Working

1. **Check syntax**:
   ```bash
   # Correct
   TEXT|0|Hello|FFFFFF|roboto12|auto|C|none
   
   # Wrong (missing parameters)
   TEXT|0|Hello
   ```

2. **Verify network**:
   ```bash
   ping 192.168.1.100
   ```

3. **Test with simple command**:
   ```bash
   echo "CLEAR_ALL" | nc -u -w1 192.168.1.100 21324
   ```

4. **Check Serial Monitor** - Look for received packet messages

### Text Not Visible

1. **Check color** - White text on white background is invisible
2. **Check segment** - Verify correct segment ID
3. **Check size** - Text may be too large for segment
4. **Try different font** - Some fonts render better

### Delayed Response

1. **Network congestion** - Check WiFi signal strength
2. **ESP32 busy** - Reduce command rate
3. **Processing load** - Lower brightness or simplify effects

## Protocol Extensions

### Future Commands (Not Yet Implemented)

**SEGMENT - Define Custom Segment:**
```
SEGMENT|id|x|y|width|height
```

**FONT - Upload Custom Font:**
```
FONT|name|size|data
```

**IMAGE - Display Image:**
```
IMAGE|segment|format|data
```

**ANIMATION - Play Animation:**
```
ANIMATION|segment|type|speed
```

## Security Considerations

- No authentication/encryption
- Use on trusted networks only
- Firewall the UDP port from internet
- Consider VPN for remote access
- No input validation on content (be careful with special characters)

## Compliance

- UDP Protocol: RFC 768
- Character Encoding: ASCII/UTF-8
- Maximum payload: 512 bytes (UDP safe size)

---

**Protocol Version**: 1.0.0  
**Last Updated**: 2026-02-16  
**Status**: Stable

For implementation examples, see the [Q-SYS Integration Guide](QSYS_INTEGRATION.md).
