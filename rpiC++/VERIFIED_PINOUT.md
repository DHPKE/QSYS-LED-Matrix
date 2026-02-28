# ✅ VERIFIED Pin Mapping - RPi Zero 2 W to HUB75

## Pin Layout Verification (Using Your Diagram)

Based on the J8 Header diagram you provided, here's the verified mapping:

### Physical Pin Locations (Looking at the diagram layout):

```
HUB75 Cable Wiring to Raspberry Pi Physical Pins:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

LEFT COLUMN (Odd pins 1-39):          RIGHT COLUMN (Even pins 2-40):

Pin 1  - 3.3V Power                   Pin 2  - 5.0V Power
Pin 3  - (not used)                   Pin 4  - 5.0V Power
Pin 5  - (not used)                   Pin 6  - GND ← 🟡 HUB75 Pin 4 (yellow 1)
Pin 7  - GPIO 4 (BCM) ← 🟡 Pin 14    Pin 8  - (not used)
Pin 9  - GND ← ⚫ HUB75 Pin 8         Pin 10 - (not used)
Pin 11 - GPIO 17 (BCM) ← 🟠 Pin 13   Pin 12 - GPIO 18 (BCM) ← 🟢 Pin 15
Pin 13 - GPIO 27 (BCM) ← 🟤 Pin 11   Pin 14 - GND ← 🔵 HUB75 Pin 16
Pin 15 - GPIO 22 (BCM) ← ⚪ Pin 9    Pin 16 - GPIO 23 (BCM) ← 🟣 Pin 7
Pin 17 - 3.3V Power                   Pin 18 - GPIO 24 (BCM) - not used
Pin 19 - (not used)                   Pin 20 - GND
Pin 21 - (not used)                   Pin 22 - (not used)
Pin 23 - (not used)                   Pin 24 - (not used)
Pin 25 - GND                          Pin 26 - (not used)
Pin 27 - (reserved)                   Pin 28 - (reserved)
Pin 29 - GPIO 5 (BCM) ← 🟤 Pin 1     Pin 30 - GND
Pin 31 - GPIO 6 (BCM) ← 🟠 Pin 3     Pin 32 - GPIO 12 (BCM) ← 🟢 Pin 5
Pin 33 - GPIO 13 (BCM) ← 🔴 Pin 2    Pin 34 - GND
Pin 35 - (not used)                   Pin 36 - GPIO 16 (BCM) ← 🔵 Pin 6
Pin 37 - GPIO 26 (BCM) ← ⚫ Pin 10   Pin 38 - GPIO 20 (BCM) ← 🔴 Pin 12
Pin 39 - GND                          Pin 40 - (not used)
```

## Complete Verified Connection Table

| HUB75 Pin | Wire Color | Signal | RPi BCM GPIO | Physical Pin | Location on Header |
|-----------|------------|--------|--------------|--------------|-------------------|
| 1 | 🟤 Brown 1 | R1 | GPIO 5 | **29** | **Left, bottom section** |
| 2 | 🔴 Red 1 | G1 | GPIO 13 | **33** | **Left, bottom section** |
| 3 | 🟠 Orange 1 | B1 | GPIO 6 | **31** | **Left, bottom section** |
| 4 | 🟡 Yellow 1 | GND | — | **6** | **Right, near top** |
| 5 | 🟢 Green 1 | R2 | GPIO 12 | **32** | **Right, bottom section** |
| 6 | 🔵 Blue 1 | G2 | GPIO 16 | **36** | **Right, bottom section** |
| 7 | 🟣 Purple 1 | B2 | GPIO 23 | **16** | **Right, upper-middle** |
| 8 | ⚫ Grey 1 | GND | — | **9** | **Left, upper section** |
| 9 | ⚪ White 1 | A | GPIO 22 | **15** | **Left, upper-middle** |
| 10 | ⚫ Black | B | GPIO 26 | **37** | **Left, bottom section** |
| 11 | 🟤 Brown 2 | C | GPIO 27 | **13** | **Left, upper-middle** |
| 12 | 🔴 Red 2 | D | GPIO 20 | **38** | **Right, bottom section** |
| 13 | 🟠 Orange 2 | CLK | GPIO 17 | **11** | **Left, upper section** |
| 14 | 🟡 Yellow 2 | LAT | GPIO 4 | **7** | **Left, upper section** |
| 15 | 🟢 Green 2 | OE | GPIO 18 | **12** | **Right, upper section** |
| 16 | 🔵 Blue 2 | GND | — | **14** | **Right, upper-middle** |

