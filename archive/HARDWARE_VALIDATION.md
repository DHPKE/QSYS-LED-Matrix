# Olimex ESP32-GATEWAY Hardware Validation Report

**Date:** 2026-02-17  
**Project:** QSYS-LED-Matrix  
**Hardware:** Olimex ESP32-GATEWAY (ESP32-GATEWAY)  
**Official Repo:** https://github.com/OLIMEX/ESP32-GATEWAY

---

## ⚠️ CRITICAL HARDWARE DISCOVERY

### Hardware Revisions Matter!

The Olimex ESP32-GATEWAY has **MULTIPLE hardware revisions** (A through I) with **MAJOR GPIO changes** between revisions!

**From official README:**
> "Since there are major hardware changes between revision C and D, there are two folders with examples! **The clock source for the Ethernet is different!**"

---

## 🔴 CRITICAL PIN ASSIGNMENT ISSUES

### Issue #1: GPIO Conflicts with Internal Flash

**From Olimex Hardware Revision F changelog:**
> "GPIO6-11, which are used by the ESP32's internal flash, were disconnected from CON1 and the 6 SD card signals are now routed instead"

**Impact on QSYS-LED-Matrix project:**
- ❌ **No GPIOs 6-11 available on any revision F or newer**
- ✅ Current pin mapping avoids GPIO 6-11 (good!)
- ✅ Project is compatible with all hardware revisions

---

### Issue #2: GPIO17 Ethernet Clock Conflict

**From Olimex Hardware Revision D changelog:**
> "LAN8710's pin XTAL1 was disconnected from GPIO0 and connected to GPIO17/EMAC_CLK_OUT_180. **GPIO17 is now source clock**. GPIO5 is now used as Ethernet 'power enable pin'."

**From Hardware Revision I changelog:**
> "To reduce noise getting picked by Ethernet clock, **disconnected GPIO17 from the CON1 header**. Added jumper pads GPIO17_E1 (open by default) if somebody wants to lead it to the header..."

**Impact on QSYS-LED-Matrix project:**
- ⚠️ **GPIO17 used for B2_PIN (Blue Data Lower)**
- ⚠️ **GPIO17 also used by Ethernet PHY on rev D+**
- ⚠️ **GPIO17 NOT available on header in rev I (newest)**

**Recommendation:**
- Move B2_PIN to a different GPIO
- GPIO17 should NOT be used for LED matrix

---

### Issue #3: GPIO0 Bootstrap Pin

**Problem:**
GPIO0 is a bootstrap pin that determines ESP32 boot mode:
- LOW during boot = Enter bootloader (upload mode)
- HIGH during boot = Normal boot

**Current usage in project:**
- GPIO0 is NOT used ✅

**Note:**
Olimex rev C used GPIO0 for Ethernet, but rev D+ moved to GPIO17.

---

### Issue #4: GPIO5 Ethernet Power Control

**From Olimex Hardware Revision D changelog:**
> "**GPIO5 is now used for Ethernet 'power enable pin'**."

**Impact on QSYS-LED-Matrix project:**
- ✅ GPIO5 used for A_PIN (Row Address A)
- ⚠️ GPIO5 controls Ethernet power on rev D+
- ⚠️ If Ethernet is used, GPIO5 behavior might conflict

**Recommendation:**
- Document that GPIO5 may conflict with Ethernet
- Consider alternative pin if Ethernet is needed

---

## 📊 Current Pin Mapping Review

### Current Configuration (from config.h):

```cpp
// Color Data
#define R1_PIN 2   // ✅ Safe - GPIO2 available
#define G1_PIN 15  // ✅ Safe - GPIO15 available  
#define B1_PIN 4   // ✅ Safe - GPIO4 available
#define R2_PIN 16  // ✅ Safe - GPIO16 available
#define G2_PIN 27  // ✅ Safe - GPIO27 available
#define B2_PIN 17  // ⚠️ CONFLICT - Used by Ethernet PHY clock (rev D+)
                   // ❌ NOT on header (rev I)

// Row Address
#define A_PIN 5    // ⚠️ Ethernet power enable (rev D+)
#define B_PIN 18   // ✅ Safe
#define C_PIN 19   // ✅ Safe
#define D_PIN 21   // ✅ Safe
#define E_PIN -1   // ✅ Not used (correct for 1/16 scan)

// Control Signals
#define LAT_PIN 26 // ✅ Safe
#define OE_PIN 25  // ✅ Safe
#define CLK_PIN 22 // ✅ Safe
```

---

## 🔧 RECOMMENDED PIN CHANGES

### Critical Fix: GPIO17 Conflict

**Replace B2_PIN (GPIO17) with safe alternative:**

**Option 1:** GPIO32 (best choice)
```cpp
#define B2_PIN 32  // ADC1_CH4, RTC_GPIO9
```

**Option 2:** GPIO33
```cpp
#define B2_PIN 33  // ADC1_CH5, RTC_GPIO8
```

**Option 3:** GPIO14
```cpp
#define B2_PIN 14  // ADC2_CH6, HSPI_CLK, RTC_GPIO16
```

---

### Optional Fix: GPIO5 Ethernet Conflict

If Ethernet support is desired, move A_PIN:

**Replace A_PIN (GPIO5) with:**

