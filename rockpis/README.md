# RADXA Rock Pi S — LED Matrix Controller

Python port of the ESP32 Arduino firmware. Provides the **same Q-SYS UDP JSON
protocol**, **same web UI**, and **same 4-segment layout system** — running on
a RADXA Rock Pi S with **built-in 100 Mbps Ethernet**, driving a 64×32 HUB75
LED panel via [`rpi-rgb-led-matrix`](https://github.com/hzeller/rpi-rgb-led-matrix).

---

## Hardware

| Item | Detail |
|------|--------|
| **SBC** | RADXA Rock Pi S v1.3 |
| **SoC** | Rockchip RK3308, quad-core Cortex-A35 @ 1.3 GHz, 512 MB DDR3 |
| **Ethernet** | Built-in 100 Mbps (IP101G PHY) — **no GPIO header pins consumed** |
| **Power** | PoE splitter on Ethernet cable (5V/2A+), or USB-C 5V |
| **Display** | 64×32 HUB75 LED Matrix |
| **OS** | Armbian (Bookworm / Jammy, minimal, 64-bit) |
| **GPIO header** | 26-pin |

> **Why Rock Pi S vs RPi Zero 2 W?**
> The Rock Pi S has built-in Ethernet so **zero GPIO header pins are used for
> networking**, leaving all 17 usable GPIOs free for HUB75.  It is also
> ~€10–15 cheaper than a Pi Zero 2 W + PoE HAT combo.  The same Python
> codebase runs identically on both boards.

---

## GPIO Wiring

### RK3308 GPIO numbering

The RK3308 names GPIOs as **GPIOx_Py** where:
- `x` = bank (0–4)
- `y` = pin in bank: A=0-7, B=8-15, C=16-23, D=24-31
- **Linux sysfs number** = `bank × 32 + offset`

### HUB75 wiring table

| HUB75 Signal | RK3308 GPIO | Linux# | Physical pin | Notes |
|---|---|---|---|---|
| R1 | GPIO0_C0 | 16 | 11 | |
| G1 | GPIO0_C1 | 17 | 12 | |
| B1 | GPIO0_C2 | 18 | 13 | |
| R2 | GPIO0_C3 | 19 | 15 | |
| G2 | GPIO0_C4 | 20 | 16 | |
| B2 | GPIO0_C5 | 21 | 18 | |
| A  | GPIO0_B3 | 11 |  3 | I2C0_SDA — disable I2C0 if in use |
| B  | GPIO0_B4 | 12 |  5 | I2C0_SCL — disable I2C0 if in use |
| C  | GPIO0_B5 | 13 |  8 | **⚠ UART0_TX** — must disable serial console (see below) |
| D  | GPIO0_B6 | 14 |  7 | **⚠ UART0_RX** — must disable serial console (see below) |
| CLK | GPIO0_C6 | 22 | 22 | |
| LAT | GPIO0_C7 | 23 | 19 | SPI0_MOSI — disable SPI0 if in use |
| OE  | GPIO0_D0 | 24 | 21 | SPI0_MISO — disable SPI0 if in use |

**GND** — connect HUB75 GND to any GND pin (6, 9, 14, 20, 25).

> The HUB75 connector uses 5 V logic; the Rock Pi S GPIOs are 3.3 V.
> Most HUB75 panels tolerate 3.3 V inputs, but if you see erratic pixels
> add a simple 74HCT245 level-shifter board between the Pi and the panel.

---

## ⚠ Critical: Disable the UART0 Serial Console

By default, Armbian uses GPIO0_B5 and GPIO0_B6 (Linux 13/14, physical pins
8/10) as the serial console (`ttyS0`).  These are the HUB75 **C** and **D**
address lines.  You **must** disable the serial console before the GPIO pins
can be used for HUB75.

The `install.sh` script handles this automatically.  To do it manually:

```bash
# Disable the systemd serial getty
sudo systemctl disable --now serial-getty@ttyS0

# Remove console=ttyS0,... from the kernel command line
sudo nano /boot/armbianEnv.txt
# → delete the console=ttyS0,1500000 token from the 'extraargs' line

# Reboot to apply
sudo reboot
```

After rebooting, verify the pin is free:
```bash
cat /sys/class/gpio/gpio13/direction   # should be 'out' after first LED init
```

---

## Installation

Flash **Armbian** (Bookworm minimal, 64-bit) to a microSD card, enable SSH,
connect to your network via the built-in Ethernet port, then:

```bash
# On the Rock Pi S
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/rockpis
sudo bash install.sh
```

The install script will:
1. Disable the UART0 serial console (frees pins 8 & 10 for HUB75)
2. Install Python 3, Pillow, build tools (`apt`)
3. Clone and compile `rpi-rgb-led-matrix` + Python bindings
4. Copy app files to `/opt/led-matrix/`
5. Create config storage at `/var/lib/led-matrix/`
6. Install and start the **`led-matrix` systemd service** (auto-start on boot)
7. Prompt for a reboot (required to fully apply the UART console change)

---

## Running manually (for testing)

```bash
# Full hardware mode (requires root for /dev/mem GPIO access)
sudo python3 /opt/led-matrix/main.py

# Virtual / NO_DISPLAY mode — web UI works, no physical panel needed
# (rgbmatrix import will fail gracefully, app continues)
python3 main.py
```

---

## Differences from the RPi Zero 2 W version

| Feature | RPi Zero 2 W (`rpi/`) | Rock Pi S (`rockpis/`) |
|---|---|---|
| Ethernet | Via PoE HAT | Built-in 100 Mbps |
| PoE | Native (HAT) | Via PoE splitter on cable |
| Hardware PWM pulsing | Enabled | **Disabled** (RK3308 has no BCM PWM) |
| GPIO slowdown default | 2 | 2 (try 3 if display shows garbage) |
| Serial console conflict | None | UART0 on pins 8/10 — must disable |
| `config.py` GPIO numbers | BCM (RPi) | RK3308 Linux sysfs numbers |
| All `.py` logic | — | **Identical** |

---

## Project structure

```
rockpis/
├── main.py            ← entry point, render loop (Rock Pi S adaptation)
├── config.py          ← all constants, RK3308 GPIO assignments
├── segment_manager.py ← segment state, thread-safe (identical to rpi/)
├── udp_handler.py     ← UDP listener thread (identical to rpi/)
├── text_renderer.py   ← Pillow → rpi-rgb-led-matrix renderer (identical to rpi/)
├── web_server.py      ← HTTP server + web UI (Rock Pi S subtitle)
├── led-matrix.service ← systemd unit file
└── install.sh         ← one-shot install script (Armbian-specific)
```

---

## UDP Protocol (unchanged from ESP32)

Send JSON to `<rockpis-ip>:21324` via UDP.  The Q-SYS plugin
(`LEDMatrix_Complete.qplug`) works **without any changes**.

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

Open `http://<rockpis-ip>/` — identical to the ESP32 web interface:
- Live canvas preview (1-second polling)
- Per-segment text, colour, alignment, effect
- Brightness slider
- Layout presets (split vertical / horizontal / quad / fullscreen)

> If you set `WEB_PORT = 8080` in `config.py` you can run without `sudo`.

---

## Tuning

### Display shows colour garbage / incorrect patterns
Increase `MATRIX_GPIO_SLOWDOWN` in `config.py`:
```python
MATRIX_GPIO_SLOWDOWN = 3   # or 4
```
Then restart: `sudo systemctl restart led-matrix`

### Display is too bright / draws too much current
Lower `MATRIX_BRIGHTNESS` in `config.py` (0–100).  You can also use the
brightness slider in the web UI or send a `brightness` UDP command.

### I2C devices on pins 3/5 conflict with HUB75 A/B lines
Disable the I2C0 overlay in `/boot/armbianEnv.txt`:
```
# Remove or comment out:
overlays=i2c0
```

---

## Useful commands

```bash
sudo systemctl status led-matrix    # check service
sudo journalctl -u led-matrix -f    # live logs
sudo systemctl restart led-matrix   # restart after config change
sudo systemctl stop led-matrix      # stop service

# Check which process owns a GPIO pin
ls -la /sys/class/gpio/gpio13/

# Quick hardware test without the full app
sudo python3 -c "
from rgbmatrix import RGBMatrix, RGBMatrixOptions
o = RGBMatrixOptions()
o.rows, o.cols = 32, 64
o.disable_hardware_pulsing = True
o.gpio_slowdown = 2
m = RGBMatrix(options=o)
c = m.CreateFrameCanvas()
c.SetPixel(0, 0, 255, 0, 0)   # red dot at top-left
m.SwapOnVSync(c)
input('Press Enter to exit')
m.Clear()
"
```
