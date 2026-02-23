# QSYS-LED-Matrix# QSYS-LED-Matrix



Control a **64x32 HUB75 LED matrix** from **Q-SYS** via JSON UDP commands.  A complete firmware and plugin solution for displaying dynamic text on a **64Ã—32 HUB75 LED matrix** controlled from **Q-SYS** via UDP. Three hardware targets are supported:

Three hardware targets are supported — pick the one that fits your installation.

| Target | Folder | Connection |

| Target | Folder | Language | Network ||---|---|---|

| --- | --- | --- | --- || ESP32 Dev Module | `src/` (PlatformIO) | Wired Ethernet (LAN8720) |

| ESP32 Dev Module | `src/` | C++ / Arduino (PlatformIO) | Wired Ethernet (LAN8720 PHY) || Raspberry Pi Zero 2 W | `rpi/` | Wired Ethernet via PoE HAT |

| Raspberry Pi Zero 2 W | `rpi/` | Python 3 | Wired Ethernet via PoE HAT || RADXA Rock Pi S | `rockpis/` | Built-in Ethernet |

| RADXA Rock Pi S | `rockpis/` | Python 3 | Built-in Ethernet (RK3308) |

---

> **Note on the ESP32 target:** The firmware is built with PlatformIO for a generic

> ESP32 Dev Module (not the Olimex Gateway). The Olimex Gateway's Ethernet PHY## ðŸŽ¯ Features

> occupies most of the available GPIOs, leaving too few free pins for HUB75.

> A plain ESP32 Dev Module with a separate LAN8720 breakout avoids this conflict.- **JSON UDP Control** â€” receive text, colours, fonts, and effects over UDP (port 21324)

- **Multi-Segment Layouts** â€” up to 4 independent text areas (fullscreen, split, quad)

---- **Auto-Scaling Text** â€” automatically fits text to the available segment area

- **Text Effects** â€” scroll, blink, fade, rainbow

## Features- **Web Interface** â€” built-in status page and test commands

- **Q-SYS Plugin** â€” drag-and-drop Lua plugin (`qsys-plugin/LEDMatrix_Complete.qplug`)

- JSON UDP control (port 21324) -- text, colour, font, size, alignment, effects- **Full RGB Colour** â€” 24-bit hex colour codes

- Up to 4 independent segments per display (fullscreen / split vertical / split horizontal / quad)

- Auto-scaling text -- fits any string into its segment automatically---

- Text effects: scroll, blink, fade, rainbow

- Built-in web interface for status and manual testing## ðŸ“¡ UDP Protocol (JSON)

- Q-SYS plugin (`LEDMatrix_Complete.qplug`) with layout presets and per-segment controls

- Persistent config -- segments and brightness survive reboots (LittleFS on ESP32, JSON file on Pi)Send UDP packets to port **21324**. All commands are JSON objects.



---### Text command

```json

## UDP Protocol{"cmd":"text","seg":0,"text":"Hello","color":"FFFFFF","bgcolor":"000000",

 "font":"arial","size":"auto","align":"C","effect":"none","intensity":255}

All commands are sent as JSON datagrams to **UDP port 21324**.```



### Send text to a segment### Layout config

```json

```json{"cmd":"config","seg":0,"x":0,"y":0,"w":64,"h":32}

