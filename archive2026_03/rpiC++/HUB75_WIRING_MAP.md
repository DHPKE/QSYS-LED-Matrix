# HUB75 to Raspberry Pi Zero 2 W - Exact Wiring Map

## Your HUB75 Cable → RPi GPIO Mapping

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  HUB75 Pin → Wire Color → Signal → RPi GPIO → RPi Physical Pin   ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

Pin  1  🟤 brown 1   → R1  (Red Upper)      → GPIO  5 → RPi Pin 29
Pin  2  🔴 red 1     → G1  (Green Upper)    → GPIO 13 → RPi Pin 33
Pin  3  🟠 orange 1  → B1  (Blue Upper)     → GPIO  6 → RPi Pin 31
Pin  4  🟡 yellow 1  → GND (Ground)         → GND     → RPi Pin  6 ┐
                                                                    │
Pin  5  🟢 green 1   → R2  (Red Lower)      → GPIO 12 → RPi Pin 32 │
Pin  6  🔵 blue 1    → G2  (Green Lower)    → GPIO 16 → RPi Pin 36 │
Pin  7  🟣 purple 1  → B2  (Blue Lower)     → GPIO 23 → RPi Pin 16 │
Pin  8  ⚫ grey 1    → GND (Ground)         → GND     → RPi Pin  9 ┤ Any GND
                                                                    │
Pin  9  ⚪ white 1   → A   (Address Row 0)  → GPIO 22 → RPi Pin 15 │
Pin 10  ⚫ black     → B   (Address Row 1)  → GPIO 26 → RPi Pin 37 │
Pin 11  🟤 brown 2   → C   (Address Row 2)  → GPIO 27 → RPi Pin 13 │
Pin 12  🔴 red 2     → D   (Address Row 3)  → GPIO 20 → RPi Pin 38 │
                                                                    │
Pin 13  🟠 orange 2  → CLK (Clock Signal)   → GPIO 17 → RPi Pin 11 │
Pin 14  🟡 yellow 2  → LAT (Latch Signal)   → GPIO  4 → RPi Pin  7 │
Pin 15  🟢 green 2   → OE  (Output Enable)  → GPIO 18 → RPi Pin 12 │
Pin 16  🔵 blue 2    → GND (Ground)         → GND     → RPi Pin 14 ┘
```

## Step-by-Step Wiring Instructions

### Data Signals (RGB - Top Half)
```
HUB75 Pin 1  (🟤 brown 1)  → RPi GPIO  5 (Physical Pin 29)  [R1 - Red Upper]
HUB75 Pin 2  (🔴 red 1)    → RPi GPIO 13 (Physical Pin 33)  [G1 - Green Upper]
HUB75 Pin 3  (🟠 orange 1) → RPi GPIO  6 (Physical Pin 31)  [B1 - Blue Upper]
```

### Data Signals (RGB - Bottom Half)
```
HUB75 Pin 5  (🟢 green 1)  → RPi GPIO 12 (Physical Pin 32)  [R2 - Red Lower]
HUB75 Pin 6  (🔵 blue 1)   → RPi GPIO 16 (Physical Pin 36)  [G2 - Green Lower]
HUB75 Pin 7  (🟣 purple 1) → RPi GPIO 23 (Physical Pin 16)  [B2 - Blue Lower]
```

### Address Signals (Row Selection)
```
HUB75 Pin 9  (⚪ white 1)  → RPi GPIO 22 (Physical Pin 15)  [A - Address 0]
HUB75 Pin 10 (⚫ black)    → RPi GPIO 26 (Physical Pin 37)  [B - Address 1]
HUB75 Pin 11 (🟤 brown 2)  → RPi GPIO 27 (Physical Pin 13)  [C - Address 2]
HUB75 Pin 12 (🔴 red 2)    → RPi GPIO 20 (Physical Pin 38)  [D - Address 3]
```

### Control Signals (Timing)
```
HUB75 Pin 13 (🟠 orange 2) → RPi GPIO 17 (Physical Pin 11)  [CLK - Clock]
HUB75 Pin 14 (🟡 yellow 2) → RPi GPIO  4 (Physical Pin 7)   [LAT - Latch]
HUB75 Pin 15 (🟢 green 2)  → RPi GPIO 18 (Physical Pin 12)  [OE - Output Enable]
```

### Ground Connections
```
HUB75 Pin 4  (🟡 yellow 1) → RPi GND (Physical Pin 6)
HUB75 Pin 8  (⚫ grey 1)   → RPi GND (Physical Pin 9)
HUB75 Pin 16 (🔵 blue 2)   → RPi GND (Physical Pin 14)
```

## Visual Raspberry Pi Header Map

```
Raspberry Pi Zero 2 W - 40 Pin Header
(Looking at the board from above, GPIO header orientation)

         ┌─────────────────────────────────────┐
         │  ● ● ← USB port this side           │
    ┏━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┷━━━━┓
    ┃                                               ┃
    ┃    3V3  [ 1]  [ 2]  5V                       ┃
    ┃    GP2  [ 3]  [ 4]  5V                       ┃
    ┃    GP3  [ 5]  [ 6]  GND  ← 🟡 HUB75 Pin 4    ┃
    ┃    GP4  [ 7]  [ 8]  GP14 ← 🟡 Pin 14 (LAT)   ┃
    ┃    GND  [ 9]  [10]  GP15 ← ⚫ Pin 8 (GND)    ┃
    ┃   GP17  [11]  [12]  GP18 ← 🟠 Pin 13, 🟢 15  ┃
    ┃   GP27  [13]  [14]  GND  ← 🟤 Pin 11, 🔵 16  ┃
    ┃   GP22  [15]  [16]  GP23 ← ⚪ Pin 9, 🟣 Pin 7┃
    ┃    3V3  [17]  [18]  GP24                     ┃
    ┃   GP10  [19]  [20]  GND                      ┃
    ┃    GP9  [21]  [22]  GP25                     ┃
    ┃   GP11  [23]  [24]  GP8                      ┃
    ┃    GND  [25]  [26]  GP7                      ┃
    ┃  ID_SD  [27]  [28]  ID_SC                    ┃
    ┃    GP5  [29]  [30]  GND  ← 🟤 Pin 1 (R1)     ┃
    ┃    GP6  [31]  [32]  GP12 ← 🟠 Pin 3, 🟢 Pin 5┃
    ┃   GP13  [33]  [34]  GND  ← 🔴 Pin 2 (G1)     ┃
    ┃   GP19  [35]  [36]  GP16 ← 🔵 Pin 6 (G2)     ┃
    ┃   GP26  [37]  [38]  GP20 ← ⚫ Pin 10, 🔴 12  ┃
    ┃    GND  [39]  [40]  GP21                     ┃
    ┃                                               ┃
    ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

