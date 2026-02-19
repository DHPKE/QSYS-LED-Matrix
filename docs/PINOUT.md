# WT32-ETH01 to HUB75 Pinout

## Wiring Diagram

![WT32-ETH01 to HUB75 Pinout](images/wt32-eth01-hub75-pinout.svg)

---

## Pin Assignment Summary

| HUB75 Signal | WT32-ETH01 GPIO | Header / Pad | Notes |
|---|---|---|---|
| R1 (Red upper)   | IO2  | J2 pin 5     | |
| G1 (Green upper) | IO15 | J2 pin 3     | |
| B1 (Blue upper)  | IO4  | J2 pin 6     | |
| R2 (Red lower)   | IO5  | Misc pad     | |
| G2 (Green lower) | IO17 | Misc pad     | |
| B2 (Blue lower)  | IO12 | J2 pin 2     | |
| A (Row addr)     | IO33 | J1 pin 6     | |
| B (Row addr)     | IO32 | J1 pin 5     | |
| C (Row addr)     | IO13 | J2 pin 4     | |
| D (Row addr)     | IO14 | J2 pin 1     | |
| E (Row addr)     | --   | n/a          | Not needed for 32px 1:16 scan |
| CLK              | IO16 | nRST pad     | 10k pull-up on board keeps PHY up after ETH.begin() |
| LAT              | IO1  | TX0 pad      | UART-TX; reused after boot log |
| OE               | GND  | Panel pin 15 | Tie to GND; library uses I2S blanking |

---

## HUB75 Connector (2x8 IDC, cable side)

`
     Pin1
    +------+------+------+------+------+------+------+------+
    |  R1  |  G1  |  B1  | GND  |  R2  |  G2  |  B2  | GND  |
    +------+------+------+------+------+------+------+------+
    |   A  |   B  |   C  |   D  | CLK  | LAT  |  OE  | GND  |
    +------+------+------+------+------+------+------+------+
`

| IDC Pin | Signal | IDC Pin | Signal |
|---------|--------|---------|--------|
| 1  | R1  | 2  | G1  |
| 3  | B1  | 4  | GND |
| 5  | R2  | 6  | G2  |
| 7  | B2  | 8  | GND |
| 9  | A   | 10 | B   |
| 11 | C   | 12 | D   |
| 13 | CLK | 14 | LAT |
| 15 | OE  | 16 | GND |

---

## J1 Header (8-pin)

| J1 Pin | GPIO | Used for |
|--------|------|----------|
| 1 | IO36 | -- (input-only) |
| 2 | IO39 | -- (input-only) |
| 3 | IO34 | -- (input-only) |
| 4 | IO35 | -- (input-only) |
| 5 | IO32 | **B (row address)** |
| 6 | IO33 | **A (row address)** |
| 7 | GND  | GND |
| 8 | 3V3  | 3.3 V power |

## J2 Header (8-pin)

| J2 Pin | GPIO | Used for |
|--------|------|----------|
| 1 | IO14 | **D (row address)** |
| 2 | IO12 | **B2 (Blue lower)** |
| 3 | IO15 | **G1 (Green upper)** |
| 4 | IO13 | **C (row address)** |
| 5 | IO2  | **R1 (Red upper)** |
| 6 | IO4  | **B1 (Blue upper)** |
| 7 | GND  | GND |
| 8 | 3V3  | 3.3 V power |

## Misc Solder Pads

| Pad  | GPIO | Used for |
|------|------|----------|
| nRST | IO16 | **CLK** -- board 10k pull-up holds LAN8720 deasserted; safe to use as CLK after ETH init |
| TX0  | IO1  | **LAT** -- repurposed after boot log |
| IO5  | IO5  | **R2 (Red lower)** |
| IO17 | IO17 | **G2 (Green lower)** |

---

## Ethernet Reserved GPIO (DO NOT USE for HUB75)

| GPIO | RMII / ETH Function |
|------|---------------------|
| IO0  | REF_CLK (50 MHz oscillator input) |
| IO18 | MDIO |
| IO19 | TXD0 |
| IO21 | TX_EN |
| IO22 | TXD1 |
| IO23 | MDC |
| IO25 | RXD0 |
| IO26 | RXD1 |
| IO27 | CRS_DV |

---

## Boot / Flash Mode

To enter download mode for flashing via PlatformIO / esptool:

1. Connect a **10 kOhm resistor between IO0 and GND**
2. Power-cycle the board (or press EN/RST)
3. Upload firmware: pio run --target upload
4. **Remove the IO0 resistor** and reset to start normal operation

UART wiring during flash:

| USB-UART Adapter | WT32-ETH01 Pad |
|-----------------|----------------|
| TX              | RX0 (IO3)      |
| RX              | TX0 (IO1)      |
| GND             | GND            |

> IO1 (TX0) doubles as LAT in firmware. Boot log output on TX0 during flashing is expected and harmless.

---

## Power Requirements

| Rail | Source | Notes |
|------|--------|-------|
| ESP32 + ETH PHY | 5 V via USB-UART or external | ~200 mA typical |
| LED Panel LEDs  | **Separate 5 V, minimum 4 A** | Do NOT power from WT32-ETH01 |
| LED Panel logic | 3.3 V from J1/J2 pin 8       | Signal levels only |
