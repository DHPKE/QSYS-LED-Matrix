# Rock Pi S → HUB75 LED Matrix - CORRECTED PINOUT
## Based on Official Radxa Rock Pi S GPIO Header 1

═══════════════════════════════════════════════════════════════════════════════

## Rock Pi S Header 1 - Official Pinout (from Radxa documentation)

```
Pin#   GPIO Name     Function          | Pin#   GPIO Name     Function
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 1     +3.3V                          |  2     +5.0V
 3     GPIO0_B3      I2C1_SDA         |  4     +5.0V
 5     GPIO0_B4      I2C1_SCL         |  6     GND
 7     GPIO2_A4      I2S0_8CH_MCLK    |  8     GPIO2_A1     UART0_TX ⚠
 9     GND                            | 10     GPIO2_A0     UART0_RX ⚠
11     GPIO0_B7      PWM2             | 12     GPIO2_A5     I2S0_8CH_SCLK_TX
13     GPIO0_C0      PWM3             | 14     GND
15     GPIO0_C1      SPDIF_TX         | 16     GPIO2_B2     I2S0_8CH_SDO1
17     +3.3V                          | 18     GPIO2_B1     I2S0_8CH_SDO0
19     GPIO1_C7      UART2_TX/UART1_RTSN | 20  GND
21     GPIO1_C6      UART2_RX/UART1_CTSN | 22  GPIO2_A7     I2S0_8CH_LRCK_TX
23     GPIO1_D0      UART1_RX/I2C0_SDA   | 24  GPIO1_D1     UART1_TX/I2C0_SCL
25     GND                            | 26     ADC_IN0
```

⚠ **Pins 8 & 10 are UART0_TX/RX - Must disable serial console!**

═══════════════════════════════════════════════════════════════════════════════

## Linux GPIO Number Calculation (RK3308)

**Formula**: `GPIOx_Py` = bank×32 + offset

Where offset encoding is:
- A0-A7 = 0-7
- B0-B7 = 8-15  
- C0-C7 = 16-23
- D0-D7 = 24-31

**Examples from Header 1:**
- GPIO0_B3 = 0×32 + 11 = **11**
- GPIO0_B4 = 0×32 + 12 = **12**
- GPIO0_B7 = 0×32 + 15 = **15**
- GPIO0_C0 = 0×32 + 16 = **16**
- GPIO0_C1 = 0×32 + 17 = **17**
- GPIO2_A0 = 2×32 + 0  = **64**
- GPIO2_A1 = 2×32 + 1  = **65**
- GPIO2_A4 = 2×32 + 4  = **68**
- GPIO2_A5 = 2×32 + 5  = **69**
- GPIO2_A7 = 2×32 + 7  = **71**
- GPIO2_B1 = 2×32 + 9  = **73**
- GPIO2_B2 = 2×32 + 10 = **74**
- GPIO1_C6 = 1×32 + 22 = **54**
- GPIO1_C7 = 1×32 + 23 = **55**
- GPIO1_D0 = 1×32 + 24 = **56**
- GPIO1_D1 = 1×32 + 25 = **57**

═══════════════════════════════════════════════════════════════════════════════

## Current config.py Assignments → Actual Pin Locations

| config.py     | Linux# | GPIO Name | Pin# (Header 1) | Available? |
|---------------|--------|-----------|-----------------|------------|
| `GPIO_R1=16`  | 16     | GPIO0_C0  | **13** ✓        | ✅ YES     |
| `GPIO_G1=17`  | 17     | GPIO0_C1  | **15** ✓        | ✅ YES     |
| `GPIO_B1=18`  | 18     | GPIO0_C2  | ❌ Not on H1    | ❌ NO      |
| `GPIO_R2=19`  | 19     | GPIO0_C3  | ❌ Not on H1    | ❌ NO      |
| `GPIO_G2=20`  | 20     | GPIO0_C4  | ❌ Not on H1    | ❌ NO      |
| `GPIO_B2=21`  | 21     | GPIO0_C5  | ❌ Not on H1    | ❌ NO      |
| `GPIO_A=11`   | 11     | GPIO0_B3  | **3** ✓         | ✅ YES     |
| `GPIO_B=12`   | 12     | GPIO0_B4  | **5** ✓         | ✅ YES     |
| `GPIO_C=13`   | 13     | GPIO0_B5  | ❌ Not on H1    | ❌ NO      |
| `GPIO_D=14`   | 14     | GPIO0_B6  | ❌ Not on H1    | ❌ NO      |
| `GPIO_CLK=22` | 22     | GPIO0_C6  | ❌ Not on H1    | ❌ NO      |
| `GPIO_LAT=23` | 23     | GPIO0_C7  | ❌ Not on H1    | ❌ NO      |
| `GPIO_OE=24`  | 24     | GPIO0_D0  | ❌ Not on H1    | ❌ NO      |

**Result**: Only 5 of 13 required pins are on Header 1! ❌

═══════════════════════════════════════════════════════════════════════════════

## SOLUTION: Reconfigure config.py for Header 1 Pins ONLY

Replace the GPIO assignments in `/opt/led-matrix/config.py`:

```python
# ── HUB75 GPIO assignments (Linux sysfs / libgpiod numbers) ──
# Updated to use ONLY Header 1 pins (all signals available)

GPIO_R1  = 16   # GPIO0_C0   physical pin 13
GPIO_G1  = 17   # GPIO0_C1   physical pin 15
GPIO_B1  = 15   # GPIO0_B7   physical pin 11   ← CHANGED
GPIO_R2  = 68   # GPIO2_A4   physical pin 7    ← CHANGED
GPIO_G2  = 69   # GPIO2_A5   physical pin 12   ← CHANGED
GPIO_B2  = 74   # GPIO2_B2   physical pin 16   ← CHANGED
GPIO_A   = 11   # GPIO0_B3   physical pin 3
GPIO_B   = 12   # GPIO0_B4   physical pin 5
GPIO_C   = 65   # GPIO2_A1   physical pin 8    ← CHANGED (UART0_TX!)
GPIO_D   = 64   # GPIO2_A0   physical pin 10   ← CHANGED (UART0_RX!)
GPIO_CLK = 71   # GPIO2_A7   physical pin 22   ← CHANGED
GPIO_LAT = 55   # GPIO1_C7   physical pin 19   ← CHANGED
GPIO_OE  = 54   # GPIO1_C6   physical pin 21   ← CHANGED
```

⚠ **CRITICAL**: Pins 8 & 10 are UART0_TX/RX. You MUST disable serial console!

═══════════════════════════════════════════════════════════════════════════════

## Complete Wiring Table (Header 1 Only - CORRECTED)

┌────────────┬──────────────┬────────┬──────────────┬─────────────────────────┐
│ HUB75      │ Rock Pi S    │ Linux  │ Physical Pin │ Notes                   │
│ Signal     │ GPIO Name    │ GPIO#  │ (Header 1)   │                         │
├────────────┼──────────────┼────────┼──────────────┼─────────────────────────┤
│ R1         │ GPIO0_C0     │   16   │     13       │ Upper half - Red        │
│ G1         │ GPIO0_C1     │   17   │     15       │ Upper half - Green      │
│ B1         │ GPIO0_B7     │   15   │     11       │ Upper half - Blue       │
├────────────┼──────────────┼────────┼──────────────┼─────────────────────────┤
│ R2         │ GPIO2_A4     │   68   │      7       │ Lower half - Red        │
│ G2         │ GPIO2_A5     │   69   │     12       │ Lower half - Green      │
│ B2         │ GPIO2_B2     │   74   │     16       │ Lower half - Blue       │
├────────────┼──────────────┼────────┼──────────────┼─────────────────────────┤
│ A          │ GPIO0_B3     │   11   │      3       │ Row address bit 0       │
│ B          │ GPIO0_B4     │   12   │      5       │ Row address bit 1       │
│ C          │ GPIO2_A1     │   65   │      8       │ Row addr bit 2 ⚠ UART TX│
│ D          │ GPIO2_A0     │   64   │     10       │ Row addr bit 3 ⚠ UART RX│
├────────────┼──────────────┼────────┼──────────────┼─────────────────────────┤
│ CLK        │ GPIO2_A7     │   71   │     22       │ Shift clock             │
│ LAT        │ GPIO1_C7     │   55   │     19       │ Latch / Strobe          │
│ OE         │ GPIO1_C6     │   54   │     21       │ Output Enable (active-L)│
├────────────┼──────────────┼────────┼──────────────┼─────────────────────────┤
│ GND        │ GND          │   —    │ 6,9,14,20,25 │ Common ground (any pin) │
└────────────┴──────────────┴────────┴──────────────┴─────────────────────────┘

═══════════════════════════════════════════════════════════════════════════════

## Quick Visual Reference

```
Rock Pi S Header 1 → HUB75 Panel

Pin  3  GPIO0_B3 (11)  ────────→  A    (address bit 0)
Pin  5  GPIO0_B4 (12)  ────────→  B    (address bit 1)
Pin  7  GPIO2_A4 (68)  ────────→  R2   (lower red)
Pin  8  GPIO2_A1 (65)  ────────→  C    (address bit 2) ⚠ UART!
Pin 10  GPIO2_A0 (64)  ────────→  D    (address bit 3) ⚠ UART!
Pin 11  GPIO0_B7 (15)  ────────→  B1   (upper blue)
Pin 12  GPIO2_A5 (69)  ────────→  G2   (lower green)
Pin 13  GPIO0_C0 (16)  ────────→  R1   (upper red)
Pin 15  GPIO0_C1 (17)  ────────→  G1   (upper green)
Pin 16  GPIO2_B2 (74)  ────────→  B2   (lower blue)
Pin 19  GPIO1_C7 (55)  ────────→  LAT  (latch)
Pin 21  GPIO1_C6 (54)  ────────→  OE   (output enable)
Pin 22  GPIO2_A7 (71)  ────────→  CLK  (clock)
Pin 6,9,14,20,25 GND   ────────→  GND  (ground, connect at least one)
```

═══════════════════════════════════════════════════════════════════════════════

## Action Required

1. **Update config.py** with the corrected GPIO numbers above
2. **Verify UART0 is disabled** (pins 8 & 10 conflict)
3. **Update and restart** the service:
   ```bash
   sudo systemctl stop led-matrix
   # Edit /opt/led-matrix/config.py with new GPIO numbers
   sudo systemctl start led-matrix
   sudo journalctl -u led-matrix -f
   ```

This mapping uses ALL pins from Header 1 only - no need for Header 2!

═══════════════════════════════════════════════════════════════════════════════