## Quick Reference Table

| HUB75 | Wire Color | Signal | Function | RPi GPIO | RPi Pin |
|-------|------------|--------|----------|----------|---------|
| 1 | 🟤 Brown 1 | R1 | Red Top | **GPIO 5** | **Pin 29** |
| 2 | 🔴 Red 1 | G1 | Green Top | **GPIO 13** | **Pin 33** |
| 3 | 🟠 Orange 1 | B1 | Blue Top | **GPIO 6** | **Pin 31** |
| 4 | 🟡 Yellow 1 | GND | Ground | GND | Pin 6 |
| 5 | 🟢 Green 1 | R2 | Red Bottom | **GPIO 12** | **Pin 32** |
| 6 | 🔵 Blue 1 | G2 | Green Bottom | **GPIO 16** | **Pin 36** |
| 7 | 🟣 Purple 1 | B2 | Blue Bottom | **GPIO 23** | **Pin 16** |
| 8 | ⚫ Grey 1 | GND | Ground | GND | Pin 9 |
| 9 | ⚪ White 1 | A | Row Addr 0 | **GPIO 22** | **Pin 15** |
| 10 | ⚫ Black | B | Row Addr 1 | **GPIO 26** | **Pin 37** |
| 11 | 🟤 Brown 2 | C | Row Addr 2 | **GPIO 27** | **Pin 13** |
| 12 | 🔴 Red 2 | D | Row Addr 3 | **GPIO 20** | **Pin 38** |
| 13 | 🟠 Orange 2 | CLK | Clock | **GPIO 17** | **Pin 11** |
| 14 | 🟡 Yellow 2 | LAT | Latch | **GPIO 4** | **Pin 7** |
| 15 | 🟢 Green 2 | OE | Output En. | **GPIO 18** | **Pin 12** |
| 16 | 🔵 Blue 2 | GND | Ground | GND | Pin 14 |

## Wiring Checklist

- [ ] Pin 1 (brown 1) → GPIO 5 (Pin 29) - R1
- [ ] Pin 2 (red 1) → GPIO 13 (Pin 33) - G1
- [ ] Pin 3 (orange 1) → GPIO 6 (Pin 31) - B1
- [ ] Pin 4 (yellow 1) → GND (Pin 6)
- [ ] Pin 5 (green 1) → GPIO 12 (Pin 32) - R2
- [ ] Pin 6 (blue 1) → GPIO 16 (Pin 36) - G2
- [ ] Pin 7 (purple 1) → GPIO 23 (Pin 16) - B2
- [ ] Pin 8 (grey 1) → GND (Pin 9)
- [ ] Pin 9 (white 1) → GPIO 22 (Pin 15) - A
- [ ] Pin 10 (black) → GPIO 26 (Pin 37) - B
- [ ] Pin 11 (brown 2) → GPIO 27 (Pin 13) - C
- [ ] Pin 12 (red 2) → GPIO 20 (Pin 38) - D
- [ ] Pin 13 (orange 2) → GPIO 17 (Pin 11) - CLK
- [ ] Pin 14 (yellow 2) → GPIO 4 (Pin 7) - LAT
- [ ] Pin 15 (green 2) → GPIO 18 (Pin 12) - OE
- [ ] Pin 16 (blue 2) → GND (Pin 14)

## Notes

⚠️ **Important**: The wire colors from your HUB75 cable do NOT match the typical signal colors. For example:
- Your "red 1" wire (Pin 2) is actually carrying **GREEN** data (G1), not red!
- Your "brown 1" wire (Pin 1) is carrying **RED** data (R1)

This is normal - cable manufacturers use sequential color coding for easy identification, not signal-based colors.

🔌 **Power**: Remember to power the LED matrix with a separate 5V supply (3-5A), NOT from the Pi's 5V pins.

🧪 **Testing**: After wiring, the IP address (10.1.1.10) should display clearly on boot. If corrupted, double-check all connections against this map.
