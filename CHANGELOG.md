# Changelog

All notable changes to the Olimex LED Matrix Text Display project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-02-16

### Breaking Changes
- **REMOVED**: PlatformIO support and configuration files
- Project now exclusively uses Arduino IDE

### Changed
- **Restructured** project for Arduino IDE as primary development environment
- **Moved** main firmware from `src/main.cpp` to `arduino/QSYS-LED-Matrix/QSYS-LED-Matrix.ino`
- **Copied** all header files and fonts to Arduino sketch directory
- **Updated** all documentation to focus exclusively on Arduino IDE
- **Renamed** `docs/SETUP.md` to `docs/HARDWARE_SETUP.md` for clarity

### Added
- **Comprehensive Arduino IDE setup guide** at `docs/ARDUINO_SETUP.md` (650+ lines)
  - Detailed prerequisites and installation instructions
  - Step-by-step board and library installation
  - Complete board configuration settings
  - Upload process and troubleshooting
  - Serial monitor usage guide
  - Extensive troubleshooting section
- **Enhanced README.md** with expanded Arduino IDE quick start
- Configuration now done via `config.h` defines instead of build flags

### Removed
- `platformio.ini` configuration file
- PlatformIO-specific `.gitignore` entries
- All PlatformIO references from documentation
- PlatformIO build instructions

### Updated Documentation
- **README.md**: Removed PlatformIO section, expanded Arduino IDE instructions
- **PROJECT_SUMMARY.md**: Updated to reflect Arduino IDE-only structure
- **CONTRIBUTING.md**: Replaced PlatformIO setup with Arduino IDE guidelines
- All file paths now reference Arduino sketch directory

### Migration Notes
For existing PlatformIO users:
- All source files remain available in `src/` and `lib/` directories for reference
- Arduino sketch in `arduino/QSYS-LED-Matrix/` contains complete working project
- WiFi credentials: Edit `arduino/QSYS-LED-Matrix/config.h`
- Matrix size: Edit `LED_MATRIX_WIDTH` and `LED_MATRIX_HEIGHT` in `config.h`
- UDP port: Edit `UDP_PORT` in `config.h`
- See `docs/ARDUINO_SETUP.md` for complete setup instructions

## [1.0.0] - 2026-02-16

### Added

#### Core Firmware
- Complete ESP32 firmware for Olimex ESP32 Gateway
- HUB75 LED matrix driver integration (64x32 default, configurable)
- DMA-based display refresh for flicker-free operation
- UDP command listener on port 21324
- Multi-segment text display system (4 segments)
- Persistent configuration storage using LittleFS
- WiFi connectivity with AP fallback mode
- Ethernet support (via Olimex Gateway hardware)

#### Text Rendering
- Multiple font support:
  - Roboto fonts (6pt, 8pt, 12pt, 16pt, 24pt)
  - Digital/LED style fonts (12pt, 24pt)
  - Monospace fonts (9pt, 12pt)
- Auto-scaling font size to fit segment
- Full RGB color support (24-bit, displayed as RGB565)
- Text alignment options (left, center, right)
- Text effects:
  - Scrolling (horizontal)
  - Blinking
  - Fade (framework ready)
  - Rainbow (framework ready)
- Border/frame support for segments

#### UDP Protocol
- TEXT command for displaying text with formatting
- CLEAR command for clearing individual segments
- CLEAR_ALL command for clearing entire display
- BRIGHTNESS command for adjusting display brightness (0-255)
- CONFIG command for system configuration
- Newline-terminated ASCII protocol
- Error handling and validation

#### Web Interface
- Embedded HTML/CSS/JS web interface
- Real-time text input and preview
- Color picker for text colors
- Font selection dropdown
- Segment-specific controls
- Brightness control slider
- Clear segment and clear all buttons
- Responsive design for mobile devices
- API endpoints:
  - `/api/config` - Get system configuration
  - `/api/segments` - Get segment status
  - `/api/test` - Test commands via HTTP

#### Q-SYS Plugin
- Complete Lua plugin for Q-SYS Designer
- 4 segment controls with individual settings
- Text input fields
- Color selection (hex input)
- Font selection dropdowns
- Send and Clear buttons per segment
- Global brightness control
- Clear All button
- Connection status indicator
- UDP socket implementation
- Configurable IP address and port
- Plugin properties for customization

#### Documentation
- Comprehensive README.md with quick start guide
- Hardware setup guide (SETUP.md):
  - Complete wiring diagram
  - Pin mapping table
  - HUB75 connector pinout
  - Power supply requirements
  - Step-by-step assembly instructions
  - Troubleshooting guide
- UDP protocol specification (UDP_PROTOCOL.md):
  - Complete command reference
  - Parameter descriptions
  - Examples for all commands
  - Testing instructions
  - Integration examples (Python, Node.js, bash)
