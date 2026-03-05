# Rock Pi S Bus Error Fix

## Problem

The rpi-rgb-led-matrix library crashes with **signal 7 (BUS error)** on Rock Pi S when trying to initialize the LED matrix:

```
non-existent Revision: Could not determine Pi model
Failed to read revision from /proc/device-tree/system/linux,revision
Unknown Revision: Could not determine Pi model
led-matrix.service: Main process exited, code=killed, status=7/BUS
```

## Root Cause

The library attempts to detect the Raspberry Pi model by reading hardware revision from `/proc/device-tree/system/linux,revision` and accessing memory-mapped BCM hardware registers. On non-Pi hardware (Rock Pi S with RK3308 SoC), this causes a **bus error** because:

1. The revision file doesn't exist or has an unrecognized format
2. The library tries to access BCM-specific memory addresses that don't exist on RK3308
3. Even with `drop_privileges=False`, the RGBMatrix constructor still attempts hardware detection

## Solutions

### Option 1: NO_DISPLAY Mode (Quick Test)

Run the service in NO_DISPLAY mode to verify UDP and Web functionality without hardware:

```bash
# Temporary test
sudo LED_MATRIX_NO_DISPLAY=1 python3 /opt/led-matrix/main.py

# OR enable in systemd service
sudo systemctl edit led-matrix
# Add:
[Service]
Environment="LED_MATRIX_NO_DISPLAY=1"

sudo systemctl restart led-matrix
```

In NO_DISPLAY mode:
- ✓ UDP handler works (port 21324)
- ✓ Web UI works (port 80)
- ✓ No bus error crash
- ✗ No physical LED display output

### Option 2: Custom Hardware Mapping (RECOMMENDED)

The library supports custom hardware mappings via a C++ file. Create `/opt/led-matrix/hardware-mapping-rockpis.h`:

```cpp
// hardware-mapping-rockpis.h
// Custom hardware mapping for RADXA Rock Pi S (RK3308)

#include "hardware-mapping.h"

class RockPiSHardwareMapping : public HardwareMapping {
public:
  RockPiSHardwareMapping() : HardwareMapping("rockpis") {}
  
  bool Init() override {
    // RK3308 GPIO pins (Linux GPIO numbers)
    // See CORRECTED_PINOUT.md for pin-to-GPIO mapping
    
    InitGPIOPin(16);  // R1 - GPIO0_C0 (pin 13)
    InitGPIOPin(17);  // G1 - GPIO0_C1 (pin 15)
    InitGPIOPin(15);  // B1 - GPIO0_B7 (pin 11)
    InitGPIOPin(68);  // R2 - GPIO2_A4 (pin 7)
    InitGPIOPin(69);  // G2 - GPIO2_A5 (pin 12)
    InitGPIOPin(74);  // B2 - GPIO2_B2 (pin 16)
    InitGPIOPin(11);  // A  - GPIO0_B3 (pin 3)
    InitGPIOPin(12);  // B  - GPIO0_B4 (pin 5)
    InitGPIOPin(65);  // C  - GPIO2_A1 (pin 8)
    InitGPIOPin(64);  // D  - GPIO2_A0 (pin 10)
    InitGPIOPin(71);  // CLK - GPIO2_A7 (pin 22)
    InitGPIOPin(55);  // LAT - GPIO1_C7 (pin 19)
    InitGPIOPin(54);  // OE  - GPIO1_C6 (pin 21)
    
    return true;
  }
  
  void SetGPIOForOutput(int gpio, bool value) override {
    char path[256];
    
    // Export GPIO
    FILE* f = fopen("/sys/class/gpio/export", "w");
    if (f) {
      fprintf(f, "%d\n", gpio);
      fclose(f);
    }
    
    // Set direction
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    f = fopen(path, "w");
    if (f) {
      fprintf(f, "out\n");
      fclose(f);
    }
    
    // Set initial value
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    f = fopen(path, "w");
    if (f) {
      fprintf(f, "%d\n", value ? 1 : 0);
      fclose(f);
    }
  }
};

REGISTER_HARDWARE_MAPPING(RockPiSHardwareMapping);
```

**Rebuild the library:**

```bash
cd ~/rpi-rgb-led-matrix
sudo make clean
sudo make -C bindings/python install
```

**Update config.py:**

```python
MATRIX_HARDWARE_MAPPING = "rockpis"
```

### Option 3: Kernel Module Approach

Some users have patched the library to skip Pi detection entirely or use a kernel module for GPIO access. This requires modifying the C++ source in `lib/hardware-mapping.cc`.

## Verification

After applying a fix:

```bash
# Check service status
sudo systemctl status led-matrix

# Watch logs
sudo journalctl -u led-matrix -f

# Expected output (Option 2 - working display):
# ✓ LED matrix initialised
# ✓ UDP handler started on port 21324
# ✓ Web server started on port 80

# OR (Option 1 - NO_DISPLAY mode):
# LED_MATRIX_NO_DISPLAY set — running in NO-DISPLAY test mode
# ↳ UDP and web server functional, but NO physical panel output
```

## Current Status

- **main.py**: Updated to support LED_MATRIX_NO_DISPLAY environment variable
- **led-matrix.service**: Includes commented Environment line for NO_DISPLAY mode
- **Custom hardware mapping**: Template created in `hardware-mapping-rockpis.h`
- **GPIO pins**: Corrected assignments in config.py (see CORRECTED_PINOUT.md)

## Next Steps

1. **Test NO_DISPLAY mode** to verify UDP/Web functionality
2. **Implement custom hardware mapping** for actual display output
3. **Verify physical wiring** matches CORRECTED_PINOUT.md
4. **Disable UART0** on pins 8 & 10 (needed for C and D address lines)

## References

- [rpi-rgb-led-matrix library](https://github.com/hzeller/rpi-rgb-led-matrix)
- [Rock Pi S GPIO pinout](https://docs.radxa.com/en/rock-pi-s/hardware/rock-pi-s)
- CORRECTED_PINOUT.md (this repository)
- GPIO_PIN_MAPPING_ISSUE.md (this repository)
