# QSYS-LED-Matrix

A complete firmware and plugin solution for displaying dynamic text on a **64×32 HUB75 LED matrix** controlled from **Q-SYS** via UDP. Three hardware targets are supported:

| Target | Folder | Language | Network |
|---|---|---|---|
| WT32-ETH01 (ESP32) | `src/` | C++ / Arduino (PlatformIO) | Wired Ethernet (LAN8720 PHY) |
| Raspberry Pi Zero 2 W | `rpi/` | Python 3 | Wired Ethernet via PoE HAT |
| RADXA Rock Pi S | `rockpis/` | Python 3 | Built-in Ethernet (RK3308) |

---

## 🎯 Features

- **JSON UDP Control** — receive text, colours, fonts, and effects over UDP (port 21324)
- **Multi-Segment Layouts** — up to 4 independent text areas with 6 layout presets
- **Auto-Scaling Text** — automatically fits text to the available segment area
- **Text Effects** — scroll, blink, fade, rainbow
- **Web Interface** — built-in status page and test commands
- **Q-SYS Plugin** — drag-and-drop Lua plugin with layout presets and per-segment controls
- **Full RGB Colour** — 24-bit hex colour codes or 14 named colours
- **Persistent Config** — settings survive reboots (LittleFS on ESP32, JSON on Pi)
- **IP Splash Screen** — displays device IP on boot until first command received
- **Fallback Static IP** — configurable fallback when DHCP fails (WT32-ETH01)

---

## 📡 UDP Protocol (JSON)

Send UDP packets to port **21324**. All commands are JSON objects.

### Text command
```json
{"cmd":"text","seg":0,"text":"Hello World","color":"FFFFFF","bgcolor":"000000",
 "font":"arial","size":"auto","align":"C","effect":"none","intensity":255}
```

Or use integer IDs for more compact commands:
```json
{"cmd":"text","seg":0,"text":"ONLINE","color":1,"bgcolor":14,"font":1,"effect":0}
```

### Layout presets
```json
{"cmd":"layout","preset":1}
```

**Available presets:**
- `1` = Fullscreen (seg0: 64×32)
- `2` = Split Horizontal (seg0: top 64×16, seg1: bottom 64×16)
- `3` = Split Vertical (seg0: left 32×32, seg1: right 32×32)
- `4` = Quad (2×2 grid, 32×16 each)
- `5` = Thirds (3 columns approx. 21×32 each)
- `6` = Triple (seg0: left 32×32, seg1/2: right quarters 32×16)
- `11-14` = Single segment fullscreen (activates only seg 0-3)

### Other commands
```json
{"cmd":"clear","seg":0}
{"cmd":"clear_all"}
{"cmd":"brightness","value":200}
{"cmd":"config","seg":0,"x":0,"y":0,"w":64,"h":32}
```

### Parameter reference

| Field | Valid values |
|---|---|
| `seg` | 0–3 |
| `color` / `bgcolor` | Hex: `RRGGBB` (no `#`) OR Integer: 1–14 |
| `font` | String: `arial`, `verdana`, `impact` OR Integer: 1–3 |
| `size` | `auto`, `8`, `12`, `16`, `24`, `32` |
| `align` | `L` (left), `C` (center), `R` (right) |
| `effect` | String: `none`, `scroll`, `blink`, `fade` OR Integer: 0–3 |
| `intensity` | 0–255 |

**Integer Enums:**

| Colors (1–14) | | | |
|---|---|---|---|
| 1=White | 2=Red | 3=Lime | 4=Blue |
| 5=Yellow | 6=Magenta | 7=Cyan | 8=Orange |
| 9=Purple | 10=Gold | 11=Grey | 12=Black |
| 13=Grey | 14=Black | | |

| Fonts (1–3) | Effects (0–3) |
|---|---|
| 1=Arial (Bold) | 0=None |
| 2=Verdana | 1=Scroll |
| 3=Impact | 2=Blink |
| | 3=Fade |

---

## 🛠️ Hardware Setup

## WT32-ETH01 (ESP32) — `src/`

> **⚠️ IMPORTANT - Library Compatibility Issue**  
> The current library versions (AsyncTCP/ESPAsyncWebServer) are not compatible with Arduino-ESP32 framework 2.0+/3.0+. Until this is resolved:
> - **Workaround**: Use `platform = espressif32@3.5.0` in platformio.ini (Arduino framework 1.0.6)
> - **Alternative**: Use the Raspberry Pi or Rock Pi S implementations which work perfectly
> - Issue tracked: [GitHub Issue Link TBD]

**Requirements:**
- WT32-ETH01 board (ESP32 + LAN8720 Ethernet PHY)
- 64×32 HUB75 LED matrix panel
- External 5V / 4A+ power supply for matrix

