# QSYS-LED-Matrix

A complete firmware and plugin solution for displaying dynamic text on a **64Ã—32 HUB75 LED matrix** controlled from **Q-SYS** via UDP. Three hardware targets are supported:

| Target | Folder | Connection |
|---|---|---|
| ESP32 Dev Module | `src/` (PlatformIO) | Wired Ethernet (LAN8720) |
| Raspberry Pi Zero 2 W | `rpi/` | Wired Ethernet via PoE HAT |
| RADXA Rock Pi S | `rockpis/` | Built-in Ethernet |

---

## ðŸŽ¯ Features

- **JSON UDP Control** â€” receive text, colours, fonts, and effects over UDP (port 21324)
- **Multi-Segment Layouts** â€” up to 4 independent text areas (fullscreen, split, quad)
- **Auto-Scaling Text** â€” automatically fits text to the available segment area
- **Text Effects** â€” scroll, blink, fade, rainbow
- **Web Interface** â€” built-in status page and test commands
- **Q-SYS Plugin** â€” drag-and-drop Lua plugin (`qsys-plugin/LEDMatrix_Complete.qplug`)
- **Full RGB Colour** â€” 24-bit hex colour codes

---

## ðŸ“¡ UDP Protocol (JSON)

Send UDP packets to port **21324**. All commands are JSON objects.

### Text command
```json
{"cmd":"text","seg":0,"text":"Hello","color":"FFFFFF","bgcolor":"000000",
 "font":"arial","size":"auto","align":"C","effect":"none","intensity":255}
```

### Layout config
```json
{"cmd":"config","seg":0,"x":0,"y":0,"w":64,"h":32}
```

### Other commands
```json
{"cmd":"clear","seg":0}
{"cmd":"clear_all"}
{"cmd":"brightness","value":200}
```

**Parameters:**
| Field | Values |
|---|---|
| `seg` | 0â€“3 |
| `color` / `bgcolor` | `RRGGBB` hex string (no `#`) |
| `font` | `arial`, `verdana`, `digital12`, `mono9` |
| `size` | `auto`, `8`, `12`, `16`, `24`, `32` |
| `align` | `L`, `C`, `R` |
| `effect` | `none`, `scroll`, `blink`, `fade`, `rainbow` |
| `intensity` | 0â€“255 |

---

## ðŸ› ï¸ Hardware Targets

### ESP32 Dev Module (`src/`)

**Requirements:**
- ESP32 Dev Module board
- 64Ã—32 HUB75 LED matrix panel
- External 5V / 4A+ power supply for matrix

**Build & Flash (PlatformIO):**
```bash
pio run --target upload --upload-port COM4
```
Or use the VS Code PlatformIO extension.

**HUB75 Pinout** â€” see [`docs/PINOUT.md`](docs/PINOUT.md) for full wiring.

---

### Raspberry Pi Zero 2 W (`rpi/`)

**Requirements:**
- Raspberry Pi Zero 2 W
- PoE HAT (802.3af) for wired Ethernet and power
- 64Ã—32 HUB75 LED matrix panel

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

**HUB75 Pinout** â€” BCM GPIO "regular" mapping:

| Signal | BCM GPIO | Physical Pin |
|---|---|---|
| R1 | 5 | 29 |
| G1 | 13 | 33 |
| B1 | 6 | 31 |
| R2 | 12 | 32 |
| G2 | 16 | 36 |
| B2 | 23 | 16 |
| A  | 22 | 15 |
| B  | 26 | 37 |
| C  | 27 | 13 |
| D  | 20 | 38 |
| CLK | 17 | 11 |
| LAT | 4  | 7  |
| OE  | 18 | 12 |

---

### RADXA Rock Pi S (`rockpis/`)

**Requirements:**
- RADXA Rock Pi S (RK3308)
- 64Ã—32 HUB75 LED matrix panel
- Armbian OS (Bookworm / Jammy recommended)

**Install:**
```bash
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/rockpis
bash install.sh
```

The install script patches the `rpi-rgb-led-matrix` hardware mapping for the RK3308 GPIO layout and disables the UART0 serial console (those pins are needed for HUB75 address lines).

