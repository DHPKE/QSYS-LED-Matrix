# Native Rock Pi S HUB75 LED Matrix Driver
## Implementation Plan

### Architecture Overview

```
┌─────────────────────────────────────────┐
│  UDP/HTTP API (existing from ESP32)    │
│  - Text rendering with Pillow          │
│  - Segment management                   │
│  - Color/brightness control             │
└────────────┬────────────────────────────┘
             │
┌────────────▼────────────────────────────┐
│  Native HUB75 Driver (NEW)             │
│  - Direct GPIO access via gpiod         │
│  - Pure Python bit-banging             │
│  - Double buffering                     │
│  - PWM brightness via bit planes        │
└────────────┬────────────────────────────┘
             │
┌────────────▼────────────────────────────┐
│  Rock Pi S GPIO (RK3308)               │
│  - 13 pins on Header 1                  │
│  - libgpiod access (kernel 4.8+)       │
└─────────────────────────────────────────┘
```

### Why This Approach Works

**Advantages:**
1. **No Pi dependencies** - Uses standard Linux GPIO subsystem
2. **Simple to debug** - Pure Python, easy to add logging
3. **Portable** - Will work on any SBC with GPIO
4. **Fast enough** - Python + gpiod can achieve 200+ Hz refresh at 64×32
5. **No kernel modules** - Userspace only, no /dev/mem hacks

**Technical Details:**
- HUB75 protocol is just GPIO bit-banging (clock, latch, data)
- 64×32 panel = 2048 pixels = manageable in Python
- Use libgpiod (modern, replaces deprecated sysfs GPIO)
- PWM brightness via binary code modulation (BCM)

### Implementation Steps

#### Phase 1: GPIO Setup & Basic Test (30 min)
1. Install libgpiod + Python bindings
2. Export all 13 GPIOs
3. Write simple blink test to verify GPIO access
4. Test all pins can toggle

#### Phase 2: HUB75 Protocol Implementation (2 hours)
1. Implement row scanning (address lines A, B, C, D)
2. Implement RGB data shifting (6 data pins + CLK)
3. Implement latch & output enable
4. Display single color pattern (all red, all green, etc.)

#### Phase 3: Frame Buffer & Rendering (2 hours)
1. Create 64×32 RGB frame buffer
2. Implement scan routine (refresh loop)
3. Add double buffering
4. Test with simple patterns

#### Phase 4: Integration with Existing Code (1 hour)
1. Replace RGBMatrix calls with native driver
2. Keep all existing UDP/HTTP/segment code
3. Test with Q-SYS plugin

#### Phase 5: Optimization (1 hour)
1. Add brightness PWM (bit planes)
2. Optimize refresh rate
3. Add error handling

### GPIO Pin Mapping (Rock Pi S Header 1)

```
Signal | GPIO# | Pin | RK3308 Name
-------|-------|-----|-------------
R1     | 16    | 13  | GPIO0_C0
G1     | 17    | 15  | GPIO0_C1
B1     | 15    | 11  | GPIO0_B7
R2     | 68    | 7   | GPIO2_A4
G2     | 69    | 12  | GPIO2_A5
B2     | 74    | 16  | GPIO2_B2
A      | 11    | 3   | GPIO0_B3
B      | 12    | 5   | GPIO0_B4
C      | 65    | 8   | GPIO2_A1 (UART TX - disable console!)
D      | 64    | 10  | GPIO2_A0 (UART RX - disable console!)
CLK    | 71    | 22  | GPIO2_A7
LAT    | 55    | 19  | GPIO1_C7
OE     | 54    | 21  | GPIO1_C6
```

### Dependencies

```bash
# Minimal dependencies
sudo apt install python3-gpiod python3-pillow fonts-dejavu-core

# Optional for testing
sudo apt install gpiod  # command-line GPIO tools
```

### File Structure

```
rockpis-native/
├── hub75_driver.py       # Core HUB75 driver class
├── gpio_config.py        # Pin mappings & GPIO setup
├── frame_buffer.py       # Double-buffered RGB array
├── main.py              # Entry point (reuse existing)
├── segment_manager.py   # Reuse from current code
├── text_renderer.py     # Reuse from current code
├── udp_handler.py       # Reuse from current code
├── web_server.py        # Reuse from current code
└── config.py            # Configuration
```

### Estimated Time to Working Display

- **Minimum (basic display)**: 3-4 hours
- **Full integration**: 6-8 hours
- **Optimized**: 10 hours

### Success Criteria

**Phase 1 Complete:**
- All 13 GPIOs can be toggled from Python
- No errors accessing GPIO chips

**Phase 2 Complete:**
- Solid color fills entire display
- Can change colors
- No flickering

**Phase 3 Complete:**
- Can display text rendered from Pillow
- Smooth updates
- 100+ Hz refresh rate

**Phase 4 Complete:**
- UDP commands work from Q-SYS
- Web UI functional
- All 4 segments working

**Phase 5 Complete:**
- Adjustable brightness
- Optimized refresh (200+ Hz)
- Stable long-term operation

### Next Steps

1. **Clean system**: Fresh Armbian or DietPi install
2. **Install dependencies**: gpiod, Python bindings, Pillow
3. **Disable UART0**: Free up pins 8 & 10
4. **Test GPIO access**: Simple toggle test
5. **Build driver**: Start with Phase 1

### Questions Before Starting

1. What OS image do you prefer? (Armbian minimal / DietPi)
2. Do you want me to implement this incrementally with testing at each phase?
3. Should we keep the existing ESP32 code as reference but build fresh for Rock Pi?

This approach is **much more likely to succeed** than fighting with the Pi library.