**Build & Flash (PlatformIO):**
```bash
pio run --target upload --upload-port COM3
```
Or use the VS Code PlatformIO extension.

**Monitor serial output:**
```bash
pio device monitor --port COM3 --baud 115200
```

**Pin Assignment** — See [docs/PINOUT.md](docs/PINOUT.md) for full details.

**Key configuration** ([src/config.h](src/config.h)):
```cpp
#define FALLBACK_IP      "10.10.10.99"   // Used when DHCP times out (15s)
#define FALLBACK_GW      "10.10.10.1"
#define FALLBACK_SUBNET  "255.255.255.0"
```

**Web UI:** `http://<IP>/` (port 80)

---

### Raspberry Pi Zero 2 W — `rpi/`

**Requirements:**
- Raspberry Pi Zero 2 W
- PoE HAT (802.3af) for wired Ethernet and power
- 64×32 HUB75 LED matrix panel
- Raspberry Pi OS Bookworm (64-bit recommended)

**Install:**
```bash
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/rpi
bash install.sh
```

The install script:
1. Blacklists `snd_bcm2835` (conflicts with LED matrix PWM)
2. Installs all dependencies
3. Clones and builds `rpi-rgb-led-matrix` from source
4. Copies app to `/opt/led-matrix`
5. Installs and starts the `led-matrix` systemd service

**Check service status:**
```bash
sudo systemctl status led-matrix
sudo journalctl -u led-matrix -f
```

**HUB75 Pinout** — BCM GPIO "regular" mapping:

| Signal | BCM GPIO | Physical Pin |
|---|---|---|
| R1  | 5  | 29 |
| G1  | 13 | 33 |
| B1  | 6  | 31 |
| R2  | 12 | 32 |
| G2  | 16 | 36 |
| B2  | 23 | 16 |
| A   | 22 | 15 |
| B   | 26 | 37 |
| C   | 27 | 13 |
| D   | 20 | 38 |
| CLK | 17 | 11 |
| LAT | 4  | 7  |
| OE  | 18 | 12 |

> **PoE HAT fan warning:** Some PoE HATs use GPIO 4 (LAT) or GPIO 26 (B) for their cooling fan. Check your HAT datasheet and adjust the wiring / `config.py` if there is a conflict.

**Web UI:** `http://<Pi-IP>:8080/`

---

### RADXA Rock Pi S — `rockpis/`

**Requirements:**
- RADXA Rock Pi S (RK3308 SoC)
- 64×32 HUB75 LED matrix panel
- Armbian OS (Bookworm / Jammy recommended)

**Install:**
```bash
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/rockpis
bash install.sh
```

The install script patches the `rpi-rgb-led-matrix` hardware mapping for the RK3308 GPIO layout and disables the UART0 serial console (those pins are needed for HUB75 address lines).

See [rockpis/README.md](rockpis/README.md) for full wiring and GPIO details.

**Web UI:** `http://<Rock-Pi-IP>:8080/`

---

## 🎛️ Q-SYS Plugin

**Current versions:**
- `WT32_LEDMatrix_v4.qplug` — Latest version with integer protocol support
- `LEDMatrix_v3.qplug` — Alternative version
- `LEDMatrix_Complete.qplug` — Legacy version

**Install in Q-SYS Designer:**
1. Copy the `.qplug` file to your Q-SYS plugin folder  
   (typically `%USERPROFILE%\Documents\QSC\Q-SYS Designer\Plugins`)
2. Drag the plugin from the Plugins library onto your schematic
3. Set the **IP Address** and **UDP Port** (21324) in the plugin properties
4. Use the **Layout** dropdown to select a preset (Fullscreen, Split, Quad, etc.)
5. Type text into segment fields — changes auto-send after 400ms

**Plugin controls per segment:**
- Text content
- Text Color (ComboBox: 1–White … 14–Black)
- Background Color
- Font (ComboBox: 1–Arial … 3–Impact)
- Alignment (L / C / R)
- Effect (0–None … 3–Fade)
- Display button (manual send)
- Clear button (deactivate segment)
- Invert button (swap color ↔ bgcolor)

**Global controls:**
- Layout preset selector
- Brightness (0-255)
- Clear All button
- Connection status indicator

---

## 📁 Project Structure

