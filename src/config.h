#ifndef CONFIG_H
#define CONFIG_H

// Hardware: WT32-ETH01 + 64x32 HUB75 LED Matrix
// WT32-ETH01 ETH reserved GPIO (DO NOT USE): 0,18,19,21,22,23,25,26,27
// GPIO0 = RMII REF_CLK output (ESP32 drives 50 MHz to LAN8720, ETH_CLOCK_GPIO0_OUT)
// Free output GPIOs: 2,4,5,12,13,14,15,17,32,33 + IO16(CLK) + IO1(LAT)
// Pin assignment: R1=2 G1=15 B1=4  R2=5 G2=17 B2=12
//                 A=33 B=32  C=13  D=14  E=-1
//                 CLK=16  LAT=1  OE=-1 (I2S-managed)

#define R1_PIN   2
#define G1_PIN  15
#define B1_PIN   4

#define R2_PIN   5
#define G2_PIN  17
#define B2_PIN  12

#define A_PIN   33
#define B_PIN   32
#define C_PIN   13
#define D_PIN   14
#define E_PIN   -1

#define CLK_PIN 16
#define LAT_PIN  1
#define OE_PIN  -1

#ifndef LED_MATRIX_WIDTH
#define LED_MATRIX_WIDTH  64
#endif

#ifndef LED_MATRIX_HEIGHT
#define LED_MATRIX_HEIGHT 32
#endif

#define MATRIX_CHAIN      1
#define MATRIX_SCAN       16

#ifndef UDP_PORT
#define UDP_PORT 21324
#endif

#define UDP_BUFFER_SIZE   512
#define MAX_SEGMENTS       4
#define DEFAULT_BRIGHTNESS 128
#define MAX_TEXT_LENGTH         128
#define DEFAULT_SCROLL_SPEED     50
#define SCROLL_UPDATE_INTERVAL   50
#define BLINK_UPDATE_INTERVAL   500
#define WEB_SERVER_PORT   80
#define CONFIG_FILE         "/config.json"
#define SEGMENT_CONFIG_FILE "/segments.json"

// ── Fallback static IP (used when no DHCP lease is obtained within 15 s) ────
#define FALLBACK_IP      "10.10.10.99"
#define FALLBACK_GW      "10.10.10.1"
#define FALLBACK_SUBNET  "255.255.255.0"

#endif // CONFIG_H