See [`rockpis/README.md`](rockpis/README.md) for full wiring and GPIO details.

---

## ðŸŽ›ï¸ Q-SYS Plugin

File: `qsys-plugin/LEDMatrix_Complete.qplug`

**Install in Q-SYS Designer:**
1. Copy the `.qplug` file to your Q-SYS plugin folder
2. Drag the plugin from the Plugins library onto your schematic
3. Set the **IP Address** and **UDP Port** (21324) in the plugin properties
4. Use the **Apply Layout** button to set the segment layout before sending text

**Layouts available:** Fullscreen, Split Vertical, Split Horizontal, Quad

---

## ðŸ“ Project Structure

```
QSYS-LED-Matrix/
â”œâ”€â”€ src/                    # ESP32 firmware (PlatformIO)
â”‚   â”œâ”€â”€ main.cpp
â”‚   â”œâ”€â”€ config.h
â”‚   â”œâ”€â”€ segment_manager.h
â”‚   â”œâ”€â”€ text_renderer.h
â”‚   â””â”€â”€ udp_handler.h
â”œâ”€â”€ rpi/                    # Raspberry Pi Zero 2 W port
â”‚   â”œâ”€â”€ main.py
â”‚   â”œâ”€â”€ config.py
â”‚   â”œâ”€â”€ segment_manager.py
â”‚   â”œâ”€â”€ text_renderer.py
â”‚   â”œâ”€â”€ udp_handler.py
â”‚   â”œâ”€â”€ web_server.py
â”‚   â”œâ”€â”€ led-matrix.service
â”‚   â””â”€â”€ install.sh
â”œâ”€â”€ rockpis/                # RADXA Rock Pi S port
â”‚   â”œâ”€â”€ main.py
â”‚   â”œâ”€â”€ config.py
â”‚   â”œâ”€â”€ segment_manager.py
â”‚   â”œâ”€â”€ text_renderer.py
â”‚   â”œâ”€â”€ udp_handler.py
â”‚   â”œâ”€â”€ web_server.py
â”‚   â”œâ”€â”€ led-matrix.service
â”‚   â”œâ”€â”€ install.sh
â”‚   â””â”€â”€ README.md
â”œâ”€â”€ qsys-plugin/
â”‚   â””â”€â”€ LEDMatrix_Complete.qplug
â”œâ”€â”€ docs/
â”‚   â”œâ”€â”€ PINOUT.md
â”‚   â”œâ”€â”€ UDP_PROTOCOL.md
â”‚   â”œâ”€â”€ QSYS_INTEGRATION.md
â”‚   â””â”€â”€ HARDWARE_SETUP.md
â”œâ”€â”€ examples/               # Test scripts (Python, Node.js, bash)
â”œâ”€â”€ archive/                # Superseded docs
â”œâ”€â”€ platformio.ini
â””â”€â”€ README.md
```

---

## ðŸ› Troubleshooting

### Matrix doesn't light up
- Verify external 5V power supply is connected (never power from the controller board)
- Check HUB75 ribbon cable orientation
- Confirm GPIO pin assignments in `config.h` / `config.py`

### UDP commands not received
- Check firewall on the host / Q-SYS core
- Verify IP address and port 21324
- Watch logs: `sudo journalctl -u led-matrix -f` (Pi/Rock Pi S)
- Watch serial output (ESP32): `pio device monitor --port COM4 --baud 115200`

### RPi service crash-loop (`snd_bcm2835` error)
- Run `sudo bash install.sh` to apply the blacklist, or manually:
  ```bash
  echo "blacklist snd_bcm2835" | sudo tee /etc/modprobe.d/blacklist-rgb-matrix.conf
  sudo reboot
  ```

### Text on wrong segment / ghost segments after layout change
- Always press **Apply Layout** in the Q-SYS plugin before sending text
- The plugin only auto-sends to segments that belong to the active layout

---

## ðŸ“ License

See [LICENSE](LICENSE) for details.

## ðŸ“ž Support

- GitHub Issues: [github.com/DHPKE/QSYS-LED-Matrix/issues](https://github.com/DHPKE/QSYS-LED-Matrix/issues)