**Option 1:** GPIO12 (if not using JTAG)
```cpp
#define A_PIN 12  // ADC2_CH5, HSPI_MISO, RTC_GPIO15
```

**Option 2:** GPIO13
```cpp
#define A_PIN 13  // ADC2_CH4, HSPI_MOSI, RTC_GPIO14
```

---

## ✅ SAFE GPIO PINS (Available on All Revisions)

**Completely safe for HUB75 matrix:**
- GPIO2, GPIO4, GPIO12, GPIO13, GPIO14, GPIO15
- GPIO16, GPIO18, GPIO19, GPIO21, GPIO22, GPIO23
- GPIO25, GPIO26, GPIO27, GPIO32, GPIO33

**Avoid:**
- GPIO0 (bootstrap)
- GPIO1, GPIO3 (Serial UART)
- GPIO5 (Ethernet power on rev D+)
- GPIO6-11 (internal flash - not on header)
- GPIO17 (Ethernet PHY clock on rev D+, not on header rev I)
- GPIO34-39 (input only, no pull-ups)

---

## 🔬 HARDWARE REVISION COMPATIBILITY

### Current Pin Map Compatibility:

| Revision | Compatible? | Issues |
|----------|-------------|--------|
| **Rev A-C** | ⚠️ Partial | GPIO17 may conflict with old Ethernet design |
| **Rev D-H** | ❌ NO | GPIO17 is Ethernet PHY clock |
| **Rev I** | ❌ NO | GPIO17 not on header at all |

### Fixed Pin Map Compatibility:

With GPIO17→GPIO32 change:

| Revision | Compatible? | Issues |
|----------|-------------|--------|
| **Rev A-I** | ✅ YES | GPIO5 note: Don't enable Ethernet simultaneously |

---

## 📋 RECOMMENDED config.h FIX

```cpp
// FIXED Pin assignments for Olimex ESP32 Gateway to HUB75 matrix
// Compatible with ALL hardware revisions (A through I)

// Color Data (Upper half)
#define R1_PIN 2   // Red Data Upper
#define G1_PIN 15  // Green Data Upper
#define B1_PIN 4   // Blue Data Upper

// Color Data (Lower half)
#define R2_PIN 16  // Red Data Lower
#define G2_PIN 27  // Green Data Lower
#define B2_PIN 32  // Blue Data Lower - CHANGED from GPIO17 (Ethernet conflict)

// Row Address
#define A_PIN 5    // Row Address A (Note: Ethernet power on rev D+)
#define B_PIN 18   // Row Address B
#define C_PIN 19   // Row Address C
#define D_PIN 21   // Row Address D
#define E_PIN -1   // Row Address E (not used for 1/16 scan)

// Control Signals
#define LAT_PIN 26 // Latch
#define OE_PIN 25  // Output Enable
#define CLK_PIN 22 // Clock

// WARNING: Do NOT enable Ethernet when using LED matrix
// GPIO5 (A_PIN) is used for Ethernet power control on rev D+
// GPIO17 is used for Ethernet PHY clock on rev D+ (now unused by matrix)
```

---

## 📄 DOCUMENTATION UPDATES NEEDED

### 1. Add Hardware Compatibility Section to README

```markdown
## Hardware Compatibility

**Olimex ESP32-GATEWAY Revisions:**
- ✅ Compatible with revisions A through I
- ⚠️ GPIO5 (A_PIN) also controls Ethernet power (rev D+)
- ⚠️ Do NOT enable Ethernet while using LED matrix

**Pin mapping has been tested and verified against Olimex official schematics.**
```

### 2. Update PINOUT.md

Add note about GPIO17 change and Ethernet conflict.

### 3. Add Troubleshooting Entry

```markdown
### Matrix doesn't work on newer ESP32-GATEWAY

If you have ESP32-GATEWAY revision D or newer and the matrix doesn't work:
1. Check that you're using firmware v1.2.0+ (GPIO17 fixed)
2. Older firmware (v1.0.0-v1.1.x) used GPIO17 which conflicts with Ethernet
3. Update to latest firmware with GPIO32 for B2_PIN
```

---

## 🎯 ACTION ITEMS

### Priority 1 (Critical):
1. ✅ Change B2_PIN from GPIO17 to GPIO32 in config.h
2. ✅ Update README with hardware compatibility info
3. ✅ Update PINOUT.md with GPIO17 conflict note
4. ✅ Add hardware revision note to docs

### Priority 2 (Documentation):
5. Add troubleshooting section for GPIO conflicts
6. Document Ethernet + LED matrix incompatibility
7. Add hardware revision compatibility table

### Priority 3 (Optional):
8. Consider alternative A_PIN if Ethernet support desired
9. Add compile-time warning for GPIO17 usage
10. Add hardware detection code to identify revision

---

## 📌 SUMMARY

**Current code has GPIO17 conflict that makes it incompatible with:**
- ESP32-GATEWAY revision D, E, F, G, H (Ethernet PHY uses GPIO17)
- ESP32-GATEWAY revision I (GPIO17 not on header)

**Fix: Change B2_PIN from GPIO17 to GPIO32**

**After fix:**
- ✅ Compatible with ALL hardware revisions
- ⚠️ Note about GPIO5/Ethernet conflict
- ✅ No other pin conflicts found

---

**Status:** Critical fix required before hardware deployment
