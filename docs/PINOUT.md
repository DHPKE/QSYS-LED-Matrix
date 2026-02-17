# Olimex ESP32 Gateway LED Matrix Pinout

## Overview

This document provides detailed pin mapping and wiring information for connecting the Olimex ESP32 Gateway to a HUB75 LED matrix panel.

## Olimex ESP32 Gateway Pinout

```
                    Top View
        ┌───────────────────────────┐
        │  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○   │
        │                           │
        │   Olimex ESP32 Gateway    │
        │                           │
        │  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○   │
        └───────────────────────────┘
```

### GPIO Header Pinout (Top Row, Left to Right)

| Pin# | GPIO | Function | HUB75 Connection |
|------|------|----------|------------------|
| 1    | 3V3  | Power 3.3V | Not used |
| 2    | EN   | Reset | Not used |
| 3    | VP (36) | Input only | Not used |
| 4    | VN (39) | Input only | Not used |
| 5    | IO34 | Input only | Not used |
| 6    | IO35 | Input only | Not used |
| 7    | IO32 | GPIO | Not used (reserved) |
| 8    | IO33 | GPIO | Not used (reserved) |
| 9    | IO25 | GPIO | **OE (Output Enable)** |
| 10   | IO26 | GPIO | **LAT (Latch)** |
| 11   | IO27 | GPIO | **G2 (Green Lower)** |
| 12   | IO14 | GPIO | Not used |
| 13   | IO12 | GPIO | Not used |

### GPIO Header Pinout (Bottom Row, Left to Right)

| Pin# | GPIO | Function | HUB75 Connection |
|------|------|----------|------------------|
| 14   | GND  | Ground | **HUB75 GND** |
| 15   | IO13 | GPIO | Not used |
| 16   | IO9  | Flash | Not used (reserved) |
| 17   | IO10 | Flash | Not used (reserved) |
| 18   | IO11 | Flash | Not used (reserved) |
| 19   | IO6  | Flash | Not used (reserved) |
| 20   | IO7  | Flash | Not used (reserved) |
| 21   | IO8  | Flash | Not used (reserved) |
| 22   | IO15 | GPIO | **G1 (Green Upper)** |
| 23   | IO2  | GPIO | **R1 (Red Upper)** |
| 24   | IO0  | GPIO | Not used (reserved for boot) |
| 25   | IO4  | GPIO | **B1 (Blue Upper)** |
| 26   | IO16 | GPIO | **R2 (Red Lower)** |
| 27   | IO17 | GPIO | **B2 (Blue Lower)** |
| 28   | IO5  | GPIO | **A (Address)** |
| 29   | IO18 | GPIO | **B (Address)** |
| 30   | IO19 | GPIO | **C (Address)** |
| 31   | IO21 | GPIO | **D (Address)** |
| 32   | IO3  | UART RX | Not used |
| 33   | IO1  | UART TX | Not used |
| 34   | IO22 | GPIO | **CLK (Clock)** |
| 35   | IO23 | GPIO | Not used |

## HUB75 Connector Pinout

### HUB75 Interface Standard

```
Looking at the back of the LED matrix panel:

    ┌──────────────────────────────────────┐
    │  HUB75 Female Connector (on panel)   │
    ├──────────────────────────────────────┤
    │ R1  G1  B1 GND  R2  G2  B2  GND  │  ← Top row
    │  A   B   C   D   E CLK LAT  OE   │  ← Bottom row
    └──────────────────────────────────────┘
        ↑
    Cable plugs in here
```

### HUB75 Pin Functions

| Pin | Name | Description | Direction | Voltage |
|-----|------|-------------|-----------|---------|
| 1   | R1   | Red data - Upper half | Input | 3.3V |
| 2   | G1   | Green data - Upper half | Input | 3.3V |
| 3   | B1   | Blue data - Upper half | Input | 3.3V |
| 4   | GND  | Ground | - | 0V |
| 5   | R2   | Red data - Lower half | Input | 3.3V |
| 6   | G2   | Green data - Lower half | Input | 3.3V |
| 7   | B2   | Blue data - Lower half | Input | 3.3V |
| 8   | GND  | Ground | - | 0V |
| 9   | A    | Row address bit 0 | Input | 3.3V |
| 10  | B    | Row address bit 1 | Input | 3.3V |
| 11  | C    | Row address bit 2 | Input | 3.3V |
| 12  | D    | Row address bit 3 | Input | 3.3V |
| 13  | E    | Row address bit 4 | Input | 3.3V |
| 14  | CLK  | Clock signal | Input | 3.3V |
| 15  | LAT  | Latch/Strobe | Input | 3.3V |
| 16  | OE   | Output Enable (active LOW) | Input | 3.3V |

## Complete Pin Mapping

### Connection Table