{```

  "cmd":       "text",

  "seg":       0,### Other commands

  "text":      "Hello World",```json

  "color":     "FFFFFF",{"cmd":"clear","seg":0}

  "bgcolor":   "000000",{"cmd":"clear_all"}

  "font":      "arial",{"cmd":"brightness","value":200}

  "size":      "auto",```

  "align":     "C",

  "effect":    "none",**Parameters:**

  "intensity": 255| Field | Values |

}|---|---|

```| `seg` | 0â€“3 |

| `color` / `bgcolor` | `RRGGBB` hex string (no `#`) |

### Configure a segment geometry (layout change)| `font` | `arial`, `verdana`, `digital12`, `mono9` |

| `size` | `auto`, `8`, `12`, `16`, `24`, `32` |

```json| `align` | `L`, `C`, `R` |

{ "cmd": "config", "seg": 0, "x": 0, "y": 0, "w": 64, "h": 32 }| `effect` | `none`, `scroll`, `blink`, `fade`, `rainbow` |

```| `intensity` | 0â€“255 |



### Other commands---



```json## ðŸ› ï¸ Hardware Targets

{ "cmd": "clear",      "seg": 0  }

{ "cmd": "clear_all"              }### ESP32 Dev Module (`src/`)

{ "cmd": "brightness", "value": 200 }

```**Requirements:**

- ESP32 Dev Module board

### Parameter reference- 64Ã—32 HUB75 LED matrix panel

- External 5V / 4A+ power supply for matrix

| Field | Valid values |

| --- | --- |**Build & Flash (PlatformIO):**

| `seg` | 0 - 3 |```bash

| `color` / `bgcolor` | `RRGGBB` hex string, no `#` (e.g. `FF0000`) |pio run --target upload --upload-port COM4

| `font` | `arial`, `verdana`, `digital12`, `mono9` |```

| `size` | `auto`, `8`, `12`, `16`, `24`, `32` |Or use the VS Code PlatformIO extension.

| `align` | `L` (left), `C` (center), `R` (right) |

| `effect` | `none`, `scroll`, `blink`, `fade`, `rainbow` |**HUB75 Pinout** â€” see [`docs/PINOUT.md`](docs/PINOUT.md) for full wiring.

| `intensity` | 0 - 255 |

---

### Important: always apply the layout before sending text

### Raspberry Pi Zero 2 W (`rpi/`)

The firmware/software only renders a segment when it is **active**.  

A segment becomes active on receipt of a `text` command and inactive on `clear`.  **Requirements:**

A `config` command updates geometry only -- it does not activate the segment.  - Raspberry Pi Zero 2 W

This means stale segments from a previous layout are automatically hidden when the- PoE HAT (802.3af) for wired Ethernet and power

layout changes, as long as the Q-SYS plugin sends `clear` for unused segments.- 64Ã—32 HUB75 LED matrix panel



---**Install:**

```bash

## Hardware Targetsgit clone https://github.com/DHPKE/QSYS-LED-Matrix.git

cd QSYS-LED-Matrix/rpi

### 1. ESP32 Dev Module (`src/`)bash install.sh

```

**What you need**

The install script:

- ESP32 Dev Module (38-pin or 30-pin)1. Blacklists `snd_bcm2835` (conflicts with LED matrix PWM)

- LAN8720 Ethernet breakout (wired Ethernet) *or* WiFi (configure SSID in `config.h`)2. Installs all dependencies

- 64x32 HUB75 LED matrix panel3. Clones and builds `rpi-rgb-led-matrix` from source

- External 5V / 4A+ PSU for the matrix (never power from the ESP32 pin)4. Copies app to `/opt/led-matrix`

5. Installs and starts the `led-matrix` systemd service

**ESP32 HUB75 pin assignment** (`src/config.h`)

**HUB75 Pinout** â€” BCM GPIO "regular" mapping:

| HUB75 signal | ESP32 GPIO |

| --- | --- || Signal | BCM GPIO | Physical Pin |

| R1 | GPIO 2 ||---|---|---|

| G1 | GPIO 15 || R1 | 5 | 29 |

| B1 | GPIO 4 || G1 | 13 | 33 |

| R2 | GPIO 16 || B1 | 6 | 31 |

| G2 | GPIO 12 || R2 | 12 | 32 |

| B2 | GPIO 14 || G2 | 16 | 36 |

| A  | GPIO 33 || B2 | 23 | 16 |

| B  | GPIO 13 || A  | 22 | 15 |

| LAT | GPIO 32 || B  | 26 | 37 |

| C, D, OE, CLK | see `config.h` || C  | 27 | 13 |

| D  | 20 | 38 |

**Build and flash (PlatformIO)**| CLK | 17 | 11 |

| LAT | 4  | 7  |

```bash| OE  | 18 | 12 |

# From the repository root:

pio run --target upload --upload-port COM4---

```

### RADXA Rock Pi S (`rockpis/`)

Or use the VS Code PlatformIO extension (click the upload arrow in the status bar).

**Requirements:**

**Monitor serial output**- RADXA Rock Pi S (RK3308)

- 64Ã—32 HUB75 LED matrix panel

```bash- Armbian OS (Bookworm / Jammy recommended)

pio device monitor --port COM4 --baud 115200

```**Install:**

```bash

The IP address is printed to serial on boot. Access the web UI at `http://<IP>/`.git clone https://github.com/DHPKE/QSYS-LED-Matrix.git

cd QSYS-LED-Matrix/rockpis

---bash install.sh

```

### 2. Raspberry Pi Zero 2 W (`rpi/`)

The install script patches the `rpi-rgb-led-matrix` hardware mapping for the RK3308 GPIO layout and disables the UART0 serial console (those pins are needed for HUB75 address lines).

**What you need**

See [`rockpis/README.md`](rockpis/README.md) for full wiring and GPIO details.

- Raspberry Pi Zero 2 W

- PoE HAT (802.3af/at) -- provides both wired Ethernet and 5V power---

- 64x32 HUB75 LED matrix panel

- Raspberry Pi OS Bookworm (64-bit recommended)## ðŸŽ›ï¸ Q-SYS Plugin



**HUB75 wiring** -- BCM GPIO "regular" mapping (rpi-rgb-led-matrix defaults)File: `qsys-plugin/LEDMatrix_Complete.qplug`



| HUB75 signal | BCM GPIO | Physical pin |**Install in Q-SYS Designer:**

| --- | --- | --- |1. Copy the `.qplug` file to your Q-SYS plugin folder

| R1  | 5  | 29 |2. Drag the plugin from the Plugins library onto your schematic

| G1  | 13 | 33 |3. Set the **IP Address** and **UDP Port** (21324) in the plugin properties

| B1  | 6  | 31 |4. Use the **Apply Layout** button to set the segment layout before sending text

| R2  | 12 | 32 |

| G2  | 16 | 36 |**Layouts available:** Fullscreen, Split Vertical, Split Horizontal, Quad

| B2  | 23 | 16 |

| A   | 22 | 15 |---

| B   | 26 | 37 |

| C   | 27 | 13 |## ðŸ“ Project Structure

| D   | 20 | 38 |

| CLK | 17 | 11 |```

| LAT | 4  | 7  |QSYS-LED-Matrix/

| OE  | 18 | 12 |â”œâ”€â”€ src/                    # ESP32 firmware (PlatformIO)

| GND | -- | 6 / 9 / 14 / 20 / 25 |â”‚   â”œâ”€â”€ main.cpp

â”‚   â”œâ”€â”€ config.h

> **PoE HAT fan warning:** Some PoE HATs use GPIO 4 (LAT) or GPIO 26 (B) forâ”‚   â”œâ”€â”€ segment_manager.h

> their cooling fan. Check your HAT datasheet and adjust the wiring / `config.py`â”‚   â”œâ”€â”€ text_renderer.h

> if there is a conflict.â”‚   â””â”€â”€ udp_handler.h

â”œâ”€â”€ rpi/                    # Raspberry Pi Zero 2 W port

**Install**â”‚   â”œâ”€â”€ main.py

â”‚   â”œâ”€â”€ config.py

```bashâ”‚   â”œâ”€â”€ segment_manager.py

git clone https://github.com/DHPKE/QSYS-LED-Matrix.gitâ”‚   â”œâ”€â”€ text_renderer.py

cd QSYS-LED-Matrix/rpiâ”‚   â”œâ”€â”€ udp_handler.py

bash install.shâ”‚   â”œâ”€â”€ web_server.py

```â”‚   â”œâ”€â”€ led-matrix.service

â”‚   â””â”€â”€ install.sh

The script will:â”œâ”€â”€ rockpis/                # RADXA Rock Pi S port

â”‚   â”œâ”€â”€ main.py

1. Blacklist `snd_bcm2835` (on-board audio conflicts with the LED matrix PWM hardware)â”‚   â”œâ”€â”€ config.py

2. Install system packages and build toolsâ”‚   â”œâ”€â”€ segment_manager.py

3. Clone and build `rpi-rgb-led-matrix` from sourceâ”‚   â”œâ”€â”€ text_renderer.py

4. Install the Python bindingsâ”‚   â”œâ”€â”€ udp_handler.py

5. Copy app files to `/opt/led-matrix/`â”‚   â”œâ”€â”€ web_server.py

6. Install and start the `led-matrix` systemd serviceâ”‚   â”œâ”€â”€ led-matrix.service

â”‚   â”œâ”€â”€ install.sh

**Check service status**â”‚   â””â”€â”€ README.md

â”œâ”€â”€ qsys-plugin/

```bashâ”‚   â””â”€â”€ LEDMatrix_Complete.qplug

sudo systemctl status led-matrixâ”œâ”€â”€ docs/

sudo journalctl -u led-matrix -fâ”‚   â”œâ”€â”€ PINOUT.md

```â”‚   â”œâ”€â”€ UDP_PROTOCOL.md

â”‚   â”œâ”€â”€ QSYS_INTEGRATION.md

Web UI is available at `http://<Pi-IP>:8080/`.â”‚   â””â”€â”€ HARDWARE_SETUP.md

â”œâ”€â”€ examples/               # Test scripts (Python, Node.js, bash)

**Key config options** (`rpi/config.py`)â”œâ”€â”€ archive/                # Superseded docs

â”œâ”€â”€ platformio.ini

| Setting | Default | Description |â””â”€â”€ README.md

| --- | --- | --- |```

| `MATRIX_WIDTH` | 64 | Panel width in pixels |

| `MATRIX_HEIGHT` | 32 | Panel height in pixels |---

| `MATRIX_GPIO_SLOWDOWN` | 1 | 0 = fastest, 4 = slowest/most stable |

| `MATRIX_BRIGHTNESS` | 50 | 0-100% |## ðŸ› Troubleshooting

| `MATRIX_PWM_BITS` | 8 | 1-11; higher = more colour depth |

| `UDP_PORT` | 21324 | UDP listen port |### Matrix doesn't light up

| `WEB_PORT` | 8080 | Web UI port |- Verify external 5V power supply is connected (never power from the controller board)

- Check HUB75 ribbon cable orientation

---- Confirm GPIO pin assignments in `config.h` / `config.py`



### 3. RADXA Rock Pi S (`rockpis/`)### UDP commands not received

- Check firewall on the host / Q-SYS core

**What you need**- Verify IP address and port 21324

- Watch logs: `sudo journalctl -u led-matrix -f` (Pi/Rock Pi S)

- RADXA Rock Pi S (RK3308 SoC, built-in 100M Ethernet)- Watch serial output (ESP32): `pio device monitor --port COM4 --baud 115200`

- 64x32 HUB75 LED matrix panel

- Armbian OS (Bookworm or Jammy)### RPi service crash-loop (`snd_bcm2835` error)

- Run `sudo bash install.sh` to apply the blacklist, or manually:

**Install**  ```bash

  echo "blacklist snd_bcm2835" | sudo tee /etc/modprobe.d/blacklist-rgb-matrix.conf

```bash  sudo reboot

git clone https://github.com/DHPKE/QSYS-LED-Matrix.git  ```

cd QSYS-LED-Matrix/rockpis

bash install.sh### Text on wrong segment / ghost segments after layout change

```- Always press **Apply Layout** in the Q-SYS plugin before sending text

- The plugin only auto-sends to segments that belong to the active layout

The script will:

---

1. Detect hardware and Armbian version

2. Install system packages and build tools## ðŸ“ License

3. Clone `rpi-rgb-led-matrix` and patch in the Rock Pi S GPIO hardware mapping

4. Build the library and Python bindingsSee [LICENSE](LICENSE) for details.

5. Disable the UART0 serial console (pins 8 and 10 are needed for HUB75 address lines)

6. Copy app files to `/opt/led-matrix/`## ðŸ“ž Support

7. Install and start the `led-matrix` systemd service

- GitHub Issues: [github.com/DHPKE/QSYS-LED-Matrix/issues](https://github.com/DHPKE/QSYS-LED-Matrix/issues)

See [`rockpis/README.md`](rockpis/README.md) for the full GPIO wiring table and
RK3308 GPIO numbering details.

---

## Q-SYS Plugin

File: `qsys-plugin/LEDMatrix_Complete.qplug`

**Install in Q-SYS Designer**

1. Copy `LEDMatrix_Complete.qplug` to your Q-SYS Plugins folder
   (typically `%USERPROFILE%\Documents\QSC\Q-SYS Designer\Plugins`)
2. In Q-SYS Designer, drag the **LED Matrix Complete** block onto your schematic
3. Set **IP Address** and **UDP Port** (21324) in the component properties
4. In the Q-SYS UCI / control page:
   - Select a **Layout Preset** (Fullscreen / Split Vertical / Split Horizontal / Quad)
   - Press **Apply Layout** -- this configures segment geometry and clears inactive segments
   - Type text into the segment **Text** fields; press **Display** or let auto-send fire

**Plugin controls per segment**

| Control | Description |
| --- | --- |
| Text | Content to display |
| Text Color | Foreground colour (named colour list) |
| BG Color | Background colour |
| Font | arial / verdana / digital / mono |
| Size | auto or fixed pixel size |
| Align | Left / Center / Right |
| Effect | none / scroll / blink / fade / rainbow |
| Intensity | 0-255 brightness multiplier |
| Display | Send the text command immediately |
| Clear | Deactivate this segment |

**Global controls:** Brightness fader, Clear All button, Connection Status indicator.

---

## Project Structure

```
QSYS-LED-Matrix/
|
+-- src/                    ESP32 firmware (C++, PlatformIO)
|   +-- main.cpp
|   +-- config.h            Pin assignments, network config
|   +-- segment_manager.h   Segment state management
|   +-- text_renderer.h     HUB75 text rendering
|   +-- udp_handler.h       JSON UDP command dispatch
|   +-- fonts.h             Embedded font data
|
+-- rpi/                    Raspberry Pi Zero 2 W port (Python 3)
|   +-- main.py
|   +-- config.py           All hardware/network constants
|   +-- segment_manager.py
|   +-- text_renderer.py
|   +-- udp_handler.py
|   +-- web_server.py
|   +-- led-matrix.service  systemd unit
|   +-- install.sh          One-shot install script
|
+-- rockpis/                RADXA Rock Pi S port (Python 3)
|   +-- main.py
|   +-- config.py
|   +-- segment_manager.py
|   +-- text_renderer.py
|   +-- udp_handler.py
|   +-- web_server.py
|   +-- led-matrix.service
|   +-- install.sh
|   +-- README.md           Rock Pi S specific wiring + GPIO table
|
+-- qsys-plugin/
|   +-- LEDMatrix_Complete.qplug    Main plugin (use this)
|   +-- led_matrix_controller.lua   Legacy standalone Lua script
|
+-- docs/
|   +-- PINOUT.md
|   +-- UDP_PROTOCOL.md
|   +-- HARDWARE_SETUP.md
|   +-- QSYS_INTEGRATION.md
|
+-- examples/               Test scripts (Python, Node.js, bash)
+-- arduino/                Legacy Arduino IDE sketch (reference only)
+-- archive/                Superseded documentation
+-- platformio.ini
+-- README.md               This file
```

---

## Troubleshooting

### Matrix does not light up

- Use an **external 5V / 4A+ PSU** for the matrix -- never draw power from the controller board
- Check HUB75 ribbon cable direction (pin 1 marking)
- Verify GPIO/pin assignments match your wiring in `config.h` (ESP32) or `config.py` (Pi/Rock Pi S)

### No UDP packets received

- Confirm the device IP and port 21324 are correct in the Q-SYS plugin properties
- Check any firewalls between the Q-SYS Core and the display controller
- ESP32: watch serial -- `pio device monitor --port COM4 --baud 115200`
- Pi / Rock Pi S: `sudo journalctl -u led-matrix -f`

### RPi service crash-loop with "snd_bcm2835" message

The on-board audio driver conflicts with the LED matrix PWM hardware.
The `install.sh` script blacklists it automatically.  To fix manually:

```bash
echo "blacklist snd_bcm2835" | sudo tee /etc/modprobe.d/blacklist-rgb-matrix.conf
sudo reboot
```

### Ghost text / wrong segment visible after layout change

Always press **Apply Layout** in the Q-SYS plugin after selecting a new layout.
This sends `config` + `clear` commands to reset inactive segments before new text arrives.

### Web UI not reachable

- ESP32: port 80, `http://<IP>/`
- Pi / Rock Pi S: port 8080, `http://<IP>:8080/`

---

## License

See [LICENSE](LICENSE) for details.

## Support

GitHub Issues: https://github.com/DHPKE/QSYS-LED-Matrix/issues
