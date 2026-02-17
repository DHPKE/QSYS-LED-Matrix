# Arduino IDE Setup Guide

Complete guide for setting up the Olimex ESP32 Gateway LED Matrix Text Display project in Arduino IDE.

## Table of Contents

- [Prerequisites](#prerequisites)
- [ESP32 Board Installation](#esp32-board-installation)
- [Library Installation](#library-installation)
- [Project Setup](#project-setup)
- [Configuration](#configuration)
- [Board Configuration](#board-configuration)
- [Upload Process](#upload-process)
- [Serial Monitor](#serial-monitor)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Arduino IDE Version

This project requires:
- **Arduino IDE 1.8.19 or later** (recommended: 1.8.19)
- **Arduino IDE 2.x** (recommended: 2.3.0 or later)

### Download Links

- **Windows/Mac/Linux**: [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
- Choose the installer appropriate for your operating system

### Operating System Notes

**Windows:**
- Use the Windows Installer for easiest setup
- Driver installation is automatic
- USB drivers for ESP32 are included with ESP32 board support

**macOS:**
- Use the macOS DMG installer
- May need to allow app in Security & Privacy settings
- CH340/CP2102 drivers usually work out of the box

**Linux:**
- Download AppImage or use package manager
- Add user to dialout group: `sudo usermod -a -G dialout $USER`
- Log out and back in for group changes to take effect

## ESP32 Board Installation

### Step 1: Add ESP32 Board Manager URL

1. Open Arduino IDE
2. Go to **File → Preferences** (Arduino IDE 1.x) or **Arduino IDE → Settings** (Arduino IDE 2.x)
3. Find the "Additional Board Manager URLs" field
4. Add the following URL:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
5. If you already have other URLs, separate them with commas or click the window icon to add on a new line
6. Click **OK**

### Step 2: Open Boards Manager

**Arduino IDE 1.x:**
- Go to **Tools → Board → Boards Manager**

**Arduino IDE 2.x:**
- Click the Boards Manager icon in the left sidebar (second icon from top)
- Or go to **Tools → Board → Boards Manager**

### Step 3: Install ESP32 Package

1. In the Boards Manager, search for "esp32"
2. Find "esp32" by Espressif Systems
3. Select version 2.0.0 or later (recommended: 2.0.11 or 3.0.0+)
4. Click **Install**
5. Wait for installation to complete (may take several minutes)

### Step 4: Verify Installation

1. Go to **Tools → Board**
2. You should now see "ESP32 Arduino" section with many ESP32 boards
3. Look for "ESP32 Dev Module" - this is what we'll use

## Library Installation

### Libraries from Library Manager

Go to **Sketch → Include Library → Manage Libraries** and install the following:

#### 1. ESP32 HUB75 LED MATRIX PANEL DMA Display

- **Library Name**: ESP32 HUB75 LED MATRIX PANEL DMA Display
- **Author**: mrfaptastic
- **Version**: 3.0.0 or later
- **Search term**: "HUB75" or "LED Matrix"

**Installation:**
1. Search for "ESP32 HUB75"
2. Select "ESP32 HUB75 LED MATRIX PANEL DMA Display" by mrfaptastic
3. Click **Install**
4. If prompted to install dependencies, click **Install All**

#### 2. Adafruit GFX Library

- **Library Name**: Adafruit GFX Library
- **Author**: Adafruit
- **Version**: 1.11.0 or later
- **Search term**: "Adafruit GFX"

**Installation:**
1. Search for "Adafruit GFX"
2. Select "Adafruit GFX Library" by Adafruit
3. Click **Install**
4. If prompted to install dependencies (Adafruit BusIO), click **Install All**

#### 3. ArduinoJson

- **Library Name**: ArduinoJson
- **Author**: Benoit Blanchon
- **Version**: 6.x (IMPORTANT: Version 7.x is NOT compatible)
- **Recommended**: 6.21.0 or later in the 6.x series

**Installation:**
1. Search for "ArduinoJson"
2. Select "ArduinoJson" by Benoit Blanchon
3. **Make sure version starts with 6** (e.g., 6.21.5)
4. Click **Install**

⚠️ **Important:** ArduinoJson version 7.x has breaking changes. This project requires version 6.x.

### Libraries Requiring Manual Installation

Two libraries must be installed manually as they're not available in the Library Manager.

#### 4. ESPAsyncWebServer

**Download:**
1. Go to [https://github.com/me-no-dev/ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
2. Click the green **Code** button
3. Select **Download ZIP**

**Installation:**
1. Extract the ZIP file
2. Rename the extracted folder from "ESPAsyncWebServer-master" to "ESPAsyncWebServer"
3. Move the folder to your Arduino libraries directory:
   - **Windows**: `C:\Users\[YourUsername]\Documents\Arduino\libraries\`
   - **macOS**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`
4. Restart Arduino IDE

#### 5. AsyncTCP

**Download:**
1. Go to [https://github.com/me-no-dev/AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
2. Click the green **Code** button
3. Select **Download ZIP**

**Installation:**
1. Extract the ZIP file
2. Rename the extracted folder from "AsyncTCP-master" to "AsyncTCP"
3. Move the folder to your Arduino libraries directory (same location as above)
4. Restart Arduino IDE

### Verify Library Installation

After restarting Arduino IDE:
1. Go to **Sketch → Include Library**
2. Scroll through the list and verify you see:
   - ESP32 HUB75 LED MATRIX PANEL DMA Display
   - Adafruit GFX Library
   - ArduinoJson
   - ESPAsyncWebServer
   - AsyncTCP

## Project Setup

### Download/Clone Repository

**Option A: Download ZIP**
1. Go to [https://github.com/DHPKE/QSYS-LED-Matrix](https://github.com/DHPKE/QSYS-LED-Matrix)
2. Click the green **Code** button
3. Select **Download ZIP**
4. Extract to a convenient location

**Option B: Clone with Git**
```bash
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
```

### Open the Sketch

1. Open Arduino IDE
2. Go to **File → Open**
3. Navigate to: `QSYS-LED-Matrix/arduino/QSYS-LED-Matrix/`
4. Select `QSYS-LED-Matrix.ino`
5. Click **Open**

### Required Files

The Arduino IDE will automatically detect all files in the same directory. You should see tabs for:
- `QSYS-LED-Matrix` (main sketch)
- `config.h`
- `segment_manager.h`
- `text_renderer.h`
- `udp_handler.h`
- `fonts.h`

All these files must be in the same directory for the sketch to compile.

## Configuration

### WiFi Credentials

1. Click on the `config.h` tab in Arduino IDE
2. Find these lines near the top:
   ```cpp
   #define WIFI_SSID "YOUR_SSID"
   #define WIFI_PASSWORD "YOUR_PASSWORD"
   ```
3. Replace with your WiFi credentials:
   ```cpp
   #define WIFI_SSID "MyHomeNetwork"
   #define WIFI_PASSWORD "MySecretPassword"
   ```

### Matrix Size Configuration

Default is 64x32 pixels. To change, edit in `config.h`:

```cpp
#define LED_MATRIX_WIDTH 64
#define LED_MATRIX_HEIGHT 32
```

Common sizes:
- **64x32** (default) - Most common
- **64x64** - Square panel
- **128x32** - Wide panel (2 panels chained)
- **128x64** - Large panel (4 panels chained)

### UDP Port Configuration

Default UDP port is 21324. To change, edit in `config.h`:

```cpp
#define UDP_PORT 21324
```

### Pin Configuration for Olimex Gateway

The pin configuration is pre-configured for Olimex ESP32 Gateway. Only change if you're using different hardware.

In `config.h`, the pins are defined as:
```cpp
#define R1_PIN 2
#define G1_PIN 15
#define B1_PIN 4
#define R2_PIN 16
#define G2_PIN 27
#define B2_PIN 17
#define A_PIN 5
#define B_PIN 18
#define C_PIN 19
#define D_PIN 21
#define LAT_PIN 26
#define OE_PIN 25
#define CLK_PIN 22
```

## Board Configuration

Before uploading, configure the board settings in Arduino IDE:

### Required Settings

Go to **Tools** menu and configure:

1. **Board**: "ESP32 Dev Module"
   - Look under "ESP32 Arduino" section
   - Select "ESP32 Dev Module"

2. **Upload Speed**: "921600"
   - Fastest upload speed supported
   - Use "460800" if you experience upload issues

3. **Flash Frequency**: "80MHz"
   - Standard frequency for ESP32

4. **Flash Mode**: "QIO"
   - Quad I/O mode for faster flash access

5. **Flash Size**: "4MB (32Mb)"
   - Olimex Gateway has 4MB flash

6. **Partition Scheme**: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
   - Provides space for firmware and file system
   - Alternative: "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)" if you need more program space

7. **Core Debug Level**: "None"
   - Use "Debug" or "Verbose" only for troubleshooting

8. **Port**: Select your COM/Serial port
   - **Windows**: Usually `COM3`, `COM4`, etc.
   - **macOS**: Usually `/dev/cu.usbserial-*` or `/dev/cu.wchusbserial*`
   - **Linux**: Usually `/dev/ttyUSB0` or `/dev/ttyACM0`

### Optional Settings

These can usually be left at default:
- **CPU Frequency**: "240MHz" (default)
- **PSRAM**: "Disabled" (Olimex Gateway doesn't have PSRAM)
- **Arduino Runs On**: "Core 1" (default)
- **Events Run On**: "Core 1" (default)

## Upload Process

### Connect Olimex Gateway

1. Connect Olimex ESP32 Gateway to your computer via USB
2. Wait for drivers to install (Windows)
3. Verify the port appears in **Tools → Port**

### Verify Sketch (Optional but Recommended)

1. Click the **Verify** button (✓) or press `Ctrl+R` / `Cmd+R`
2. Wait for compilation to complete
3. Check the output window for errors
4. If successful, you'll see "Done compiling"

### Upload to Board

1. Click the **Upload** button (→) or press `Ctrl+U` / `Cmd+U`
2. Arduino IDE will compile and then upload
3. You'll see progress in the output window:
   - "Compiling sketch..."
   - "Uploading..."
   - Upload percentage
4. Wait for "Done uploading" message

**Upload typically takes 30-60 seconds for this project.**

### Common Upload Messages

**Success:**
```
Hard resetting via RTS pin...
Done uploading.
```

**Failure examples:**
```
A fatal error occurred: Failed to connect to ESP32
```
See Troubleshooting section below.

## Serial Monitor

### Opening Serial Monitor

**Arduino IDE 1.x:**
- Click **Tools → Serial Monitor** or press `Ctrl+Shift+M` / `Cmd+Shift+M`

**Arduino IDE 2.x:**
- Click the Serial Monitor icon in the top right (looks like a magnifying glass)
- Or go to **Tools → Serial Monitor**

### Configure Serial Monitor

1. Set baud rate to **115200** (bottom-right dropdown)
2. Set line ending to "Newline" or "Both NL & CR"

### Expected Output

After upload and reset, you should see:

```
==================================
Olimex LED Matrix Text Display
==================================

✓ LittleFS mounted successfully
✓ Configuration loaded
Initializing LED Matrix...
✓ LED Matrix initialized
✓ Matrix size: 64x32
Connecting to WiFi........
✓ WiFi connected
  IP Address: 192.168.1.100
  MAC Address: XX:XX:XX:XX:XX:XX
Starting UDP listener...
✓ UDP listening on port 21324
Starting web server...
✓ Web server started on port 80

==================================
System Ready!
==================================
IP Address: 192.168.1.100
UDP Port: 21324
Web Interface: http://192.168.1.100
==================================
```

### Finding IP Address

The IP address is displayed in the Serial Monitor after "IP Address:". You'll need this to:
- Access the web interface
- Send UDP commands
- Configure Q-SYS plugin

## Troubleshooting

### Board Not Found Errors

**Problem:** ESP32 Dev Module not appearing in Tools → Board

**Solutions:**
1. Verify ESP32 board package is installed (see ESP32 Board Installation)
2. Restart Arduino IDE
3. Check Board Manager URL is correct
4. Try re-installing ESP32 board package

### Library Conflicts

**Problem:** Compilation errors mentioning missing libraries

**Solutions:**
1. Verify all 5 libraries are installed (3 from Library Manager, 2 manual)
2. Check ArduinoJson is version 6.x (not 7.x)
3. Restart Arduino IDE after installing libraries
4. Try deleting and re-installing problematic library

**Problem:** "Multiple libraries found for X"

**Solutions:**
1. Go to Arduino libraries folder
2. Remove duplicate library folders
3. Keep only one version of each library

### Compilation Errors

**Problem:** "undefined reference to" or "not declared in this scope"

**Solutions:**
1. Make sure all 6 files are in the same directory
2. Verify `fonts.h` is present in the sketch directory
3. Check WiFi credentials don't contain special characters that need escaping

**Problem:** "ArduinoJson.h: No such file or directory"

**Solutions:**
1. Install ArduinoJson library from Library Manager
2. Verify version is 6.x (not 7.x)

### Upload Failures

**Problem:** "A fatal error occurred: Failed to connect to ESP32"

**Solutions:**
1. Check USB cable is properly connected
2. Try a different USB port
3. Verify correct port is selected in Tools → Port
4. Hold BOOT button on ESP32 while clicking Upload
5. Try lower upload speed (460800 or 115200)
6. Check if another program is using the serial port
7. Restart Arduino IDE

**Problem:** "Timed out waiting for packet header"

**Solutions:**
1. Hold BOOT button during upload
2. Reduce upload speed to 460800 or 115200
3. Try a different USB cable (some are power-only)
4. Disable any VPN or security software temporarily

**Problem:** "espcomm_upload_mem failed"

**Solutions:**
1. Select correct board: "ESP32 Dev Module"
2. Check flash size is set to "4MB (32Mb)"
3. Try different partition scheme

### Serial Port Selection Issues

**Problem:** No port appears in Tools → Port menu

**Windows Solutions:**
1. Check Device Manager for "Ports (COM & LPT)"
2. Look for "USB-SERIAL CH340" or similar
3. Install CH340 drivers if needed: [CH340 Driver](https://sparks.gogo.co.nz/ch340.html)
4. Try different USB port
5. Restart computer

**macOS Solutions:**
1. Check System Information → USB for connected device
2. Install CH340 drivers if needed: [CH340 Driver](https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver)
3. Check System Preferences → Security & Privacy for driver approval

**Linux Solutions:**
1. Check user is in dialout group: `groups $USER`
2. Add to group if missing: `sudo usermod -a -G dialout $USER`
3. Log out and back in
4. Verify port exists: `ls -l /dev/ttyUSB*`
5. Check permissions: `sudo chmod 666 /dev/ttyUSB0`

### Runtime Issues

**Problem:** ESP32 resets repeatedly (boot loop)

**Solutions:**
1. Check power supply can provide enough current
2. Verify matrix panel has separate 5V power supply
3. Reduce brightness in code temporarily
4. Check for wiring short circuits

**Problem:** WiFi connection fails

**Solutions:**
1. Verify SSID and password are correct in config.h
2. Check WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
3. Move ESP32 closer to router
4. Check for special characters in password (use escape sequences)
5. Try AP mode if connection fails (SSID: "QSYS-LED-Matrix", password: "12345678")

**Problem:** Matrix displays nothing

**Solutions:**
1. Check matrix has power supply connected
2. Verify wiring matches pinout in config.h
3. Check matrix power supply is 5V (not 12V or 24V)
4. Try sending test command via web interface or UDP

**Problem:** Web interface not accessible

**Solutions:**
1. Check IP address in Serial Monitor
2. Verify ESP32 and computer are on same network
3. Try accessing via `http://[IP_ADDRESS]` (not https)
4. Disable any VPN on your computer
5. Check firewall isn't blocking port 80

### Getting Help

If you're still experiencing issues:

1. Check existing GitHub issues: [https://github.com/DHPKE/QSYS-LED-Matrix/issues](https://github.com/DHPKE/QSYS-LED-Matrix/issues)
2. Create a new issue with:
   - Arduino IDE version
   - ESP32 board package version
   - Complete error message
   - Serial Monitor output
   - Steps you've already tried
3. Include relevant configuration (WiFi SSID removed)

## Next Steps

After successful upload:

1. **Test Web Interface**: Open `http://[IP_ADDRESS]` in your browser
2. **Test UDP Commands**: Use the example Python script in `examples/send_text.py`
3. **Configure Q-SYS**: Follow instructions in `docs/QSYS_INTEGRATION.md`
4. **Hardware Setup**: See `docs/HARDWARE_SETUP.md` for wiring details

## Additional Resources

- **Hardware Wiring**: [docs/HARDWARE_SETUP.md](HARDWARE_SETUP.md)
- **UDP Protocol**: [docs/UDP_PROTOCOL.md](UDP_PROTOCOL.md)
- **Q-SYS Integration**: [docs/QSYS_INTEGRATION.md](QSYS_INTEGRATION.md)
- **ESP32 Arduino Core**: [https://github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
- **HUB75 Library**: [https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA)