| Olimex GPIO | Function | HUB75 Pin | Wire Color (typical) |
|-------------|----------|-----------|----------------------|
| **GPIO 2**  | R1       | Pin 1     | Red |
| **GPIO 15** | G1       | Pin 2     | Green |
| **GPIO 4**  | B1       | Pin 3     | Blue |
| **GPIO 16** | R2       | Pin 5     | Dark Red |
| **GPIO 27** | G2       | Pin 6     | Dark Green |
| **GPIO 17** | B2       | Pin 7     | Dark Blue |
| **GPIO 5**  | A        | Pin 9     | White/Gray 1 |
| **GPIO 18** | B        | Pin 10    | White/Gray 2 |
| **GPIO 19** | C        | Pin 11    | White/Gray 3 |
| **GPIO 21** | D        | Pin 12    | White/Gray 4 |
| -           | E        | Pin 13    | NOT CONNECTED (1/16 scan) |
| **GPIO 22** | CLK      | Pin 14    | Yellow |
| **GPIO 26** | LAT      | Pin 15    | Orange |
| **GPIO 25** | OE       | Pin 16    | Purple |
| **GND**     | GND      | Pins 4,8  | Black |

### Power Connections

**⚠️ CRITICAL: LED matrix MUST have external 5V power supply**

```
5V Power Supply                LED Matrix
┌──────────┐                  ┌──────────┐
│  +5V  ○─────────────────────○ 5V IN    │
│       │                     │          │
│  GND  ○─────────┬───────────○ GND      │
└──────────┘      │           └──────────┘
                  │
                  │
                  └───────────○ ESP32 GND
```

**Never connect**:
- ESP32 5V pin to matrix power
- ESP32 3.3V pin to matrix power
- Matrix 5V to ESP32 VIN (can damage ESP32 with high current)

## Wiring Color Code

While wire colors may vary, typical HUB75 cables use:

### Data Lines (RGB)
- **Red tones**: R1, R2
- **Green tones**: G1, G2
- **Blue tones**: B1, B2

### Address Lines
- **White/Gray**: A, B, C, D
- Sometimes numbered or in grayscale shades

### Control Lines
- **Yellow**: CLK (Clock)
- **Orange/Brown**: LAT (Latch)
- **Purple/Violet**: OE (Output Enable)

### Ground
- **Black**: GND (usually 2 wires)

## Physical Connection Methods

### Method 1: Direct IDC Cable Modification

1. Obtain spare HUB75 cable
2. Cut cable in middle
3. Strip wires on ESP32 side
4. Attach DuPont female connectors
5. Use multimeter to verify each wire
6. Label each wire with masking tape

### Method 2: Breakout Board (Recommended)

Use a HUB75 to pin header breakout board:
- Available on Amazon/eBay: "HUB75 breakout" or "HUB75 RGB Matrix Adapter"
- Provides labeled screw terminals or pin headers
- No cable cutting required
- Professional appearance
- Example: [HUB75 to DuPont Adapter Board]

### Method 3: Custom PCB

For permanent installation:
- Design PCB with ESP32 socket and HUB75 connector
- Can include level shifters (optional but recommended for 5V matrices)
- Add proper decoupling capacitors
- Professional solution for production

## Electrical Characteristics

### Voltage Levels

- ESP32 GPIO output: 3.3V logic
- HUB75 typical input: 3.3V or 5V tolerant
- Most modern HUB75 panels accept 3.3V logic
- Older panels may require level shifters (3.3V → 5V)

### Current Requirements

#### Per GPIO Pin:
- Maximum source/sink: 40mA (ESP32 spec)
- Recommended maximum: 12mA (per GPIO)
- Typical HUB75 input: 1-5mA (well within limits)

#### Matrix Power:
- Per panel at full white: 2-8A @ 5V (depends on pixel pitch and brightness)
- Per panel typical use: 0.5-2A @ 5V
- **Must use external power supply**

### Signal Integrity

#### Clock Frequency:
- Typical: 5-15 MHz
- Maximum: 20 MHz (ESP32 limit)
- DMA refresh handles all timing

#### Wire Length:
- Maximum recommended: 30cm (12 inches)
- Longer cables may require:
  - Termination resistors (100Ω)
  - Shielded cable for CLK line
  - Level shifters/buffers

## Level Shifters (Optional)

If your HUB75 panel requires 5V logic levels:

### When to Use:
- Panel datasheet specifies 5V logic
- Display shows dim or incorrect colors with 3.3V
- Long cable runs (>30cm)

### Recommended ICs:
- **74HCT245** - Octal bus transceiver (3 chips needed)
- **74AHCT125** - Quad buffer (4 chips needed)
- **TXS0108E** - 8-bit bidirectional level shifter module

### Connection with Level Shifter:

```
ESP32 GPIO → Level Shifter (3.3V side) → Level Shifter (5V side) → HUB75
     3.3V Power → Level Shifter VCC
                  5V Power → Level Shifter VCC_B
                            Common GND
```

## Testing Connections

### Continuity Test

Before powering on, use multimeter to verify:

1. **Pin-to-pin continuity**:
   - Beep test from ESP32 GPIO to HUB75 pin
   - Verify each of 13 signal connections

2. **No shorts**:
   - Test between adjacent pins
   - Test power to ground (should be open circuit)

3. **Ground connections**:
   - Verify common ground between ESP32 and matrix
   - Check power supply ground to matrix ground

### Visual Inspection Checklist

- [ ] All wires firmly connected
- [ ] No loose strands
- [ ] Correct pin positions (no off-by-one errors)
- [ ] Power polarity correct (+/-)
- [ ] No damaged wires or connectors

## Troubleshooting Connection Issues

### No Display

**Check**:
1. Power supply connected and on (measure 5V)
2. All ground connections secure
3. HUB75 cable in INPUT connector (not output)
4. R1, G1, B1 connections (upper half)

### Wrong Colors

**Check**:
1. R1/G1/B1 swapped
2. R2/G2/B2 swapped
3. Upper/lower half swapped (R1↔R2, etc.)

### Partial Display

**Check**:
1. Address lines (A, B, C, D)
2. If only top half works: check R2, G2, B2
3. If only bottom half works: check R1, G1, B1

### Flickering

**Check**:
1. CLK (clock) connection
2. LAT (latch) connection
3. Power supply capacity (try higher amperage)

### Dim Display

**Check**:
1. OE (output enable) connection
2. Brightness setting in software
3. Power supply voltage (should be 5.0V ±0.2V)

## Pinout Change for Different Matrices

### For 64x64 or other sizes:

The pin mapping remains the same, only the E (5th address line) may be needed:

- **64x32**: E not connected (1/16 scan)
- **64x64**: Connect E to available GPIO (1/32 scan)

Add to config.h if needed:
```cpp
#define E_PIN 23  // Use GPIO 23 for E line
```

### For P2.5 or higher density:

May require different scan rates, but same pin connections.

## Alternative Pin Mappings

If default pins conflict with other peripherals:

Edit `arduino/QSYS-LED-Matrix/config.h`:

```cpp
// Example alternative mapping
#define R1_PIN 13   // Changed from 2
#define G1_PIN 12   // Changed from 15
// ... etc
```

**Important**: Avoid these GPIOs:
- GPIO 0 (boot mode)
- GPIO 1, 3 (UART)
- GPIO 6-11 (flash)

## Schematic Diagram

```
Olimex ESP32 Gateway                         HUB75 LED Matrix
┌──────────────────┐                         ┌──────────────────┐
│                  │                         │                  │
│ GPIO2  ──────────┼─────────────────────────┼─ R1             │
│ GPIO15 ──────────┼─────────────────────────┼─ G1             │
│ GPIO4  ──────────┼─────────────────────────┼─ B1             │
│                  │                         │                  │
│ GPIO16 ──────────┼─────────────────────────┼─ R2             │
│ GPIO27 ──────────┼─────────────────────────┼─ G2             │
│ GPIO17 ──────────┼─────────────────────────┼─ B2             │
│                  │                         │                  │
│ GPIO5  ──────────┼─────────────────────────┼─ A              │
│ GPIO18 ──────────┼─────────────────────────┼─ B              │
│ GPIO19 ──────────┼─────────────────────────┼─ C              │
│ GPIO21 ──────────┼─────────────────────────┼─ D              │
│                  │                         │                  │
│ GPIO22 ──────────┼─────────────────────────┼─ CLK            │
│ GPIO26 ──────────┼─────────────────────────┼─ LAT            │
│ GPIO25 ──────────┼─────────────────────────┼─ OE             │
│                  │                         │                  │
│ GND    ──────────┼─────┬───────────────────┼─ GND            │
│                  │     │                   │                  │
└──────────────────┘     │                   │ 5V ─────────┐   │
                         │                   └─────────────┼───┘
                         │                                 │
                         │         5V Power Supply         │
                         │         ┌──────────┐            │
                         │         │ +5V (4A+)├────────────┘
                         │         │          │
                         └─────────┤ GND      │
                                   └──────────┘
```

## Documentation References

- **Olimex ESP32 Gateway**: [Official Documentation](https://www.olimex.com/Products/IoT/ESP32/ESP32-GATEWAY/)
- **HUB75 Standard**: Community documentation and datasheets
- **ESP32 Pinout**: [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)

---

**⚠️ Always double-check connections before powering on!**

**Last Updated**: 2026-02-16  
**Hardware Version**: Olimex ESP32-GATEWAY Rev. K
