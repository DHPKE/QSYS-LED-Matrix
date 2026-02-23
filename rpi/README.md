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

## Features

### Display Performance & Configuration
- **Current stable settings** (optimized for RPi 4 / RPi Zero 2 W):
  - GPIO slowdown: 2 (~250-300Hz refresh, balanced performance)
  - PWM bits: 7 (128 color levels per channel, 2M total colors)
  - Scan mode: 0 (progressive scanning)
  - PWM LSB: 200ns (stable timing)
  - Font rendering: Binary threshold at 128 (sharp, no anti-aliasing)
- **Hardware compatibility**: Works with RPi 4, RPi Zero 2 W
- **Signal quality**: Best results with Adafruit HUB75 adapter and quality cables
- **Hardware PWM support**: Full color depth with smooth brightness control

### Orientation Support
- **Landscape mode**: Native 64×32 (default)
- **Portrait mode**: Rotated 32×64 with automatic layout adjustment
- **Dynamic switching**: Change orientation via WebUI or UDP command
- **Auto-layout**: Segments automatically resize when orientation changes

### WebUI Improvements
- **Orientation-aware preview**: Canvas automatically resizes (64×32 ↔ 32×64)
- **Real IP display**: Shows actual network IP (fixes 127.0.1.1 issue)
- **Editable network settings**: IP address and UDP port with Apply button
- **Layout presets**: Separate presets for landscape and portrait modes
- **Live updates**: 1-second polling with dirty-state optimization

### Segment Management
- **4 independent segments** with individual text, color, alignment, effects
- **Smart clearing**: Clear text without deactivating segments
- **Layout presets**: 7 pre-configured layouts for both orientations
- **Thread-safe**: Concurrent access from UDP, web, and render threads

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

### Updating existing installation

After pulling new changes from git:

```bash
cd QSYS-LED-Matrix/rpi
bash update.sh
```

This will copy updated Python files and restart the service.

---

## Performance Optimization

For **best display performance** and **minimal flickering**, run the system optimization script:

```bash
cd /opt/led-matrix
sudo ./optimize-system.sh
sudo reboot
```

### What it does:

1. **Disables Desktop GUI** — Frees ~200MB RAM and reduces CPU load by 30-40%
2. **Sets process priority** — Real-time scheduling (FIFO) for consistent timing
3. **Optimizes swappiness** — Reduces swap usage (60 → 10) for better responsiveness
4. **Disables unnecessary services** — Stops avahi-daemon, bluetooth, triggerhappy
5. **Configures CPU isolation** — Reserves CPU core 3 exclusively for LED matrix
6. **Optimizes filesystem** — Adds `noatime` flag to reduce disk I/O

### Expected improvements:

| Metric | Before | After |
|---|---|---|
| CPU Usage | ~60-65% | ~30-40% |
| Memory Used | 210MB + 106MB swap | 150MB + minimal swap |
| Display Flicker | Random lines | Near-zero |
| Response Time | Good | Excellent |

### Code optimizations included:

- **Smart rendering**: Skips render cycles when no segments are dirty
- **CPU affinity**: Pins process to dedicated CPU core
- **Reduced logging**: 200-frame batches instead of 50
- **Fixed sleep timing**: Eliminates adaptive sleep overhead
- **Service priority**: Nice=-10, FIFO scheduling, realtime I/O

> ⚠️ **Reboot required**: System optimizations take effect after reboot.

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

{"cmd":"layout",     "preset":1}
{"cmd":"clear",      "seg":0}
{"cmd":"clear_all"}
{"cmd":"brightness", "value":200}
{"cmd":"orientation", "value":"portrait"}
{"cmd":"orientation", "value":"landscape"}
{"cmd":"config",     "seg":0, "x":0, "y":0, "w":64, "h":32}
```

---

## Web UI

Open `http://<pi-ip>:8080` — enhanced web interface with:
- **Live canvas preview**: Orientation-aware (automatically switches 64×32 ↔ 32×64)
- **Network settings**: Editable IP address and UDP port
- **Per-segment controls**: Text, color, alignment, effects, intensity
- **Brightness slider**: Real-time brightness adjustment (0-100%)
- **Layout presets**: 7 presets each for landscape and portrait modes
- **Orientation toggle**: Switch between landscape/portrait modes
- **Real-time updates**: 1-second polling with visual feedback

> Default port is 8080 (no sudo required). Change `WEB_PORT` in `config.py` if needed.

---

## Troubleshooting

### Display Flickering

If you see single-pixel flickering that persists with software optimizations, it's likely a signal integrity issue:

**Software settings to try** (in `config.py`):
- Increase `MATRIX_GPIO_SLOWDOWN` (2 → 3 for more stable timing)
- Reduce `MATRIX_PWM_BITS` (7 → 6 for faster refresh)
- Try `MATRIX_SCAN_MODE = 1` (interlaced scanning)

**Hardware recommendations**:
- Use an **Adafruit HUB75 RGB Matrix Bonnet** or similar adapter with:
  - Proper 3.3V → 5V level shifting
  - Signal buffering for data/clock lines
  - Quality ribbon cable (short, proper gauge)
- Poor cable quality is the #1 cause of persistent flickering
- Avoid long or low-quality ribbon cables
- Ensure good grounding between Pi and panel

### Font Quality

Font rendering uses binary thresholding (threshold=128 by default in `text_renderer.py`):
- **Lower threshold** (80-120): Thicker, bolder fonts, may show more flicker on edges
- **Higher threshold** (140-200): Thinner fonts, less flicker, may be hard to read
- **Current default** (128): Balanced weight, sharp edges, no anti-aliasing

### Service Issues

```bash
# Check service status
sudo systemctl status led-matrix

# View real-time logs
sudo journalctl -u led-matrix -f

# Restart service
sudo systemctl restart led-matrix

# View recent errors
sudo journalctl -u led-matrix -n 50
```

---

## Useful commands

```bash
sudo systemctl status led-matrix    # check service
sudo journalctl -u led-matrix -f    # live logs
sudo systemctl restart led-matrix   # restart after config change
```
