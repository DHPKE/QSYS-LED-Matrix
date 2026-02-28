// config.h - Configuration for RPi RGB LED Matrix Controller

#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>

#include <string>
#include <vector>

// ─── Matrix Hardware ─────────────────────────────────────────────────────────
#define MATRIX_WIDTH   64
#define MATRIX_HEIGHT  32
#define MATRIX_CHAIN   1        // Number of panels chained
#define MATRIX_PARALLEL 1       // Parallel chains

// GPIO Configuration
#define HARDWARE_MAPPING "regular"  // "regular", "adafruit-hat", "adafruit-hat-pwm"
#define GPIO_SLOWDOWN   3           // Match Python version
#define PWM_BITS        8           // Match Python version
#define BRIGHTNESS      50          // 0-100: Initial brightness percentage

// Additional matrix options
#define SCAN_MODE           0       // 0=progressive, 1=interlaced
#define ROW_ADDR_TYPE       0       // 0-5: Address line config
#define MULTIPLEXING        0       // 0-17: Multiplex type
#define PWM_LSB_NANOSECONDS 200     // Timing for LSB (100-300 typical)
#define PWM_DITHER_BITS     0       // 0-2: Match Python version
#define LED_RGB_SEQUENCE    "RGB"   // "RGB", "RBG", "GRB", etc.
#define REFRESH_LIMIT       200     // Hz: Match Python version (200 = stable refresh)

// ─── Network ─────────────────────────────────────────────────────────────────
#define UDP_PORT       21324
#define UDP_BIND_ADDR  "0.0.0.0"
#define WEB_PORT       8080

// Fallback static IP (applied if DHCP fails)
#define FALLBACK_IP      "10.20.30.40"
#define FALLBACK_NETMASK "255.255.255.0"
#define FALLBACK_GATEWAY "10.20.30.1"
#define FALLBACK_IFACE   "eth0"
#define DHCP_TIMEOUT_S   15

// ─── Display ─────────────────────────────────────────────────────────────────
#define MAX_SEGMENTS      4
#define MAX_TEXT_LENGTH   128
#define EFFECT_INTERVAL   50    // milliseconds between effect updates (20 fps, matches Python)

// Orientation: 0=landscape (64×32), 1=portrait (32×64)
enum Orientation {
    LANDSCAPE = 0,
    PORTRAIT = 1
};

// ─── Text Effects ────────────────────────────────────────────────────────────
enum Effect {
    EFFECT_NONE = 0,
    EFFECT_SCROLL,
    EFFECT_BLINK,
    EFFECT_FADE
};

// ─── Text Alignment ──────────────────────────────────────────────────────────
enum Align {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

// ─── Font Paths ──────────────────────────────────────────────────────────────
#define FONT_PATH          "/usr/share/fonts/truetype/msttcorefonts/Arial_Bold.ttf"
#define FONT_PATH_FALLBACK "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
#define FONT_MONO_PATH     "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"

// Font size search range (tries largest first)
const int FONT_SIZES[] = {32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 13, 12, 11, 10, 9, 8, 7, 6};
const int FONT_SIZES_COUNT = sizeof(FONT_SIZES) / sizeof(FONT_SIZES[0]);

// ─── Persistence ─────────────────────────────────────────────────────────────
#define CONFIG_FILE   "/var/lib/led-matrix/config.json"
#define SEGMENT_FILE  "/var/lib/led-matrix/segments.json"

// ─── Group Configuration ─────────────────────────────────────────────────────
struct GroupColor {
    uint8_t r, g, b;
};

const GroupColor GROUP_COLORS[9] = {
    {0, 0, 0},       // 0: No group (black/invisible)
    {255, 255, 255}, // 1: White
    {255, 255, 0},   // 2: Yellow
    {255, 165, 0},   // 3: Orange
    {255, 0, 0},     // 4: Red
    {255, 0, 255},   // 5: Magenta
    {0, 0, 255},     // 6: Blue
    {0, 255, 255},   // 7: Cyan
    {0, 255, 0}      // 8: Green
};

#define GROUP_INDICATOR_SIZE 2  // Size of group indicator square (2×2 pixels)

// ─── Layout Presets ──────────────────────────────────────────────────────────
// Each layout is a vector of rectangles: {x, y, width, height}
struct LayoutRect {
    int x, y, w, h;
};

// Landscape layouts (64×32)
const std::vector<LayoutRect> LAYOUT_LANDSCAPE[15] = {
    {},  // 0: Invalid
    {{0, 0, 64, 32}},  // 1: Fullscreen
    {{0, 0, 64, 16}, {0, 16, 64, 16}},  // 2: Top/Bottom halves
    {{0, 0, 32, 32}, {32, 0, 32, 32}},  // 3: Left/Right halves
    {{0, 0, 32, 32}, {32, 0, 32, 16}, {32, 16, 32, 16}},  // 4: Triple left
    {{0, 0, 32, 16}, {0, 16, 32, 16}, {32, 0, 32, 32}},   // 5: Triple right
    {{0, 0, 21, 32}, {21, 0, 21, 32}, {42, 0, 22, 32}},   // 6: Thirds vertical
    {{0, 0, 32, 16}, {32, 0, 32, 16}, {0, 16, 32, 16}, {32, 16, 32, 16}},  // 7: Quad
    {},  // 8-10: Reserved
    {},
    {},
    {{0, 0, 64, 32}},  // 11: Seg 0 fullscreen
    {{0, 0, 1, 1}, {0, 0, 64, 32}},  // 12: Seg 1 fullscreen
    {{0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 64, 32}},  // 13: Seg 2 fullscreen
    {{0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 64, 32}}  // 14: Seg 3 fullscreen
};

// Portrait layouts (32×64)
const std::vector<LayoutRect> LAYOUT_PORTRAIT[15] = {
    {},  // 0: Invalid
    {{0, 0, 32, 64}},  // 1: Fullscreen
    {{0, 0, 32, 32}, {0, 32, 32, 32}},  // 2: Top/Bottom halves
    {{0, 0, 16, 64}, {16, 0, 16, 64}},  // 3: Left/Right halves
    {{0, 0, 32, 32}, {0, 32, 16, 32}, {16, 32, 16, 32}},  // 4: Triple top
    {{0, 0, 16, 32}, {16, 0, 16, 32}, {0, 32, 32, 32}},   // 5: Triple bottom
    {{0, 0, 32, 21}, {0, 21, 32, 21}, {0, 42, 32, 22}},   // 6: Thirds horizontal
    {{0, 0, 16, 32}, {16, 0, 16, 32}, {0, 32, 16, 32}, {16, 32, 16, 32}},  // 7: Quad
    {},  // 8-10: Reserved
    {},
    {},
    {{0, 0, 32, 64}},  // 11: Seg 0 fullscreen
    {{0, 0, 1, 1}, {0, 0, 32, 64}},  // 12: Seg 1 fullscreen
    {{0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 32, 64}},  // 13: Seg 2 fullscreen
    {{0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 32, 64}}  // 14: Seg 3 fullscreen
};

#endif // CONFIG_H
