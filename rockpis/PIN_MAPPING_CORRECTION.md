# Rock Pi S HUB75 Wiring - CORRECTED PIN MAPPING

**IMPORTANT**: This document corrects the pin mapping based on official Radxa Rock Pi S documentation.

## Issue Found
The previous pinout documentation had incorrect physical pin numbers. The Rock Pi S GPIO header does NOT match Raspberry Pi pinout!

## Official Rock Pi S Header 1 Layout (from Radxa docs)

```
      3.3V  [ 1] [ 2]  5V
  GPIO0_B3  [ 3] [ 4]  5V
  GPIO0_B4  [ 5] [ 6]  GND
  GPIO2_A4  [ 7] [ 8]  GPIO2_A1 (UART0_TX)
       GND  [ 9] [10]  GPIO2_A0 (UART0_RX)
  GPIO0_B7  [11] [12]  GPIO2_A5
  GPIO0_C0  [13] [14]  GND
  GPIO0_C1  [15] [16]  GPIO2_B2
      3.3V  [17] [18]  GPIO2_B1
  GPIO1_C7  [19] [20]  GND
  GPIO1_C6  [21] [22]  GPIO2_A7
  GPIO1_D0  [23] [24]  GPIO1_D1
       GND  [25] [26]  ADC_IN0
```

## config.py GPIO Assignments vs Physical Pins

| config.py     | Linux GPIO# | Rockchip Name | Actual Physical Pin | Available on Header 1? |
|---------------|-------------|---------------|---------------------|------------------------|
| `GPIO_R1=16`  | 16          | GPIO0_C0      | 13                  | ✅ YES                 |
| `GPIO_G1=17`  | 17          | GPIO0_C1      | 15                  | ✅ YES                 |
| `GPIO_B1=18`  | 18          | GPIO0_C2      | ❌ NOT on Header 1  | ❌ NO (need Header 2)  |
| `GPIO_R2=19`  | 19          | GPIO0_C3      | ❌ NOT on Header 1  | ❌ NO (need Header 2)  |
| `GPIO_G2=20`  | 20          | GPIO0_C4      | ❌ NOT on Header 1  | ❌ NO (need Header 2)  |
| `GPIO_B2=21`  | 21          | GPIO0_C5      | ❌ NOT on Header 1  | ❌ NO (need Header 2)  |
| `GPIO_A=11`   | 11          | GPIO0_B3      | 3                   | ✅ YES                 |
| `GPIO_B=12`   | 12          | GPIO0_B4      | 5                   | ✅ YES                 |
| `GPIO_C=13`   | 13          | GPIO0_B5      | ❌ NOT on Header 1  | ❌ NO                  |
| `GPIO_D=14`   | 14          | GPIO0_B6      | ❌ NOT on Header 1  | ❌ NO                  |
| `GPIO_CLK=22` | 22          | GPIO0_C6      | ❌ NOT on Header 1  | ❌ NO (need Header 2)  |
| `GPIO_LAT=23` | 23          | GPIO0_C7      | ❌ NOT on Header 1  | ❌ NO (need Header 2)  |
| `GPIO_OE=24`  | 24          | GPIO0_D0      | ❌ NOT on Header 1  | ❌ NO (need Header 2)  |

## Problem

**The config.py GPIO assignments assume pins that are NOT all available on Header 1!**

Many of the required GPIO pins (GPIO0_C2, GPIO0_C3, GPIO0_C4, GPIO0_C5, GPIO0_C6, GPIO0_C7, GPIO0_D0) are NOT exposed on Header 1.

## Solution Options

### Option 1: Use BOTH Headers (Complex Wiring)
Wire to pins across both 26-pin headers. This requires finding where each GPIO is located.

### Option 2: Reconfigure to Use Only Header 1 Pins (RECOMMENDED)
Update `config.py` to use ONLY the GPIOs available on Header 1:

```python
# Available on Header 1:
GPIO_R1  = 16   # GPIO0_C0   pin 13
GPIO_G1  = 17   # GPIO0_C1   pin 15  
GPIO_B1  = 15   # GPIO0_B7   pin 11   # CHANGED!
GPIO_R2  = 68   # GPIO2_A4   pin 7    # CHANGED!
GPIO_G2  = 69   # GPIO2_A5   pin 12   # CHANGED!
GPIO_B2  = 74   # GPIO2_B2   pin 16   # CHANGED!
GPIO_A   = 11   # GPIO0_B3   pin 3
GPIO_B   = 12   # GPIO0_B4   pin 5
GPIO_C   = 65   # GPIO2_A1   pin 8    # CHANGED! (UART0_TX - disable console!)
GPIO_D   = 64   # GPIO2_A0   pin 10   # CHANGED! (UART0_RX - disable console!)
GPIO_CLK = 71   # GPIO2_A7   pin 22   # CHANGED!
GPIO_LAT = 55   # GPIO1_C7   pin 19   # CHANGED!
GPIO_OE  = 54   # GPIO1_C6   pin 21   # CHANGED!
```

**Linux GPIO Number Calculation for RK3308:**
- `GPIOx_Ay` = x*32 + y (where A=0-7)
- `GPIOx_By` = x*32 + 8 + y (where B=8-15)
- `GPIOx_Cy` = x*32 + 16 + y (where C=16-23)
- `GPIOx_Dy` = x*32 + 24 + y (where D=24-31)

Examples:
- GPIO2_A1 = 2*32 + 1 = 65
- GPIO2_A7 = 2*32 + 7 = 71
- GPIO1_C7 = 1*32 + 23 = 55

### Option 3: Find Physical Pinout for GPIOs in config.py
Check if your Rock Pi S model exposes GPIO0_C2-C7 and GPIO0_D0 somewhere (possibly Header 2 or test pads).

## Recommendation

**You need to physically check which GPIO header you're actually using**, then either:

1. Update config.py GPIO numbers to match Header 1 pins (Option 2 above)
2. Verify your wiring spans both headers and document which header each wire connects to

The current configuration expects pins that may not all be accessible on a single header!
