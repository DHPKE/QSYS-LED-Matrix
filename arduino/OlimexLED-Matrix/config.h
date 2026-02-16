#ifndef CONFIG_H
#define CONFIG_H

// Hardware Configuration for Olimex ESP32 Gateway
// HUB75 LED Matrix Pin Configuration

// Pin assignments for Olimex ESP32 Gateway to HUB75 matrix
#define R1_PIN 2
#define G1_PIN 15
#define B1_PIN 4
#define R2_PIN 16
#define G2_PIN 27
#define B2_PIN 17

#define A_PIN 5
#define B_PIN 18
#define C_PIN 19
#define D_PIN 21
#define E_PIN -1  // Not used for 1/16 scan

#define LAT_PIN 26
#define OE_PIN 25
#define CLK_PIN 22

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

// UDP Configuration
#ifndef UDP_PORT
#define UDP_PORT 21324
#endif

#define UDP_BUFFER_SIZE 512

// Segment Configuration
#define MAX_SEGMENTS 4
#define DEFAULT_BRIGHTNESS 128

// Text Configuration
#define MAX_TEXT_LENGTH 128
#define DEFAULT_SCROLL_SPEED 50  // pixels per second

// Web Server Configuration
#define WEB_SERVER_PORT 80

// Storage Configuration
#define CONFIG_FILE "/config.json"
#define SEGMENT_CONFIG_FILE "/segments.json"

#endif // CONFIG_H
