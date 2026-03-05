# LED Matrix Control Examples

This directory contains example scripts for controlling the Olimex LED Matrix from various programming languages and platforms.

## Available Examples

### Python Script (`send_text.py`)

Full-featured Python script with command-line arguments.

**Installation:**
```bash
# No dependencies required (uses standard library)
python3 send_text.py "Hello World"
```

**Usage:**
```bash
# Basic text display
python3 send_text.py "Hello World"

# Custom color and segment
python3 send_text.py "Alert!" -s 1 -c FF0000

# Scrolling text
python3 send_text.py "Long scrolling message..." -e scroll

# Clear all
python3 send_text.py --clear-all

# Set brightness
python3 send_text.py -b 128
```

**Full Options:**
```bash
python3 send_text.py --help
```

### Bash Script (`send_command.sh`)

Simple bash script for quick commands.

**Requirements:**
- `netcat` (nc) command

**Installation:**
```bash
# Ubuntu/Debian
sudo apt install netcat

# macOS
brew install netcat
```

**Usage:**
```bash
# Basic usage
./send_command.sh "Hello World"

# With options
./send_command.sh -s 1 -c FF0000 "Alert!"

# Clear display
./send_command.sh --clear-all

# Set brightness
./send_command.sh -b 128
```

**Environment Variables:**
```bash
export LED_MATRIX_IP="192.168.1.100"
export LED_MATRIX_PORT="21324"
./send_command.sh "Hello"
```

### Node.js Module (`led_matrix_client.js`)

JavaScript/Node.js client library.

**Installation:**
```bash
# No dependencies required (uses built-in dgram)
node led_matrix_client.js
```

**Usage as Module:**
```javascript
const LEDMatrixClient = require('./led_matrix_client');

const matrix = new LEDMatrixClient('192.168.1.100', 21324);

// Send text
await matrix.sendText(0, 'Hello', 'FFFFFF', 'roboto12', 'auto', 'C', 'none');

// Clear all
await matrix.clearAll();

// Set brightness
await matrix.setBrightness(128);

matrix.close();
```

**Run Example:**
```bash
node led_matrix_client.js
```

## Quick Start Guide

### 1. Find Your Matrix IP Address

Check the Serial Monitor output when ESP32 boots, or check your router's DHCP leases.

### 2. Test Connection

```bash
# Ping the matrix
ping 192.168.1.100

# Simple test (bash)
echo "TEXT|0|TEST|FFFFFF|roboto12|auto|C|none" | nc -u -w1 192.168.1.100 21324
```

### 3. Send Your First Message

**Python:**
```bash
python3 examples/send_text.py "Hello Matrix!"
```

**Bash:**
```bash
./examples/send_command.sh "Hello Matrix!"
```

**Node.js:**
```bash
node examples/led_matrix_client.js
```

## Common Use Cases

### Display Temperature

**Python:**
```bash
python3 send_text.py "72.5°F" -s 1 -c 00FF00 -f digital12
```

**Bash:**
```bash
./send_command.sh -s 1 -c 00FF00 -f digital12 "72.5°F"
```

### Display Alert

**Python:**
```bash
python3 send_text.py "ALERT" -c FF0000 -f roboto24 -e blink
```

### Multi-Segment Display

**Python:**
```python
import subprocess

# Segment 0: Room name
subprocess.run(['python3', 'send_text.py', 'Conference A', '-s', '0', '-c', 'FFFFFF'])

# Segment 1: Status
subprocess.run(['python3', 'send_text.py', 'Available', '-s', '1', '-c', '00FF00'])

# Segment 2: Time
subprocess.run(['python3', 'send_text.py', '14:30', '-s', '2', '-c', '00FFFF', '-f', 'digital12'])
```

### Scrolling Ticker

**Bash:**
```bash
./send_command.sh -e scroll "Latest News: Stock market up 5% today..."
```

## Integration Examples

### Cron Job (Linux)

Display time every minute:

```bash
# Add to crontab (crontab -e)
* * * * * /usr/bin/python3 /path/to/send_text.py "$(date +'%H:%M')" -s 0 -c 00FFFF -f digital12
```

### Home Assistant

```yaml
# configuration.yaml
shell_command:
  led_matrix_display: '/usr/bin/python3 /path/to/send_text.py "{{ message }}" -s {{ segment }} -c {{ color }}'

# automation
automation:
  - alias: "Display temperature on matrix"
    trigger:
      platform: state
      entity_id: sensor.living_room_temperature
    action:
      service: shell_command.led_matrix_display
      data:
        message: "{{ states('sensor.living_room_temperature') }}°F"
        segment: 1
        color: "00FF00"
```

### Node-RED

Use the "exec" node:

```
Command: python3 /path/to/send_text.py
Arguments: "Your message" -s 0 -c FFFFFF
```

Or use the "udp out" node with function:

```javascript
var segment = 0;
var text = msg.payload;
var color = "FFFFFF";
var font = "roboto12";

msg.payload = `TEXT|${segment}|${text}|${color}|${font}|auto|C|none\n`;
msg.ip = "192.168.1.100";
msg.port = 21324;

return msg;
```

### Bash Script with Sensor Input

```bash
#!/bin/bash
# Display CPU temperature

TEMP=$(cat /sys/class/thermal/thermal_zone0/temp)
TEMP_C=$((TEMP / 1000))

if [ $TEMP_C -gt 70 ]; then
    COLOR="FF0000"  # Red
elif [ $TEMP_C -gt 60 ]; then
    COLOR="FFA500"  # Orange
else
    COLOR="00FF00"  # Green
fi

./send_command.sh -s 1 -c $COLOR "CPU: ${TEMP_C}°C"
```

### Python Weather Display

```python
#!/usr/bin/env python3
import requests
import subprocess

API_KEY = "your_openweather_api_key"
CITY = "London"

response = requests.get(
    f"http://api.openweathermap.org/data/2.5/weather?q={CITY}&appid={API_KEY}&units=imperial"
)
weather = response.json()

temp = weather['main']['temp']
description = weather['weather'][0]['description']

# Display on matrix
subprocess.run([
    'python3', 'send_text.py',
    f"{description} {temp}°F",
    '-s', '0',
    '-c', '00FFFF',
    '-f', 'roboto12'
])
```

## Troubleshooting

### Command Not Received

1. **Check IP address:**
   ```bash
   ping 192.168.1.100
   ```

2. **Check port:**
   ```bash
   nc -zvu 192.168.1.100 21324
   ```

3. **Check firewall:**
   ```bash
   # Linux
   sudo ufw allow 21324/udp
   
   # macOS
   # System Preferences > Security & Privacy > Firewall > Firewall Options
   ```

### Script Permission Denied

```bash
chmod +x send_command.sh
chmod +x send_text.py
```

### Python ModuleNotFoundError

Python scripts use only standard library, no installation needed. Ensure Python 3.x is installed:

```bash
python3 --version
```

### Netcat Not Found

```bash
# Ubuntu/Debian
sudo apt install netcat-traditional

# macOS
brew install netcat

# Alpine
apk add netcat-openbsd
```

## Advanced Examples

See the [docs/](../docs/) folder for more advanced integration examples:

- Q-SYS Integration
- UDP Protocol Specification
- Custom Automation Scripts

## Contributing

Have a useful example? Submit a pull request!

## License

MIT License - See LICENSE file for details