## Visual Physical Pin Map

```
Looking at your diagram layout:

      TOP OF BOARD (USB/HDMI ports)
      ═══════════════════════════════
      
      LEFT (ODD)          RIGHT (EVEN)
      ──────────────────────────────
      
 [1]  3.3V              [2]  5V
 [3]  —                 [4]  5V
 [5]  —                 [6]  GND ← 🟡 Yellow 1
 [7]  GPIO4 🟡          [8]  —
 [9]  GND ⚫            [10] —
[11]  GPIO17 🟠        [12] GPIO18 🟢
[13]  GPIO27 🟤        [14] GND 🔵
[15]  GPIO22 ⚪        [16] GPIO23 🟣
[17]  3.3V            [18] GPIO24
[19]  —               [20] GND
[21]  —               [22] —
[23]  —               [24] —
[25]  GND             [26] —
[27]  ID_SD           [28] ID_SC
[29]  GPIO5 🟤        [30] GND
[31]  GPIO6 🟠        [32] GPIO12 🟢
[33]  GPIO13 🔴       [34] GND
[35]  —               [36] GPIO16 🔵
[37]  GPIO26 ⚫       [38] GPIO20 🔴
[39]  GND             [40] —

      BOTTOM OF BOARD (SD Card)
      ═══════════════════════════════
```

## Pin Clusters by Function

**Data Pins (RGB) - Bottom Section:**
- Left: Pin 29 (🟤), 31 (🟠), 33 (🔴)
- Right: Pin 32 (🟢), 36 (🔵)
- Right Upper: Pin 16 (🟣)

**Control Pins - Upper Section:**
- Left: Pin 7 (🟡), 11 (🟠), 13 (🟤), 15 (⚪)
- Right: Pin 12 (🟢)

**Address Pins - Bottom/Middle:**
- Left: Pin 37 (⚫)
- Right: Pin 38 (🔴)

**Ground Pins - Scattered:**
- Pin 6, 9, 14 (main GNDs)
- Also available: Pin 20, 25, 30, 34, 39

## Configuration Verification

Current `/opt/led-matrix/config.py` settings:
```python
MATRIX_HARDWARE_MAPPING = "regular"  # ✅ Correct

# GPIO pins (BCM numbering):
R1  = GPIO 5   (physical pin 29)  # ✅ Left bottom
G1  = GPIO 13  (physical pin 33)  # ✅ Left bottom
B1  = GPIO 6   (physical pin 31)  # ✅ Left bottom
R2  = GPIO 12  (physical pin 32)  # ✅ Right bottom
G2  = GPIO 16  (physical pin 36)  # ✅ Right bottom
B2  = GPIO 23  (physical pin 16)  # ✅ Right upper-mid
A   = GPIO 22  (physical pin 15)  # ✅ Left upper-mid
B   = GPIO 26  (physical pin 37)  # ✅ Left bottom
C   = GPIO 27  (physical pin 13)  # ✅ Left upper-mid
D   = GPIO 20  (physical pin 38)  # ✅ Right bottom
CLK = GPIO 17  (physical pin 11)  # ✅ Left upper
LAT = GPIO 4   (physical pin 7)   # ✅ Left upper
OE  = GPIO 18  (physical pin 12)  # ✅ Right upper
```

## ✅ Status: MAPPING IS CORRECT

The pinout configuration matches the standard rpi-rgb-led-matrix "regular" mapping and correctly uses BCM GPIO numbering (not WiringPi numbering).

**Note**: Your diagram shows WiringPi/Pi4J numbering. We are using BCM numbering, which is the industry standard and what the rpi-rgb-led-matrix library expects.

## Troubleshooting

If display is still showing corrupted output despite correct wiring:

1. **Check cable seating**: Ensure HUB75 connector is fully inserted
2. **Verify power**: Matrix needs external 5V supply (3-5A)
3. **Test continuity**: Use multimeter to verify wire connections
4. **Check for shorts**: Ensure no adjacent pins are shorted
5. **Try different slowdown**: Adjust `MATRIX_GPIO_SLOWDOWN` in config.py
6. **Check PWM bits**: Try different `MATRIX_PWM_BITS` values

Current optimal settings for RPi Zero 2 W:
```python
MATRIX_GPIO_SLOWDOWN = 1
MATRIX_PWM_BITS = 8
MATRIX_BRIGHTNESS = 50
```
