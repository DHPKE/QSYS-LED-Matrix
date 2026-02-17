# Visual Studio Code Setup for QSYS-LED-Matrix

## Prerequisites

1. **Visual Studio Code** installed
2. **PlatformIO IDE extension** for VSCode

## Install PlatformIO IDE Extension

1. Open VSCode
2. Go to Extensions (Cmd+Shift+X or click the Extensions icon in the sidebar)
3. Search for "PlatformIO IDE"
4. Click **Install** on the official PlatformIO extension
5. Reload VSCode when prompted

## Project Structure

```
QSYS-LED-Matrix/
├── platformio.ini          # PlatformIO configuration
├── src/                    # Source files
│   ├── main.cpp           # Main Arduino sketch (renamed from .ino)
│   ├── config.h           # Hardware & network configuration
│   ├── segment_manager.h  # Segment management
│   ├── text_renderer.h    # Text rendering
│   ├── udp_handler.h      # UDP command handler
│   └── fonts.h            # Font definitions
├── lib/                    # Local libraries
│   ├── AsyncTCP/          # ESP32 async TCP library
│   └── ESPAsyncWebServer/ # Async web server library
└── arduino/                # Original Arduino IDE files (backup)
```

## Configuration

### 1. WiFi Credentials

Edit `src/config.h`:

```cpp
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"
```

### 2. Matrix Size (if different from 64x32)

Edit `src/config.h`:

```cpp
#define LED_MATRIX_WIDTH 64
#define LED_MATRIX_HEIGHT 32
```

### 3. UDP Port (default: 21324)

Edit `src/config.h`:

```cpp
#define UDP_PORT 21324
```

## Building & Uploading

### Using PlatformIO GUI (Recommended)

1. Open the PlatformIO panel (click the alien icon in the left sidebar)
2. Under **Project Tasks**, expand **esp32dev**
3. Click **Build** to compile
4. Connect your ESP32-GATEWAY via USB
5. Click **Upload** to flash the firmware
6. Click **Monitor** to view serial output

### Using PlatformIO Terminal

Open VSCode Terminal (Ctrl+` or Cmd+`) and run:

```bash
# Build only
pio run

# Build and upload
pio run --target upload

# Upload and monitor
pio run --target upload --target monitor

# Clean build
pio run --target clean
```

### Keyboard Shortcuts (with PlatformIO extension)

- **Build**: Ctrl+Alt+B (Cmd+Option+B on Mac)
- **Upload**: Ctrl+Alt+U (Cmd+Option+U on Mac)
- **Serial Monitor**: Ctrl+Alt+S (Cmd+Option+S on Mac)

## Board Configuration

The `platformio.ini` is pre-configured for **ESP32 Dev Module** with:

- Platform: Espressif32
- Framework: Arduino
- Upload speed: 921600
- Monitor speed: 115200
- Flash size: 4MB

## Library Dependencies

Libraries are automatically installed on first build:

1. **ESP32 HUB75 LED MATRIX PANEL DMA Display** (^3.0.9)
2. **Adafruit GFX Library** (^1.11.9)
3. **ArduinoJson** (^6.21.5)
4. **AsyncTCP** (local in lib/)
5. **ESPAsyncWebServer** (local in lib/)

## Troubleshooting

### PlatformIO Extension Not Loading

1. Restart VSCode
2. Check Python is installed: `python3 --version`
3. Reinstall PlatformIO extension

### Board Not Detected

1. Connect ESP32-GATEWAY via USB
2. Check serial port in bottom status bar
3. Try clicking on the port to select manually
4. On macOS: Usually `/dev/cu.usbserial-*`
5. On Linux: Add user to dialout group: `sudo usermod -a -G dialout $USER`

### Upload Failed

1. Hold BOOT button while clicking Upload
2. Try lower upload speed (edit platformio.ini):
   ```ini
   upload_speed = 460800  # or 115200
   ```
3. Check USB cable (some are power-only)

### Compilation Errors

1. Clean build: `pio run --target clean`
2. Delete `.pio` folder and rebuild
3. Check WiFi credentials don't contain special characters

## IntelliSense Configuration

PlatformIO automatically configures IntelliSense. If autocomplete isn't working:

1. Open Command Palette (Cmd+Shift+P)
2. Type "PlatformIO: Rebuild IntelliSense Index"
3. Wait for indexing to complete

## Serial Monitor

After upload, open Serial Monitor (Cmd+Option+S) to see:

```
==================================
Olimex LED Matrix Text Display
Version: 1.2.0
==================================

✓ LittleFS mounted successfully
✓ Configuration loaded
Initializing LED Matrix...
✓ LED Matrix initialized
✓ Matrix size: 64x32
Connecting to WiFi........
✓ WiFi connected
  IP Address: 192.168.1.100
Starting UDP listener...
✓ UDP listening on port 21324
Starting web server...
✓ Web server started on port 80

==================================
System Ready!
==================================
```

## Next Steps

1. Note the IP address from serial monitor
2. Access web interface: `http://[IP_ADDRESS]`
3. Test UDP commands from Q-SYS or Python
4. Configure Q-SYS plugin with ESP32 IP address

## Alternative: Arduino IDE

If you prefer Arduino IDE, the original sketch is in:
- `arduino/QSYS-LED-Matrix/QSYS-LED-Matrix.ino`
- See `docs/ARDUINO_SETUP.md` for Arduino IDE instructions

## Resources

- **PlatformIO Docs**: https://docs.platformio.org/
- **Project README**: [README.md](README.md)
- **Hardware Setup**: [docs/HARDWARE_SETUP.md](docs/HARDWARE_SETUP.md)
- **UDP Protocol**: [docs/UDP_PROTOCOL.md](docs/UDP_PROTOCOL.md)
