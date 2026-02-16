# Olimex ESP32 Gateway LED Matrix Text Display

A complete firmware solution for displaying dynamic text on a 64x32 pixel LED matrix using the Olimex ESP32 Gateway. Receives UDP commands from Q-SYS or any UDP client to render text with advanced features including multiple fonts, colors, effects, and multi-segment layouts.

## 🎯 Features

- **UDP Text Control**: Receive and display text via UDP commands (default port 21324)
- **Multi-Segment Support**: Display up to 4 independent text areas
- **Multiple Fonts**: Roboto, Digital, and Monospace fonts in various sizes
- **Auto-Scaling**: Automatically fit text to available space
- **Text Effects**: Scrolling, blinking, fading, and rainbow effects
- **Web Interface**: Configure and test via built-in web server
- **Q-SYS Integration**: Ready-to-use Lua plugin for Q-SYS control
- **Full RGB Color Support**: 24-bit color with hex color codes
- **Persistent Configuration**: Settings saved to flash memory

## 📋 Hardware Requirements

- **Olimex ESP32 Gateway** (ESP32-GATEWAY)
- **64x32 HUB75 LED Matrix Panel** (P3, P4, P5, or P6 pitch)
- **5V Power Supply** (minimum 4A for matrix panel)
- **Ethernet or WiFi** connection

## 🚀 Quick Start

### Arduino IDE Setup

