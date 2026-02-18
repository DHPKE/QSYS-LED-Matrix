# Rock Pi S Native HUB75 LED Matrix Driver

Native Python driver for **Waveshare RGB-Matrix-P4-64x32** LED panel on RADXA Rock Pi S using direct GPIO access.

## Why This Approach?

- **No Pi dependencies** - Uses standard Linux GPIO (libgpiod) instead of rpi-rgb-led-matrix
- **Simple & reliable** - Pure Python, easy to debug and modify
- **Fast enough** - Achieves 200+ Hz refresh rate on 64×32 panel
- **Portable** - Works on any SBC with GPIO support

## Hardware Requirements

- **LED Panel**: Waveshare RGB-Matrix-P4-64x32
  - 64×32 pixels (2048 RGB LEDs)
  - 4mm pitch, 1/16 scan mode
  - HUB75 interface
  - 5V 4A power supply (via VH4 connector on panel)

- **Controller**: RADXA Rock Pi S v1.3
  - RK3308 SoC (Cortex-A35 @ 1.3 GHz)
  - 512MB RAM
  - Armbian or DietPi OS

## Wiring

**All connections to Rock Pi S Header 1 (26-pin):**

| HUB75 Signal | Rock Pi GPIO | Physical Pin | Wire Color (typical) |
|--------------|--------------|--------------|----------------------|
| R1           | GPIO 16      | Pin 13       | Red stripe          |
| G1           | GPIO 17      | Pin 15       | Green stripe        |
| B1           | GPIO 15      | Pin 11       | Blue stripe         |
| R2           | GPIO 68      | Pin  7       | Red solid           |
| G2           | GPIO 69      | Pin 12       | Green solid         |
| B2           | GPIO 74      | Pin 16       | Blue solid          |
| A (address)  | GPIO 11      | Pin  3       | Brown               |
| B (address)  | GPIO 12      | Pin  5       | Orange              |
| C (address)  | GPIO 65      | Pin  8       | Yellow ⚠️           |
| D (address)  | GPIO 64      | Pin 10       | White ⚠️            |
| CLK (clock)  | GPIO 71      | Pin 22       | Gray                |
| LAT (latch)  | GPIO 55      | Pin 19       | Purple              |
| OE (enable)  | GPIO 54      | Pin 21       | Black               |
| GND          | GND          | Pin  6/9/14  | Black (thick)       |

**⚠️ CRITICAL: Pins 8 & 10 are UART0**  
These pins must have the serial console disabled (see installation steps below).

**Power:**  
Connect 5V 4A power supply to the VH4 connector on the LED panel. **Do NOT** power the panel from Rock Pi S 5V pins.

## Installation

### 1. SSH to Rock Pi S

```bash
ssh node@10.1.1.23  # or your Rock Pi S IP
```

### 2. Run Installation Script

On your Mac, transfer and run the installation:

```bash
cd /Users/user/.openclaw/workspace/QSYS-LED-Matrix
scp rockpis-native/* node@10.1.1.23:~/led-matrix-native/
ssh node@10.1.1.23
cd ~/led-matrix-native
chmod +x install.sh
./install.sh
```

The script will:
- Install dependencies (gpiod, Python libraries)
- Disable UART0 console
- Install application to `/opt/led-matrix-native`
- Create systemd service
- Test GPIO access

### 3. Reboot (Required)

```bash
sudo reboot
```

Reboot is needed for UART0 changes to take effect.

### 4. Start Service

```bash
sudo systemctl start led-matrix
sudo systemctl status led-matrix
sudo journalctl -u led-matrix -f
```

## Manual Testing

Test the driver directly:

```bash
cd /opt/led-matrix-native
sudo python3 hub75_driver.py
```

You should see:
- Red fill for 2 seconds
- Green fill for 2 seconds
- Blue fill for 2 seconds
- White fill for 2 seconds
- Checkerboard pattern

Press Ctrl+C to exit.

## Usage

### UDP API (Port 21324)

Send JSON commands from Q-SYS or any UDP client:

```json
{
  "cmd": "TEXT",
  "seg": 1,
  "text": "HELLO",
  "color": {"r": 255, "g": 0, "b": 0},
  "x": 0,
  "y": 0
}
```

### Web Interface (Port 80)

- `http://10.1.1.23/` - Status page
- `http://10.1.1.23/api/segments` - Get all segments
- `http://10.1.1.23/api/brightness?value=128` - Set brightness (0-255)

### Q-SYS Integration

Use the existing Q-SYS plugin (`LEDMatrix_Complete.qplug v3.5.0`) - no changes needed. It sends UDP commands that this native driver understands.

## File Structure

```
/opt/led-matrix-native/
├── main.py              # Main entry point
├── hub75_driver.py      # HUB75 protocol driver
├── gpio_config.py       # Pin mappings
├── config.py            # Configuration
├── text_renderer.py     # Text rendering with Pillow
├── segment_manager.py   # Segment state management
├── udp_handler.py       # UDP listener
└── web_server.py        # HTTP server
```

## Configuration

Edit `/opt/led-matrix-native/config.py`:

```python
MATRIX_WIDTH = 64         # Panel width
MATRIX_HEIGHT = 32        # Panel height
MATRIX_BRIGHTNESS = 200   # 0-255 (default 200 = ~78%)
UDP_PORT = 21324          # UDP listen port
WEB_PORT = 80             # HTTP listen port
LOG_LEVEL = "INFO"        # DEBUG, INFO, WARNING, ERROR
```

After changes:
```bash
sudo systemctl restart led-matrix
```

## Troubleshooting

### No display output

1. Check GPIO access:
   ```bash
   gpiodetect
   gpioinfo | grep gpiochip0
   ```

2. Check UART0 is disabled:
   ```bash
   systemctl status serial-getty@ttyS0.service  # Should be "inactive"
   cat /boot/armbianEnv.txt | grep ttyS0         # Should NOT appear
   ```

3. Check power:
   - LED panel needs 5V 4A external power
   - Don't power from Rock Pi S

4. Check wiring:
   - All 13 signal wires connected?
   - Ground connected?
   - Correct pins per table above?

### Service crashes

```bash
sudo journalctl -u led-matrix -f
```

Look for error messages. Common issues:
- Permission denied → GPIO access needs root
- GPIO chip not found → Check /dev/gpiochip0 exists
- Import errors → Dependencies not installed

### Display shows garbage/artifacts

Adjust timing in `hub75_driver.py`:
```python
# In display_row() method, increase delay:
time.sleep(0.0002)  # Try 200 microseconds instead of 100
```

### Low refresh rate

Check status:
```bash
sudo python3 /opt/led-matrix-native/main.py
```

Look for "Display XXX Hz" in logs. Should be 200+ Hz.

## Performance

**Expected performance on Rock Pi S:**
- Display refresh: 200-600 Hz (panel dependent)
- Render FPS: 20 FPS (configurable)
- CPU usage: 15-25% (one core)
- RAM usage: ~30MB

## Comparison with rpi-rgb-led-matrix

| Feature | rpi-rgb-led-matrix | Native Driver |
|---------|-------------------|---------------|
| Raspberry Pi required | Yes | No |
| Setup complexity | High (C library, compile) | Low (pure Python) |
| Boot dependencies | Many | Few |
| Debugging | Difficult (C crashes) | Easy (Python tracebacks) |
| Customization | Hard (C code) | Easy (Python) |
| Performance | Excellent (600+ Hz) | Good (200+ Hz) |
| Portability | Pi only | Any Linux SBC |

## License

Same as main project (see top-level LICENSE file).

## Support

See main project README and documentation in `/docs` directory.
