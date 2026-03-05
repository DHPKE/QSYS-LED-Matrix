# Rock Pi S - GPIO Pin Mapping Issue & Solution

## Problem

The `rpi-rgb-led-matrix` library's Python bindings on Rock Pi S don't expose the `gpio_*` attributes needed to override pin mappings. The error you're seeing:

```
'rgbmatrix.core.RGBMatrixOptions' object has no attribute 'gpio_r1'
```

This means the library is falling back to NO_DISPLAY mode (virtual mode). The good news is that `drop_privileges=False` worked and prevented the bus error!

## Root Cause

The library's Python bindings were compiled without GPIO pin override support, so it falls back to using the hardcoded "regular" mapping which expects Raspberry Pi BCM pin numbers, not Rock Pi S Linux GPIO numbers.

## Solution Options

### Option 1: Use Adafruit HAT Adapter (RECOMMENDED - Hardware Solution)

Use an Adafruit RGB Matrix HAT or Bonnet which has a level shifter and connects via the standard pinout. This is the most reliable solution.

### Option 2: Custom Hardware Mapping (Software - Requires Recompile)

Edit the library source and add a Rock Pi S mapping:

1. On the Rock Pi S:
```bash
cd ~/
git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
cd rpi-rgb-led-matrix/lib
```

2. Edit `hardware-mapping.c` and add this at the end of the `hardware_mappings` array (before the NULL):

```c
static struct HardwareMapping rockpi_s_mapping = {
  .name = "rockpi-s",
  .output_enable = 24, .clock = 22, .strobe = 23,
  .a = 11, .b = 12, .c = 13, .d = 14, .e = -1,
  .p0_r1 = 16, .p0_g1 = 17, .p0_b1 = 18,
  .p0_r2 = 19, .p0_g2 = 20, .p0_b2 = 21,
};
```

Then add `&rockpi_s_mapping,` to the `hardware_mappings` array.

3. Recompile:
```bash
cd ~/rpi-rgb-led-matrix
make clean
make -j$(nproc)
cd bindings/python
sudo make build-python
sudo make install-python
```

4. Update `/opt/led-matrix/config.py`:
```python
MATRIX_HARDWARE_MAPPING = "rockpi-s"
```

5. Restart:
```bash
sudo systemctl restart led-matrix
```

### Option 3: Quick Test with Regular Pi Wiring

Rewire your HUB75 cable to match the Raspberry Pi "regular" mapping BCM pins:

| HUB75 | Pi BCM | Rock Pi S Physical Pin |
|-------|--------|------------------------|
| R1    | 5      | 29                     |
| G1    | 13     | 33                     |
| B1    | 6      | 31                     |
| R2    | 12     | 32                     |
| G2    | 16     | 36                     |
| B2    | 23     | 16                     |
| A     | 22     | 15                     |
| B     | 26     | 37                     |
| C     | 27     | 13                     |
| D     | 20     | 38                     |
| CLK   | 17     | 11                     |
| LAT   | 4      | 7                      |
| OE    | 18     | 12                     |

**NOTE**: This may require disabling additional conflicting interfaces (I2C, SPI, etc.)

## Current Status

The code successfully prevents the bus error with `drop_privileges=False`, but you need one of the above solutions to actually drive the display.

## Verification

After implementing one of the solutions, you should see:

```
20:16:41 [INFO] ✓ LED matrix initialised
20:16:41 [INFO]   ↳ Hardware pulsing disabled (bit-bang OE- mode for RK3308)
20:16:41 [INFO]   ↳ Privilege drop disabled (non-Pi hardware mode)
```

And the display should show "READY" in green.