- Q-SYS integration guide (QSYS_INTEGRATION.md):
  - Plugin installation steps
  - Configuration instructions
  - Control script examples
  - UCI integration guide
  - Best practices
- Pinout documentation (PINOUT.md):
  - Complete GPIO mapping
  - HUB75 connector details
  - Electrical characteristics
  - Level shifter information
  - Alternative pin mappings

#### Examples
- Python script (`send_text.py`):
  - Command-line UDP client
  - Full argument parsing
  - All commands supported
  - Help documentation
- Bash script (`send_command.sh`):
  - Simple shell-based client
  - Environment variable support
  - Netcat-based UDP sending
- Node.js module (`led_matrix_client.js`):
  - Promise-based API
  - Full feature support
  - Example usage included
- Examples README with use cases

#### Build System
- PlatformIO configuration (platformio.ini)
- Arduino IDE compatibility
- Automatic library dependency management
- ESP32 board configuration
- Build flags for customization
- LittleFS filesystem support

#### Project Structure
- Modular code organization:
  - `src/main.cpp` - Main firmware
  - `src/config.h` - Hardware configuration
  - `src/segment_manager.h` - Segment management
  - `src/text_renderer.h` - Text rendering engine
  - `src/udp_handler.h` - UDP protocol handler
  - `lib/fonts/fonts.h` - Font definitions
- Separate Arduino IDE version
- Example scripts directory
- Comprehensive documentation directory

### Technical Specifications

#### Performance
- Display refresh rate: 100+ Hz (flicker-free)
- UDP command latency: <50ms
- Maximum segments: 4 (configurable)
- Maximum text length: 128 characters per segment
- Color depth: 24-bit RGB (16-bit RGB565 display)
- Supported matrix sizes: 64x32 (configurable for others)

#### Networking
- WiFi 802.11 b/g/n
- Ethernet 10/100 Mbps (via Olimex hardware)
- UDP port: 21324 (configurable)
- Web server: Port 80
- Static and DHCP IP support
- Access Point fallback mode

#### Hardware
- Olimex ESP32-GATEWAY board
- HUB75 LED matrix panels (64x32 standard)
- External 5V power supply (4A+ recommended)
- GPIO pin mapping optimized for Olimex Gateway
- 3.3V logic levels (compatible with most HUB75 panels)

#### Memory
- Flash: 4MB (default partition)
- RAM: ~320KB available
- LittleFS filesystem for configuration
- Configuration persistence across reboots

### Known Limitations

- Maximum 4 segments (can be increased by modifying code)
- No authentication on UDP protocol (use trusted networks)
- Web interface has no password protection
- Maximum 128 characters per text segment
- Fade and Rainbow effects framework in place but not fully implemented
- Single matrix panel support (chaining not yet implemented)

### Compatibility

#### Hardware
- ✅ Olimex ESP32-GATEWAY (tested)
- ✅ Generic ESP32 Dev boards (with pin mapping changes)
- ✅ 64x32 HUB75 matrices (P3, P4, P5, P6 pitch)
- ⚠️ Other matrix sizes (requires configuration changes)

#### Software
- ✅ PlatformIO (VS Code) - Recommended
- ✅ Arduino IDE 1.8.x and 2.x
- ✅ Q-SYS Designer 8.0+
- ✅ Python 3.6+
- ✅ Node.js 12+
- ✅ Bash 4.0+

#### Network
- ✅ Local network UDP
- ❌ Internet routing (UDP not recommended over internet)
- ✅ VPN connections
- ✅ Direct Ethernet connection

### Security Notes

- UDP protocol has no authentication
- Web interface has no password protection
- Recommended for private/trusted networks only
- Consider using VPN for remote access
- No HTTPS support in web interface

### Future Roadmap

See [CONTRIBUTING.md](CONTRIBUTING.md) for areas accepting contributions:
- Additional text effects (fade, rainbow completion)
- More fonts and font sizes
- Custom font upload via web interface
- OTA (Over-The-Air) firmware updates
- MQTT protocol support
- Home Assistant integration
- Image and icon display
- Animation sequences
- Multi-panel chaining
- Battery backup and monitoring

## [Unreleased]

### Planned
- Fade effect implementation
- Rainbow effect implementation
- Custom font upload
- OTA updates
- MQTT support

---

## Version History

- **v1.0.0** (2026-02-16) - Initial release

## Links

- [GitHub Repository](https://github.com/DHPKE/QSYS-LED-Matrix)
- [Issue Tracker](https://github.com/DHPKE/QSYS-LED-Matrix/issues)
- [Releases](https://github.com/DHPKE/QSYS-LED-Matrix/releases)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on how to contribute.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
