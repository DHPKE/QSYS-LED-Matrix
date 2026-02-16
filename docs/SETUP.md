# Hardware Setup Guide

## Overview

This guide covers the complete hardware setup for the Olimex ESP32 Gateway LED Matrix Text Display system.

## Required Hardware

### Main Components

1. **Olimex ESP32-GATEWAY**
   - ESP32-based board with Ethernet
   - Price: ~$25-30
   - Purchase: [Olimex](https://www.olimex.com/Products/IoT/ESP32/ESP32-GATEWAY/)

2. **64x32 HUB75 LED Matrix Panel**
   - Indoor P3, P4, P5, or P6 pitch
   - 64 pixels wide × 32 pixels tall
   - HUB75 (or HUB75E) interface
   - Price: $20-40 depending on pitch
   - Purchase: Amazon, AliExpress, Adafruit

3. **5V Power Supply**
   - Output: 5V DC
   - Current: Minimum 4A (10A recommended for full brightness)
   - Connector: Depends on matrix panel (usually screw terminals)
   - **Critical**: Must be external, do NOT power from ESP32

### Cables and Connectors

1. **HUB75 IDC Cable**
   - Usually included with LED matrix
   - 16-pin ribbon cable with 2×8 IDC connectors
   - Length: 15-20cm typical

2. **Jumper Wires**
   - Female-to-female DuPont cables
   - Quantity: ~20 wires
   - For connecting ESP32 GPIOs to HUB75 connector

3. **USB Cable**
   - Micro-USB for programming Olimex Gateway
   - USB-A to Micro-USB

### Optional Components

1. **Ethernet Cable** (if using wired connection)
2. **Mounting Hardware** (screws, standoffs)
3. **Enclosure** (for clean installation)
4. **Level Shifter** (optional, for 5V logic - usually not needed)

## Wiring Diagram

### Olimex ESP32 Gateway to HUB75 Matrix

```
                    Olimex ESP32 Gateway
                    ┌─────────────────┐
                    │                 │
         GPIO 2  ───┤ D2          GND ├─── GND
         GPIO 15 ───┤ D15         3V3 │
         GPIO 4  ───┤ D4          EN  │
         GPIO 16 ───┤ D16         VP  │
         GPIO 27 ───┤ D27         VN  │
         GPIO 17 ───┤ D17         D34 │
         GPIO 5  ───┤ D5          D35 │
         GPIO 18 ───┤ D18         D32 │
         GPIO 19 ───┤ D19         D33 │
         GPIO 21 ───┤ D21         D25 ├─── GPIO 25 (OE)
         GPIO 26 ───┤ D26         D26 │
         GPIO 22 ───┤ D22         D27 │
                    │ D23         TX  │
                    │ GND         RX  │
                    │ 5V          D13 │
                    └─────────────────┘
                    
                    
                    HUB75 Connector (Looking at matrix back)
                    ┌─────────────────────────────────┐
                    │  R1  G1  B1 GND  R2  G2  B2 GND │  ← Top Row
                    │   A   B   C   D   E  CLK LAT  OE│  ← Bottom Row
                    └─────────────────────────────────┘
```

### Complete Pin Mapping Table

| ESP32 GPIO | Function | HUB75 Pin | Description |
|------------|----------|-----------|-------------|
| GPIO 2     | R1       | R1        | Red data (upper half) |
| GPIO 15    | G1       | G1        | Green data (upper half) |
| GPIO 4     | B1       | B1        | Blue data (upper half) |
| GPIO 16    | R2       | R2        | Red data (lower half) |
| GPIO 27    | G2       | G2        | Green data (lower half) |
| GPIO 17    | B2       | B2        | Blue data (lower half) |
| GPIO 5     | A        | A         | Row address bit 0 |
| GPIO 18    | B        | B         | Row address bit 1 |
| GPIO 19    | C        | C         | Row address bit 2 |
| GPIO 21    | D        | D         | Row address bit 3 |
| -          | E        | E         | Row address bit 4 (not used for 1/16 scan) |
| GPIO 26    | LAT      | LAT       | Latch/Strobe |
| GPIO 25    | OE       | OE        | Output Enable (active low) |
| GPIO 22    | CLK      | CLK       | Clock signal |
| GND        | GND      | GND       | Ground (connect to power supply GND too) |

### Power Connections

**⚠️ CRITICAL: Never power the LED matrix from the ESP32's 5V pin!**

```
    5V Power Supply (4-10A)
         ┌──────┐
    +5V ─┤  +   │
         │      │
    GND ─┤  -   │
         └──────┘
           │  │
           │  └──────────→ Matrix 5V Input (red wire)
           │
           └──────────────→ Matrix GND (black wire)
                              │
                              └────→ ESP32 GND (common ground)
```

## Step-by-Step Assembly

### Step 1: Prepare Components

1. Unpack all components
2. Check LED matrix for any physical damage
3. Verify ESP32 Gateway powers on (connect USB, check LEDs)
4. Test power supply output voltage (should be 5.0V ±0.2V)

### Step 2: Create HUB75 Breakout (Method 1 - Direct Wiring)

If you don't have a HUB75 breakout board:

1. Take the HUB75 cable that came with the matrix
2. Cut it in half, or use a spare connector
3. Strip the wires on one end (IDC connector remains on matrix side)
4. Identify each wire using a multimeter and the pinout above
5. Attach female DuPont connectors to each wire

**Wire Color Reference** (varies by manufacturer):
- Red/Brown shades: Usually R1, R2
- Green shades: Usually G1, G2
- Blue shades: Usually B1, B2
- Black: Ground
- White/Gray: Address lines (A, B, C, D)
- Yellow: CLK
- Orange: LAT
- Purple: OE

### Step 3: Create HUB75 Breakout (Method 2 - PCB Adapter)

Recommended for clean installation:

1. Use a HUB75 to DuPont adapter PCB
2. Available on Amazon/eBay: "HUB75 breakout board"
3. Solder pin headers
4. Connect IDC cable from matrix to adapter
5. Connect DuPont wires from adapter to ESP32

### Step 4: Connect Data Lines

**Work with power disconnected from both devices!**

Connect GPIO pins to HUB75 according to the pin mapping table:

1. **Color Data Lines** (most critical):
   ```
   GPIO 2  → R1 (Red upper)
   GPIO 15 → G1 (Green upper)
   GPIO 4  → B1 (Blue upper)
   GPIO 16 → R2 (Red lower)
   GPIO 27 → G2 (Green lower)
   GPIO 17 → B2 (Blue lower)
   ```

2. **Address Lines** (row selection):
   ```
   GPIO 5  → A
   GPIO 18 → B
   GPIO 19 → C
   GPIO 21 → D
   (E not connected for 32px height)
   ```

3. **Control Lines** (timing):
   ```
   GPIO 26 → LAT (Latch)
   GPIO 25 → OE (Output Enable)
   GPIO 22 → CLK (Clock)
   ```

4. **Ground Connection**:
   ```
   ESP32 GND → HUB75 GND (multiple GND pins on HUB75)
   ```

### Step 5: Connect Power

1. **Matrix Power**:
   - Connect 5V power supply **positive** to matrix 5V input (red wire/connector)
   - Connect 5V power supply **ground** to matrix GND (black wire/connector)

2. **Common Ground**:
   - Connect power supply GND to ESP32 GND
   - This creates a common ground reference for signals

3. **ESP32 Power**:
   - Power ESP32 via USB (for programming)
   - Or use separate 5V to Micro-USB cable
   - Or use ESP32's 5V pin (if power supply has enough capacity)

### Step 6: Verify Connections

Before powering on:

1. **Visual Inspection**:
   - Check all wires are firmly connected
   - Verify no short circuits
   - Ensure correct pin mapping

2. **Continuity Test** (with multimeter):
   - Test each data line from ESP32 GPIO to HUB75 pin
   - Verify all GND connections
   - Check no shorts between adjacent pins

3. **Double-Check Power**:
   - Verify power supply is 5V (not 12V!)
   - Check polarity (+ and -)
   - Ensure ESP32 is NOT connected to matrix 5V

## First Power-On

### Safety Checklist

- [ ] All data connections verified
- [ ] Common ground established
- [ ] Power supply is 5V
- [ ] ESP32 not powered from matrix
- [ ] No visible short circuits
- [ ] Firmware uploaded to ESP32

### Power-On Sequence

1. **First**: Connect USB to ESP32 (powers ESP32 only)
2. **Second**: Open Serial Monitor (115200 baud)
3. **Third**: Plug in matrix 5V power supply
4. **Observe**: Matrix should initialize and display "READY"

### Expected Behavior

✅ **Success Indicators**:
- Serial monitor shows initialization messages
- Matrix displays "READY" in green
- No flickering or dimming
- ESP32 reports IP address

❌ **Problem Indicators**:
- Matrix completely dark → Check power connections
- Random pixels/garbage → Check data line connections
- Flickering → Check clock and latch lines
- ESP32 crashes/reboots → Check for short circuits
- Only one color → Check R1/G1/B1 or R2/G2/B2 connections

## Testing

### Basic Display Test

1. Access web interface at ESP32's IP address
2. Enter text in Segment 0: "TEST"
3. Click "Send"
4. Should display centered on matrix

### UDP Test

From command line:
```bash
echo "TEXT|0|Hello|FFFFFF|roboto12|auto|C|none" | nc -u -w1 [ESP32_IP] 21324
```

### Color Test

Test each color channel:
```
TEXT|0|RED|FF0000|roboto24|auto|C|none
TEXT|0|GREEN|00FF00|roboto24|auto|C|none
TEXT|0|BLUE|0000FF|FF0000|roboto24|auto|C|none
```

## Troubleshooting Hardware Issues

### Matrix doesn't light up at all

1. **Check power supply**:
   - Verify 5V output with multimeter
   - Check connections are tight
   - Try different power cable

2. **Check matrix input connector**:
   - Some matrices have IN and OUT connectors
   - Ensure cable is connected to IN side
   - Try reseating the cable

3. **Test matrix independently**:
   - Some matrices have a test mode (check manual)
   - Or use a different controller to verify matrix works

### Partial display or missing colors

1. **Check data lines**:
   - Test continuity of R1, G1, B1, R2, G2, B2
   - Reconnect any loose wires
   - Verify correct GPIO assignments in config.h

2. **Check address lines**:
   - Test A, B, C, D connections
   - If only top or bottom half works, check R2/G2/B2

### Flickering or unstable display

1. **Power issues**:
   - Power supply may be undersized (try higher amperage)
   - Check for voltage drop on long power cables
   - Add bulk capacitor (1000µF) near matrix power input

2. **Signal integrity**:
   - Shorten jumper wires if possible
   - Use shielded cable for clock line
   - Add 100Ω resistors in series with clock/data lines

3. **Software settings**:
   - Reduce brightness in software
   - Adjust refresh rate in code if needed

### ESP32 keeps crashing

1. **Check for shorts**:
   - Verify no GPIO pins shorted together
   - Check no shorts to ground or power

2. **Power supply**:
   - Matrix may be causing voltage drop
   - Use separate power for ESP32
   - Add bulk capacitor on ESP32 power input

3. **GPIO conflicts**:
   - Verify no GPIO pin conflicts with Olimex onboard peripherals
   - Check config.h matches your wiring

## Mechanical Assembly

### Mounting Matrix

1. Use M3 screws through matrix mounting holes
2. Leave 2-3cm clearance behind matrix for electronics
3. Ensure good ventilation (matrix generates heat)

### Organizing Cables

1. Use cable ties to bundle wires
2. Label wires at both ends
3. Route wires away from high-current paths
4. Leave slack for maintenance

### Enclosure (Optional)

1. Use ventilated enclosure
2. Mount ESP32 on standoffs
3. Create access panel for USB port
4. Add ethernet port or WiFi antenna hole

## Safety Warnings

⚠️ **IMPORTANT SAFETY NOTES**:

- Never exceed 5V on matrix power input
- Do not touch circuits while powered
- Matrix can draw 10+ amps at full brightness white
- Use appropriate wire gauge for current (18 AWG minimum for 5A+)
- Ensure power supply has overcurrent protection
- Do not cover matrix (fire hazard)
- Unplug before making any wiring changes

## Maintenance

### Regular Checks

- Monthly: Visual inspection for loose connections
- Quarterly: Clean dust from matrix (compressed air, power off)
- Yearly: Check solder joints if any, reseat connections

### LED Matrix Lifetime

- Typical lifetime: 50,000-100,000 hours
- Reduce brightness to extend life
- Avoid static images (screen burn-in)
- Use screensaver or rotating content

## Next Steps

Once hardware is assembled and tested:

1. Configure WiFi settings in firmware
2. Set up web interface access
3. Test UDP protocol
4. Install Q-SYS plugin (if using Q-SYS)
5. Create custom segment layouts
6. Add custom fonts

---

**Need Help?**

- Check [Troubleshooting Section](#troubleshooting-hardware-issues)
- See [UDP Protocol Documentation](UDP_PROTOCOL.md)
- Report issues on GitHub

**Hardware setup complete!** Proceed to software configuration.
