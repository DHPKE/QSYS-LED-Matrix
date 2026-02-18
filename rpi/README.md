# RPi Zero 2 W + PoE HAT — LED Matrix Controller

Python port of the ESP32 Arduino firmware. Provides the **same Q-SYS UDP JSON
protocol**, **same web UI**, and **same 4-segment layout system** — now running
on a Raspberry Pi Zero 2 W with a PoE HAT for wired Ethernet power + network,
driving a 64×32 HUB75 LED panel via [`rpi-rgb-led-matrix`](https://github.com/hzeller/rpi-rgb-led-matrix).

---

## Hardware

| Item | Detail |
|------|--------|
| **SBC** | Raspberry Pi Zero 2 W |
| **Power + Ethernet** | PoE HAT (802.3af/at) — e.g. Waveshare PoE HAT (B) |
| **Display** | 64×32 HUB75 LED Matrix |
| **OS** | Raspberry Pi OS Lite (64-bit, Bookworm) |

### HUB75 Wiring (BCM pin numbers)

The `rpi-rgb-led-matrix` library uses a fixed "regular" mapping by default.
Solder/jumper your HUB75 cable accordingly:

| HUB75 Signal | BCM GPIO | Physical Pin |
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
| CLK| 17 | 11 |
| LAT| 4  | 7  |
| OE | 18 | 12 |

> ⚠ **PoE HAT fan pins**: Some PoE HATs use GPIO 4 or GPIO 14 for a cooling fan.
> If yours does, you must physically move the LAT or CLK wire and update
> `MATRIX_HARDWARE_MAPPING` / the library's `--led-gpio-mapping` accordingly,
> OR disable the fan in the HAT firmware.

---

## Installation

Flash **Raspberry Pi OS Lite (64-bit)** to a microSD card, enable SSH, connect
to your network via the PoE HAT, then:

```bash
# On the Pi
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/rpi
sudo bash install.sh
```

The install script will:
1. Install Python 3, Pillow, build tools
2. Clone and compile `rpi-rgb-led-matrix` + Python bindings
3. Copy app files to `/opt/led-matrix/`
4. Create config storage at `/var/lib/led-matrix/`
5. Install and start the **`led-matrix` systemd service** (auto-start on boot)

---

## Running manually (for testing)

```bash
# Full hardware mode (requires root for GPIO DMA)
sudo python3 /opt/led-matrix/main.py

# Virtual / NO_DISPLAY mode — web UI works, no physical panel needed
# (rgbmatrix import will fail gracefully, app continues)
python3 main.py
```

---

## Project structure

```
rpi/
├── main.py            ← entry point, render loop (port of main.cpp)
├── config.py          ← all constants (port of config.h)
├── segment_manager.py ← segment state, thread-safe (port of segment_manager.h)
├── udp_handler.py     ← UDP listener thread (port of udp_handler.h)
├── text_renderer.py   ← Pillow → rpi-rgb-led-matrix renderer (port of text_renderer.h)
├── web_server.py      ← HTTP server + identical web UI (port of web server in main.cpp)
├── led-matrix.service ← systemd unit file
└── install.sh         ← one-shot install script
```

---

## UDP Protocol (unchanged from ESP32)

Send JSON to `<pi-ip>:21324` via UDP.  The Q-SYS plugin (`LEDMatrix_Complete.qplug`) works **without any changes**.

```json
{"cmd":"text",  "seg":0, "text":"Hello", "color":"FFFFFF", "bgcolor":"000000",
 "font":"arial","size":"auto","align":"C","effect":"none","intensity":255}

{"cmd":"clear",      "seg":0}
{"cmd":"clear_all"}
{"cmd":"brightness", "value":200}
{"cmd":"config",     "seg":0, "x":0, "y":0, "w":64, "h":32}
```

---

## Web UI

Open `http://<pi-ip>/` — identical to the ESP32 web interface:
- Live canvas preview (1-second polling)
- Per-segment text, colour, alignment, effect
- Brightness slider
- Layout presets (split vertical / horizontal / quad / fullscreen)

> If you set `WEB_PORT = 8080` in `config.py` you can run without `sudo`.

---

## Useful commands

```bash
sudo systemctl status led-matrix    # check service
sudo journalctl -u led-matrix -f    # live logs
sudo systemctl restart led-matrix   # restart after config change
```
