#ifndef CONFIG_H
#define CONFIG_H

// Hardware Configuration for Olimex ESP32 Gateway
// HUB75 LED Matrix Pin Configuration
// Compatible with ALL hardware revisions (A through I)

// Pin assignments for Olimex ESP32 Gateway to HUB75 matrix
// TESTED AND VERIFIED against Olimex official schematics

// Color Data (Upper half)
#define R1_PIN 2   // Red Data Upper
#define G1_PIN 15  // Green Data Upper
#define B1_PIN 4   // Blue Data Upper

// Color Data (Lower half)
#define R2_PIN 16  // Red Data Lower
#define G2_PIN 27  // Green Data Lower
#define B2_PIN 32  // Blue Data Lower - FIXED: Changed from GPIO17 (Ethernet conflict)
                   // GPIO17 used by Ethernet PHY on rev D+ and not on header in rev I

// Row Address Lines
#define A_PIN 5    // Row Address A (Note: Also controls Ethernet power on rev D+)
#define B_PIN 18   // Row Address B
#define C_PIN 19   // Row Address C
#define D_PIN 21   // Row Address D
#define E_PIN -1   // Row Address E (Not used for 1/16 scan on 32px height panels)

// Control Signals
#define LAT_PIN 26 // Latch
#define OE_PIN 25  // Output Enable (active low)
#define CLK_PIN 22 // Clock

// ⚠️ IMPORTANT COMPATIBILITY NOTES:
// - GPIO17 cannot be used: Ethernet PHY clock on rev D+, not on header in rev I
// - GPIO5 (A_PIN) also controls Ethernet power on rev D+
// - Do NOT enable Ethernet while using LED matrix
// - Tested compatible with ESP32-GATEWAY hardware revisions A through I

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
#if defined(WIFI_SSID) && (strcmp(WIFI_SSID, "YOUR_SSID") == 0)
#warning "WiFi SSID not configured! Update config.h with your network credentials"
#endif

// UDP Configuration
#ifndef UDP_PORT
#define UDP_PORT 21324
#endif

#define UDP_BUFFER_SIZE 256  // Reduced from 512 (was wasteful, max command ~200 bytes)

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
