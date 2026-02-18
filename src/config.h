#ifndef CONFIG_H
#define CONFIG_H

// Hardware Configuration for Olimex ESP32 Gateway
// HUB75 LED Matrix Pin Configuration
// Compatible with ALL hardware revisions (A through I)

// Pin assignments for Olimex ESP32 Gateway to HUB75 matrix
//
// ══════════════════════════════════════════════════════════════════
// ⚠ HARDWARE CONFLICT WARNING ⚠
//
// The Olimex ESP32-GATEWAY uses Ethernet via ESP32 RMII interface.
// These GPIO pins are PERMANENTLY RESERVED for Ethernet and CANNOT
// be used for any other purpose:
//
//   GPIO 0  — RMII REF_CLK      (fixed by silicon)
//   GPIO 19 — RMII TXD0         (fixed by silicon)
//   GPIO 21 — RMII TX_EN        (fixed by silicon)
//   GPIO 22 — RMII TXD1         (fixed by silicon)
//   GPIO 25 — RMII RXD0         (fixed by silicon)
//   GPIO 26 — RMII RXD1         (fixed by silicon)
//   GPIO 27 — RMII CRS_DV       (fixed by silicon)
//   GPIO 5  — ETH PHY power     (wired on board)
//   GPIO 17 — ETH clock output  (wired on board, Rev D+)
//   GPIO 18 — ETH MDIO          (wired on board)
//   GPIO 23 — ETH MDC           (wired on board)
//
// Available output GPIOs: 2, 4, 12, 13, 14, 15, 16, 32, 33  (9 pins)
// HUB75 requires: R1,G1,B1,R2,G2,B2,A,B,C,D,LAT,OE,CLK     (13 signals)
//
// ⚠ CONCLUSION: This board CANNOT drive HUB75 directly while
//   Ethernet is active. You need an external shift-register
//   buffer (e.g. 74HC595) to expand the available GPIO count,
//   OR use a different ESP32 board that has more free GPIOs
//   (e.g. ESP32-DevKitC, ESP32-S3).
//
// The pin assignments below use the 9 available free GPIOs for
// the 6 colour data lines + 3 most critical address/control lines.
// This is a PARTIAL assignment — the matrix will NOT work correctly
// until the hardware conflict is resolved.
// ══════════════════════════════════════════════════════════════════

// Color Data (Upper half) — 3 free GPIOs
#define R1_PIN 2   // Red Data Upper
#define G1_PIN 15  // Green Data Upper
#define B1_PIN 4   // Blue Data Upper

// Color Data (Lower half) — 3 free GPIOs
#define R2_PIN 16  // Red Data Lower
#define G2_PIN 12  // Green Data Lower  (was 27 — RMII CRS_DV conflict)
#define B2_PIN 14  // Blue Data Lower   (was 32)

// Row Address Lines — only 1 free GPIO left after colour data
#define A_PIN 33   // Row Address A  (was 5  — ETH power pin conflict)
#define B_PIN 13   // Row Address B  (was 18 — ETH MDIO conflict)
#define C_PIN -1   // Row Address C  *** NO FREE GPIO — ETH conflict ***
#define D_PIN -1   // Row Address D  *** NO FREE GPIO — ETH conflict ***
#define E_PIN -1   // Row Address E (Not used for 1/16 scan on 32px height panels)

// Control Signals — 2 free GPIOs left
#define LAT_PIN 32 // Latch
#define OE_PIN  -1 // Output Enable  *** NO FREE GPIO — ETH conflict ***
#define CLK_PIN -1 // Clock          *** NO FREE GPIO — ETH conflict ***

// Matrix Configuration
#ifndef LED_MATRIX_WIDTH
#define LED_MATRIX_WIDTH 64
#endif

#ifndef LED_MATRIX_HEIGHT
#define LED_MATRIX_HEIGHT 32
#endif

#define MATRIX_CHAIN 1      // Number of chained panels
#define MATRIX_SCAN 16      // 1/16 scan for 32px height

// Network Configuration
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// Compile-time warning for unconfigured WiFi
#if defined(WIFI_SSID)
  #define WIFI_SSID_STR WIFI_SSID
  #if __has_include(<cstring>)
    #include <cstring>
  #endif
#endif

// UDP Configuration
#ifndef UDP_PORT
#define UDP_PORT 21324
#endif

#define UDP_BUFFER_SIZE 512  // JSON commands can be up to ~300 bytes

// Segment Configuration
#define MAX_SEGMENTS 4
#define DEFAULT_BRIGHTNESS 128

// Text Configuration
#define MAX_TEXT_LENGTH 128
#define DEFAULT_SCROLL_SPEED 50  // pixels per second

// Effect Update Intervals (milliseconds)
#define SCROLL_UPDATE_INTERVAL 50   // 50ms for smooth scrolling
#define BLINK_UPDATE_INTERVAL 500   // 500ms for blinking

// Web Server Configuration
#define WEB_SERVER_PORT 80

// Storage Configuration
#define CONFIG_FILE "/config.json"
#define SEGMENT_CONFIG_FILE "/segments.json"

#endif // CONFIG_H
