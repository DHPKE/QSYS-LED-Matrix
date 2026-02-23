# HUB75 to Raspberry Pi Zero 2 W - Complete Wiring Map

**Hardware**: Raspberry Pi Zero 2 W + 64×32 HUB75 LED Matrix  
**Configuration**: Adafruit RGB Matrix Bonnet/HAT pinout  
**Date**: February 22, 2026  
**Status**: ✅ VERIFIED WORKING

## Pin Mapping Table

| HUB75 Pin | Signal | Wire Color | Function | BCM GPIO | Physical Pin | Alt Function |
|-----------|--------|------------|----------|----------|--------------|--------------|
| 1 | R1 | 🟤 brown1 | Red (top half) | GPIO 11 | Pin 23 | SCLK |
| 2 | G1 | 🔴 red1 | Green (top half) | GPIO 27 | Pin 13 | - |
| 3 | B1 | 🟠 orange1 | Blue (top half) | GPIO 7 | Pin 26 | CE1 |
| 4 | GND | 🟡 yellow1 | Ground | GND | Pin 14 | - |
| 5 | R2 | 🟢 green1 | Red (bottom half) | GPIO 8 | Pin 24 | CE0 |
| 6 | G2 | 🔵 blue1 | Green (bottom half) | GPIO 9 | Pin 21 | MISO |
| 7 | B2 | 🟣 purple1 | Blue (bottom half) | GPIO 10 | Pin 19 | MOSI |
| 8 | GND E | ⚫ grey1 | Ground | GND | Pin 39 | - |
| 9 | A | ⚪ white1 | Row select A | GPIO 22 | Pin 15 | - |
| 10 | B | 🟤 brown2 | Row select B | GPIO 23 | Pin 16 | - |
| 11 | C | 🔴 red2 | Row select C | GPIO 24 | Pin 18 | - |
| 12 | D | 🟠 orange2 | Row select D | GPIO 25 | Pin 22 | - |
| 13 | CLK | 🟡 yellow2 | Clock | GPIO 17 | Pin 11 | - |
| 14 | LAT | 🟢 green2 | Latch | GPIO 4 | Pin 7 | GPCLK0 |
| 15 | OE | 🔵 blue2 | Output Enable | GPIO 18 | Pin 12 | PWM0 |
| 16 | GND | 🟣 purple2 | Ground | GND | Pin 20 | - |

## Signal Groups

### RGB Data Lines (6 wires)
Controls the color of each LED:
- **Top Half**: 
  - 🟤 brown1 (R1) → GPIO 11, Pin 23
  - 🔴 red1 (G1) → GPIO 27, Pin 13
  - 🟠 orange1 (B1) → GPIO 7, Pin 26
  
- **Bottom Half**: 
  - 🟢 green1 (R2) → GPIO 8, Pin 24
  - 🔵 blue1 (G2) → GPIO 9, Pin 21
  - 🟣 purple1 (B2) → GPIO 10, Pin 19

### Row Address Lines (4 wires)
Selects which row to display (0-31 for 32-row panel):
- ⚪ white1 (A) → GPIO 22, Pin 15
- 🟤 brown2 (B) → GPIO 23, Pin 16
- 🔴 red2 (C) → GPIO 24, Pin 18
- 🟠 orange2 (D) → GPIO 25, Pin 22

### Control Signals (3 wires)
Timing and output control:
- 🟡 yellow2 (CLK) → GPIO 17, Pin 11 — Data clock
- 🟢 green2 (LAT) → GPIO 4, Pin 7 — Latch signal (transfers data to display)
- 🔵 blue2 (OE) → GPIO 18, Pin 12 — Output enable (active low, controls brightness)

### Ground Lines (4 wires)
- 🟡 yellow1 → Pin 14
- ⚫ grey1 → Pin 39
- 🟣 purple2 → Pin 20
- Additional GND available: Pins 6, 9, 25, 30, 34

## Wire-to-Pin Quick Reference

### Brown Wires
- 🟤 **brown1** → R1 → GPIO 11 → Pin 23
- 🟤 **brown2** → B → GPIO 23 → Pin 16

### Red Wires
- 🔴 **red1** → G1 → GPIO 27 → Pin 13
- 🔴 **red2** → C → GPIO 24 → Pin 18

### Orange Wires
- 🟠 **orange1** → B1 → GPIO 7 → Pin 26
- 🟠 **orange2** → D → GPIO 25 → Pin 22

### Yellow Wires
- 🟡 **yellow1** → GND → Pin 14
- 🟡 **yellow2** → CLK → GPIO 17 → Pin 11

### Green Wires
- 🟢 **green1** → R2 → GPIO 8 → Pin 24
- 🟢 **green2** → LAT → GPIO 4 → Pin 7

### Blue Wires
- 🔵 **blue1** → G2 → GPIO 9 → Pin 21
- 🔵 **blue2** → OE → GPIO 18 → Pin 12

### Purple Wires
- 🟣 **purple1** → B2 → GPIO 10 → Pin 19
- 🟣 **purple2** → GND → Pin 20

### Special Wires
- ⚫ **grey1** → GND → Pin 39
- ⚪ **white1** → A → GPIO 22 → Pin 15

## Configuration Settings

**File**: `/opt/led-matrix/config.py`

```python
MATRIX_HARDWARE_MAPPING = "adafruit-hat"
MATRIX_GPIO_SLOWDOWN    = 2
MATRIX_PWM_BITS        = 7
MATRIX_BRIGHTNESS       = 50
```

## Important Notes

1. **Hardware Mapping**: This uses the **Adafruit RGB Matrix Bonnet/HAT** pinout, not the "regular" mapping
2. **GPIO Numbering**: All GPIO numbers use BCM (Broadcom) numbering, NOT physical pin numbers
3. **HUB75 Keying**: Pin 1 is typically marked with a triangle or arrow on the connector
4. **RPi Orientation**: Pin 1 is closest to the SD card slot, Pin 40 is at the opposite end
5. **Ground Distribution**: Multiple ground wires ensure proper current return path and signal integrity
6. **External Power**: LED matrix MUST have separate 5V power supply (3-5A recommended)
7. **Pin Conflicts**: GPIO 4 (LAT) may conflict with some PoE HAT fans - verify your HAT datasheet

## Physical Connection Tips

1. **Connector Orientation**: Align HUB75 Pin 1 marking with connector keying
2. **Cable Seating**: Ensure all wires are fully inserted and locked in place
3. **No Shorts**: Verify no adjacent pins are bridged or bent
4. **Ground Continuity**: All ground wires should have continuity to RPi GND
5. **Power Supply**: Matrix needs external 5V power (3-5A for 64×32 panel)

## Verification

✅ Configuration verified working: February 22, 2026  
✅ Hardware mapping: adafruit-hat  
✅ Display output: Correct, no corruption  
✅ Service status: Active and running  

## Related Documentation

- [docs/PINOUT.md](docs/PINOUT.md) - Detailed GPIO configuration
- [docs/HARDWARE_SETUP.md](docs/HARDWARE_SETUP.md) - Complete setup guide
- [VERIFIED_PINOUT.md](VERIFIED_PINOUT.md) - Pinout verification details
- [rpi/config.py](rpi/config.py) - Configuration file

---

**Created**: February 22, 2026  
**Last Updated**: February 22, 2026  
**Pinout**: Adafruit RGB Matrix Bonnet/HAT (verified working)