```
QSYS-LED-Matrix/
├── src/                    # WT32-ETH01 firmware (PlatformIO)
│   ├── main.cpp
│   ├── config.h            # Pin assignments, network config
│   ├── segment_manager.h   # Segment state management
│   ├── text_renderer.h     # HUB75 text rendering
│   ├── udp_handler.h       # JSON UDP command dispatch
│   └── fonts.h             # Embedded font data
├── rpi/                    # Raspberry Pi Zero 2 W port (Python)
│   ├── main.py
│   ├── config.py
│   ├── segment_manager.py
│   ├── text_renderer.py
│   ├── udp_handler.py
│   ├── web_server.py
│   ├── led-matrix.service
│   └── install.sh
├── rockpis/                # RADXA Rock Pi S port (Python)
│   ├── main.py
│   ├── config.py
│   ├── segment_manager.py
│   ├── text_renderer.py
│   ├── udp_handler.py
│   ├── web_server.py
│   ├── led-matrix.service
│   ├── install.sh
│   └── README.md
├── qsys-plugin/
│   ├── WT32_LEDMatrix_v4.qplug      # Latest recommended
│   ├── LEDMatrix_v3.qplug
│   └── LEDMatrix_Complete.qplug     # Legacy
├── docs/
│   ├── PINOUT.md
│   ├── UDP_PROTOCOL.md
│   ├── QSYS_INTEGRATION.md
│   └── HARDWARE_SETUP.md
├── examples/               # Test scripts (Python, Node.js, bash)
├── archive/                # Superseded documentation
├── platformio.ini
└── README.md
```

---

## 🔧 Troubleshooting

### ESP32 firmware won't compile (AsyncTCP errors)
**Issue**: Compilation fails with IPAddress conversion errors in AsyncTCP/ESPAsyncWebServer libraries.

**Cause**: Current AsyncTCP library versions are incompatible with Arduino-ESP32 framework 2.0+ and 3.0+.

**Solutions**:
1. **Use older framework** (temporary workaround):
   ```ini
   # In platformio.ini
   platform = espressif32@3.5.0
   ```
   
2. **Use Raspberry Pi or Rock Pi S implementations** - These work perfectly and don't have this issue

3. **Wait for library updates** - Monitor https://github.com/esp32async/AsyncTCP for compatibility updates

### Matrix doesn't light up
- Verify external 5V power supply is connected (never power from the controller board)
- Check HUB75 ribbon cable orientation (pin 1 marking)
- Confirm GPIO pin assignments in `config.h` (ESP32) or `config.py` (Pi/Rock Pi S)
- ESP32: Remove `-DNO_DISPLAY` flag from `platformio.ini` build_flags

### UDP commands not received
- Check firewall on the host network / Q-SYS core
- Verify IP address and port 21324
- Watch logs:
  - ESP32: `pio device monitor --port COM3 --baud 115200`
  - Pi/Rock Pi S: `sudo journalctl -u led-matrix -f`

### RPi service crash-loop (`snd_bcm2835` error)
The on-board audio driver conflicts with the LED matrix PWM hardware. The `install.sh` script blacklists it automatically. To fix manually:
```bash
echo "blacklist snd_bcm2835" | sudo tee /etc/modprobe.d/blacklist-rgb-matrix.conf
sudo reboot
```

### Text on wrong segment / ghost segments after layout change
- Always apply the layout preset before sending text
- The Q-SYS plugin auto-sends layout changes when you select a new preset from the dropdown
- Previous segment data is cleared when layout changes

### Web UI not reachable
- WT32-ETH01: `http://<IP>/` (port 80)
- Pi / Rock Pi S: `http://<IP>:8080/` (port 8080)
- Check if device obtained IP (ESP32: watch serial output, Pi: `ip addr`)
- WT32-ETH01: If DHCP fails, device uses fallback IP (default: 10.10.10.99)

### IP address not displayed on boot (WT32-ETH01)
- IP splash displays for 15 seconds or until first UDP command
- Check if NO_DISPLAY flag is set in platformio.ini (remove it once panel is connected)
- Verify matrix is powered and HUB75 cable is connected

---

## 🚀 Quick Start Example

1. Flash WT32-ETH01 firmware:
   ```bash
   pio run --target upload --upload-port COM3
   ```

2. Connect to device serial to see IP address:
   ```bash
   pio device monitor --port COM3 --baud 115200
   ```

3. Install Q-SYS plugin (copy `.qplug` to plugins folder)

4. In Q-SYS Designer:
   - Add plugin to schematic
   - Set IP address from serial output
   - Select "1 – Fullscreen" layout
   - Type "HELLO WORLD" in Segment 1 text field
   - Auto-send triggers after 400ms

5. Test from command line:
   ```bash
   echo '{"cmd":"text","seg":0,"text":"TEST","color":1,"bgcolor":14}' | nc -u <IP> 21324
   ```

---

## 📜 License

See [LICENSE](LICENSE) for details.

## 📞 Support

- GitHub Issues: [github.com/DHPKE/QSYS-LED-Matrix/issues](https://github.com/DHPKE/QSYS-LED-Matrix/issues)
- Documentation: See `/docs` folder for detailed guides

---

**Version:** 2.0.0  
**Last Updated:** 2026-02-20