1. **Install Arduino IDE**
   - Download Arduino IDE 1.8.19+ or 2.x from [arduino.cc](https://www.arduino.cc/en/software)
   - Install and launch the IDE

2. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to **File → Preferences**
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to **Tools → Board → Boards Manager**
   - Search for "esp32"
   - Install "esp32" by Espressif Systems (version 2.0.0 or later)

3. **Install Required Libraries**
   
   From **Sketch → Include Library → Manage Libraries**, install:
   - **ESP32 HUB75 LED MATRIX PANEL DMA Display** by mrfaptastic (version 3.0.0+)
   - **Adafruit GFX Library** by Adafruit (version 1.11.0+)
   - **ArduinoJson** by Benoit Blanchon (version 6.x - important!)
   
   Manual installation required for:
   - **ESPAsyncWebServer**: Download from [GitHub](https://github.com/me-no-dev/ESPAsyncWebServer)
     - Download as ZIP, extract to Arduino/libraries/ESPAsyncWebServer
   - **AsyncTCP**: Download from [GitHub](https://github.com/me-no-dev/AsyncTCP)
     - Download as ZIP, extract to Arduino/libraries/AsyncTCP

4. **Clone and Open Project**
   ```bash
   git clone https://github.com/DHPKE/OlimexLED-Matrix.git
   ```
   - Open Arduino IDE
   - Go to **File → Open**
   - Navigate to `OlimexLED-Matrix/arduino/OlimexLED-Matrix/`
   - Open `OlimexLED-Matrix.ino`
   - All required header files are in the same directory

5. **Configure WiFi**
   - Open `config.h` tab in Arduino IDE
   - Update your WiFi credentials:
     ```cpp
     #define WIFI_SSID "YourWiFiNetwork"
     #define WIFI_PASSWORD "YourPassword"
     ```

6. **Configure Board Settings**
   - **Tools → Board** → "ESP32 Dev Module"
   - **Tools → Upload Speed** → "921600"
   - **Tools → Flash Frequency** → "80MHz"
   - **Tools → Flash Mode** → "QIO"
   - **Tools → Flash Size** → "4MB (32Mb)"
   - **Tools → Partition Scheme** → "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
   - **Tools → Core Debug Level** → "None"
   - **Tools → Port** → Select your COM/Serial port

7. **Upload to Board**
   - Connect Olimex ESP32 Gateway via USB
   - Click **Verify** (✓) to compile and check for errors
   - Click **Upload** (→) to flash the firmware
   - Wait for "Done uploading" message

8. **Monitor Serial Output**
   - Open **Tools → Serial Monitor**
   - Set baud rate to **115200**
   - Wait for ESP32 to boot
   - Note the IP address displayed
   - Access web interface at `http://[IP_ADDRESS]`

For detailed setup instructions, see [docs/ARDUINO_SETUP.md](docs/ARDUINO_SETUP.md).

## 🔌 Hardware Connection

### HUB75 Matrix Pinout for Olimex ESP32 Gateway

```
ESP32 GPIO  →  HUB75 Pin   Function
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
GPIO 2      →  R1          Red Data (Upper)
GPIO 15     →  G1          Green Data (Upper)
GPIO 4      →  B1          Blue Data (Upper)
GPIO 16     →  R2          Red Data (Lower)
GPIO 27     →  G2          Green Data (Lower)
GPIO 17     →  B2          Blue Data (Lower)
GPIO 5      →  A           Row Address A
GPIO 18     →  B           Row Address B
GPIO 19     →  C           Row Address C
GPIO 21     →  D           Row Address D
GPIO 26     →  LAT         Latch
GPIO 25     →  OE          Output Enable
GPIO 22     →  CLK         Clock

GND         →  GND         Ground
5V Ext      →  5V          Power (from external PSU)
```

**⚠️ Important Notes:**
- **Do NOT power the LED matrix from ESP32!** Use external 5V power supply (4-10A depending on matrix size)
- Connect GND of power supply to GND of ESP32 Gateway
- Double-check all connections before powering on
- See [docs/PINOUT.md](docs/PINOUT.md) for detailed wiring diagram

## 🌐 Web Interface

After uploading, access the web interface:

1. Open Serial Monitor to find the IP address
2. Navigate to `http://[IP_ADDRESS]` in your web browser
3. Use the interface to:
   - Send text to different segments
   - Change colors and fonts
   - Adjust brightness
   - Test UDP commands

## 📡 UDP Protocol

Send UDP packets to port **21324** with the following format:

### TEXT Command
```
TEXT|segment|content|color|font|size|align|effect
```

**Parameters:**
- `segment`: 0-3 (segment ID)
- `content`: Text to display
- `color`: Hex color code (RRGGBB, without #)
- `font`: Font name (roboto12, roboto16, roboto24, digital12, mono9)
- `size`: Font size (6-32) or "auto"
- `align`: L (left), C (center), R (right)
- `effect`: none, scroll, blink, fade, rainbow

**Examples:**
```
TEXT|0|Hello World|FF0000|roboto16|auto|C|none
TEXT|1|Temperature: 72.5|00FF00|digital12|auto|L|none
TEXT|2|ALERT|FF0000|roboto24|auto|C|blink
```

### Other Commands
```
CLEAR|segment          - Clear specific segment
CLEAR_ALL              - Clear all segments
BRIGHTNESS|value       - Set brightness (0-255)
```

### Testing with Command Line

**Linux/Mac:**
```bash
echo "TEXT|0|Hello World|FFFFFF|roboto12|auto|C|none" | nc -u -w1 192.168.1.100 21324
```

**Windows (PowerShell):**
```powershell
$udpClient = New-Object System.Net.Sockets.UdpClient
$bytes = [System.Text.Encoding]::ASCII.GetBytes("TEXT|0|Hello World|FFFFFF|roboto12|auto|C|none`n")
$udpClient.Send($bytes, $bytes.Length, "192.168.1.100", 21324)
$udpClient.Close()
```

## 🎛️ Q-SYS Plugin

A complete Q-SYS plugin is included for easy integration with Q-SYS systems.

### Installation

1. Copy `qsys-plugin/led_matrix_controller.lua` to your Q-SYS plugin folder
2. In Q-SYS Designer:
   - Drag the plugin from the "Plugins" schematic library
   - Configure IP address and UDP port in plugin properties
   - Create control links to your UCI or other controls

### Features

- 4 independent text segments with individual controls
- Color pickers (hex input)
- Font selection dropdowns
- Send and Clear buttons per segment
- Global brightness control
- Clear All button
- Connection status indicator

See [docs/QSYS_INTEGRATION.md](docs/QSYS_INTEGRATION.md) for detailed setup instructions.

## 📁 Project Structure

```
OlimexLED-Matrix/
├── arduino/
│   └── OlimexLED-Matrix/
│       ├── OlimexLED-Matrix.ino  # Main Arduino sketch
│       ├── config.h              # Hardware pin configuration
│       ├── segment_manager.h     # Segment layout management
│       ├── text_renderer.h       # Text rendering engine
│       ├── udp_handler.h         # UDP protocol handler
│       └── fonts.h               # Font definitions
├── src/                          # Original source (for reference)
├── lib/                          # Original libraries (for reference)
├── data/                         # Web interface files (optional)
├── qsys-plugin/
│   └── led_matrix_controller.lua # Q-SYS plugin
├── docs/
│   ├── ARDUINO_SETUP.md          # Arduino IDE setup guide
│   ├── UDP_PROTOCOL.md           # Complete protocol specification
│   ├── QSYS_INTEGRATION.md       # Q-SYS integration guide
│   └── PINOUT.md                 # Hardware wiring diagram
├── examples/                     # Example scripts for UDP control
└── README.md                     # This file
```

## 🔧 Configuration

### WiFi Configuration

Edit `arduino/OlimexLED-Matrix/config.h`:
```cpp
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"
```

### Matrix Size

Default is 64x32. To change, edit `arduino/OlimexLED-Matrix/config.h`:
```cpp
#define LED_MATRIX_WIDTH 64
#define LED_MATRIX_HEIGHT 32
```

### UDP Port

Default is 21324. To change, edit `arduino/OlimexLED-Matrix/config.h`:
```cpp
#define UDP_PORT 21324
```

### Pin Configuration

If using different GPIO pins, edit the pin definitions in `arduino/OlimexLED-Matrix/config.h`.

## 📊 Segment Layouts

### Default Segments

- **Segment 0**: Full screen (64x32)
- **Segment 1**: Top half (64x16)
- **Segment 2**: Bottom half (64x16)
- **Segment 3**: Custom (32x16)

Segments can be reconfigured in `src/segment_manager.h` by modifying the `initDefaultLayout()` function.

## 🎨 Available Fonts

- **roboto6**: Small Roboto-style font (6pt)
- **roboto8**: Roboto-style font (8-9pt)
- **roboto12**: Medium Roboto-style font (12pt)
- **roboto16**: Large Roboto-style font (16-18pt)
- **roboto24**: Extra large Roboto-style font (24pt)
- **digital12**: Digital/LED style font (12pt)
- **digital24**: Large digital font (24pt)
- **mono9**: Monospace font (9pt)
- **mono12**: Monospace font (12pt)

## 🐛 Troubleshooting

### Matrix doesn't light up
- Check power supply (must be external 5V, 4A+)
- Verify HUB75 cable connections
- Check GPIO pin assignments in config.h

### WiFi connection fails
- Verify SSID and password in config.h
- Device will create AP "OlimexLED-Matrix" if connection fails
- Connect to AP (password: 12345678) and access 192.168.4.1

### UDP commands not working
- Check firewall settings
- Verify IP address and port (21324)
- Test with command line tools first
- Check Serial Monitor for received packets

### Text not displaying correctly
- Verify font name is correct
- Try auto size instead of fixed size
- Check segment is activated (isActive = true)
- Verify color is not same as background

### Web interface not loading
- LittleFS may not be initialized
- The HTML is embedded in code and should work without filesystem
- Try accessing /api/config to test API

## 📈 Performance

- **Refresh Rate**: 100+ Hz (flicker-free)
- **UDP Latency**: <50ms from packet receipt to display update
- **Color Depth**: 24-bit RGB (displayed as 16-bit RGB565)
- **Max Segments**: 4 (configurable)
- **Max Text Length**: 128 characters per segment

## 🔐 Security Notes

- Default WiFi AP password should be changed
- UDP protocol has no authentication (use trusted network)
- Web interface has no password protection
- Recommended for internal/private networks only

## 📝 License

This project is open source. See LICENSE file for details.

## 🤝 Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## 📞 Support

- GitHub Issues: [Report bugs or request features](https://github.com/DHPKE/OlimexLED-Matrix/issues)
- Documentation: See `docs/` folder for detailed guides

## 🙏 Acknowledgments

- Based on WLED firmware concepts
- Uses [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA) library
- Adafruit GFX Library for text rendering
- Q-SYS plugin framework by QSC

## 📌 Version History

### v1.0.0 (2026-02-16)
- Initial release
- UDP text protocol implementation
- Multi-segment support
- Web interface
- Q-SYS plugin
- Multiple font support
- Text effects (scroll, blink, fade)
- Auto-scaling
- Persistent configuration

---

**Made with ❤️ for the Olimex ESP32 Gateway community**
