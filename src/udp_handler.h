#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

// UDP handler: JSON protocol over POSIX BSD sockets (lwIP)
// Accepts commands from Q-SYS plugin via UDP on port UDP_PORT.
//
// ── Text command ─────────────────────────────────────────────────────────────
//   {"cmd":"text","seg":0,"text":"Hello",
//    "color":"FFFFFF" or 1..14,   "bgcolor":"000000" or 1..14,
//    "font":"arial"   or 1..3,    "size":"auto",
//    "align":"C"/"L"/"R",         "effect": "none" or 0..3,
//    "intensity":255}
//
// ── Layout preset ────────────────────────────────────────────────────────────
//   {"cmd":"layout","preset":1}
//   1=Fullscreen  2=Split-H  3=Split-V  4=Quad
//   5=Thirds(1|2|3)  6=Triple(left|top-right|bottom-right)
//   11=Seg1-Full  12=Seg2-Full  13=Seg3-Full  14=Seg4-Full
//
// ── Other commands ───────────────────────────────────────────────────────────
//   {"cmd":"clear","seg":0}
//   {"cmd":"clear_all"}
//   {"cmd":"brightness","value":200}
//   {"cmd":"config","seg":0,"x":0,"y":0,"w":64,"h":32}
//
// ── Integer enums ────────────────────────────────────────────────────────────
//   Colors 1-14: white red lime blue yellow magenta cyan orange purple pink gold silver grey black
//   Fonts  1-3 : Arial Verdana Impact
//   Effects 0-3: none scroll blink fade
//   Align  l/c/r

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <ArduinoJson.h>
#include "config.h"
#include "segment_manager.h"
#include "fonts.h"

// ── Color helpers ─────────────────────────────────────────────────────────────
static uint16_t rgb888to565(uint32_t c) {
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >>  8) & 0xFF;
    uint8_t b = (c      ) & 0xFF;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Integer color id → RGB565
static uint16_t colorIdTo565(int id) {
    switch (id) {
        case  1: return rgb888to565(0xFFFFFF); // white
        case  2: return rgb888to565(0xFF0000); // red
        case  3: return rgb888to565(0x00FF00); // lime
        case  4: return rgb888to565(0x0000FF); // blue
        case  5: return rgb888to565(0xFFFF00); // yellow
        case  6: return rgb888to565(0xFF00FF); // magenta
        case  7: return rgb888to565(0x00FFFF); // cyan
        case  8: return rgb888to565(0xFF8000); // orange
        case  9: return rgb888to565(0x800080); // purple
        case 10: return rgb888to565(0xFF69B4); // pink
        case 11: return rgb888to565(0xFFD700); // gold
        case 12: return rgb888to565(0xC0C0C0); // silver
        case 13: return rgb888to565(0x808080); // grey
        case 14: return rgb888to565(0x000000); // black
        default: return rgb888to565(0xFFFFFF);
    }
}

// Parse color field — accepts integer id OR hex string "RRGGBB"/"#RRGGBB"
static uint16_t parseColor565(JsonVariant v) {
    if (v.is<int>()) return colorIdTo565(v.as<int>());
    const char* s = v | "";
    if (s[0] == '\0') return 0xFFFF; // default white
    const char* p = (s[0] == '#') ? s + 1 : s;
    return rgb888to565((uint32_t)strtol(p, nullptr, 16));
}

static TextAlign parseAlign(const char* a) {
    if (!a) return ALIGN_CENTER;
    if (a[0] == 'L' || a[0] == 'l') return ALIGN_LEFT;
    if (a[0] == 'R' || a[0] == 'r') return ALIGN_RIGHT;
    return ALIGN_CENTER;
}

// Parse effect — accepts integer 0-3 or string
static TextEffect parseEffect(JsonVariant v) {
    if (v.is<int>()) {
        switch (v.as<int>()) {
            case 1: return EFFECT_SCROLL;
            case 2: return EFFECT_BLINK;
            case 3: return EFFECT_FADE;
            default: return EFFECT_NONE;
        }
    }
    const char* e = v | "none";
    if (strcasecmp(e, "scroll")  == 0) return EFFECT_SCROLL;
    if (strcasecmp(e, "blink")   == 0) return EFFECT_BLINK;
    if (strcasecmp(e, "fade")    == 0) return EFFECT_FADE;
    if (strcasecmp(e, "rainbow") == 0) return EFFECT_RAINBOW;
    return EFFECT_NONE;
}

// Parse font field — accepts integer 1-3 or name string → stores as "arial"/"verdana"/"impact"
static void parseFontName(JsonVariant v, char* out, size_t len) {
    const char* names[] = { "arial", "verdana", "impact" };
    if (v.is<int>()) {
        int id = v.as<int>();
        if (id < 1 || id > 3) id = 1;
        strncpy(out, names[id - 1], len - 1);
        out[len - 1] = '\0';
        return;
    }
    const char* s = v | "arial";
    // Normalise to canonical name via parseFontId
    uint8_t id = parseFontId(s);
    strncpy(out, names[id - 1], len - 1);
    out[len - 1] = '\0';
}


class UDPHandler {
private:
    int sock = -1;
    SegmentManager* segmentManager;
    uint8_t brightness;
    char packetBuffer[UDP_BUFFER_SIZE];
    bool _firstCommandReceived = false;

public:
    UDPHandler(SegmentManager* sm) : segmentManager(sm), brightness(255) {}

    // Apply a layout preset — sets geometry + isActive for all 4 segments.
    // W=LED_MATRIX_WIDTH (64), H=LED_MATRIX_HEIGHT (32)
    void applyLayoutPreset(int preset) {
        if (!segmentManager) return;
        const int W = LED_MATRIX_WIDTH;
        const int H = LED_MATRIX_HEIGHT;

        // Helper lambda: configure one segment geometry
        auto cfg = [&](int i, int x, int y, int w, int h) {
            Segment* s = segmentManager->getSegment((uint8_t)i);
            if (!s) return;
            s->x = x; s->y = y; s->width = w; s->height = h;
            s->isActive = (w > 0 && h > 0);
            s->isDirty  = true;
        };
        // Disable all first
        for (int i = 0; i < MAX_SEGMENTS; i++) cfg(i, 0, 0, 0, 0);

        switch (preset) {
            case 1: // Fullscreen — seg0 takes whole panel
                cfg(0,   0,   0,   W,   H);
                break;
            case 2: // Split-H — top / bottom
                cfg(0,   0,   0,   W, H/2);
                cfg(1,   0, H/2,   W, H/2);
                break;
            case 3: // Split-V — left | right
                cfg(0,   0,   0, W/2,   H);
                cfg(1, W/2,   0, W/2,   H);
                break;
            case 4: // Quad — 2×2 grid
                cfg(0,   0,   0, W/2, H/2);
                cfg(1, W/2,   0, W/2, H/2);
                cfg(2,   0, H/2, W/2, H/2);
                cfg(3, W/2, H/2, W/2, H/2);
                break;
            case 5: // Thirds — 3 equal columns
                cfg(0,       0, 0, W/3,   H);
                cfg(1,     W/3, 0, W/3,   H);
                cfg(2, 2*(W/3),0, W-2*(W/3), H); // remainder handles rounding
                break;
            case 6: // Triple — left half | top-right quarter | bottom-right quarter
                cfg(0,   0,   0, W/2,   H);
                cfg(1, W/2,   0, W/2, H/2);
                cfg(2, W/2, H/2, W/2, H/2);
                break;
            case 11: // Seg 1 fullscreen (seg0)
                cfg(0, 0, 0, W, H);
                break;
            case 12: // Seg 2 fullscreen (seg1)
                cfg(1, 0, 0, W, H);
                break;
            case 13: // Seg 3 fullscreen (seg2)
                cfg(2, 0, 0, W, H);
                break;
            case 14: // Seg 4 fullscreen (seg3)
                cfg(3, 0, 0, W, H);
                break;
            default:
                Serial.printf("[UDP] Unknown layout preset %d\n", preset);
                return;
        }
        Serial.printf("[UDP] LAYOUT preset=%d applied\n", preset);
    }

    // Public so web test handler in main.cpp can call it directly
    void dispatchCommand(const char* raw) {
        _firstCommandReceived = true;   // any command clears the IP splash

        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, raw);
        if (err) {
            Serial.printf("[UDP] JSON parse error: %s  raw: %.120s\n", err.c_str(), raw);
            return;
        }

        const char* cmd = doc["cmd"] | "";
        Serial.printf("[UDP] cmd=%s\n", cmd);

        // ── text ──────────────────────────────────────────────────────────────
        if (strcmp(cmd, "text") == 0) {
            int seg    = doc["seg"] | 0;
            const char* txt = doc["text"] | "";
            const char* sz  = doc["size"] | "auto";
            const char* aln = doc["align"] | "C";

            char fontBuf[16] = "arial";
            parseFontName(doc["font"], fontBuf, sizeof(fontBuf));

            uint16_t col565 = parseColor565(doc["color"]);
            uint16_t bg565  = parseColor565(doc["bgcolor"]);
            // Default color: white on black
            if (doc["color"].isNull())   col565 = 0xFFFF;
            if (doc["bgcolor"].isNull()) bg565  = 0x0000;

            TextEffect fx = parseEffect(doc["effect"]);

            Serial.printf("[UDP] TEXT seg%d \"%s\" font=%s al=%s fx=%d\n",
                          seg, txt, fontBuf, aln, (int)fx);

            if (segmentManager && seg >= 0 && seg < MAX_SEGMENTS) {
                Segment* s = segmentManager->getSegment((uint8_t)seg);
                if (s) {
                    strncpy(s->text, txt, MAX_TEXT_LENGTH - 1);
                    s->text[MAX_TEXT_LENGTH - 1] = '\0';
                    s->color   = col565;
                    s->bgColor = bg565;
                    strncpy(s->fontName, fontBuf, 15);
                    s->fontName[15] = '\0';
                    s->autoSize = (strcasecmp(sz, "auto") == 0);
                    if (!s->autoSize) s->fontSize = (uint8_t)atoi(sz);
                    s->align    = parseAlign(aln);
                    s->effect   = fx;
                    s->isActive = true;
                    s->isDirty  = true;
                }
            }

        // ── layout preset ─────────────────────────────────────────────────────
        } else if (strcmp(cmd, "layout") == 0) {
            int preset = doc["preset"] | 0;
            applyLayoutPreset(preset);

        // ── clear ─────────────────────────────────────────────────────────────
        } else if (strcmp(cmd, "clear") == 0) {
            int seg = doc["seg"] | 0;
            Serial.printf("[UDP] CLEAR seg%d\n", seg);
            if (segmentManager) segmentManager->clearSegment((uint8_t)seg);

        } else if (strcmp(cmd, "clear_all") == 0) {
            Serial.println("[UDP] CLEAR ALL");
            if (segmentManager) segmentManager->clearAll();

        // ── brightness ────────────────────────────────────────────────────────
        } else if (strcmp(cmd, "brightness") == 0) {
            int val = doc["value"] | -1;
            if (val >= 0 && val <= 255) {
                brightness = (uint8_t)val;
                Serial.printf("[UDP] BRIGHTNESS %d\n", brightness);
            }

        // ── config (raw geometry override) ───────────────────────────────────
        } else if (strcmp(cmd, "config") == 0) {
            int seg = doc["seg"] | 0;
            int x   = doc["x"]   | 0;
            int y   = doc["y"]   | 0;
            int w   = doc["w"]   | LED_MATRIX_WIDTH;
            int h   = doc["h"]   | LED_MATRIX_HEIGHT;
            Serial.printf("[UDP] CONFIG seg%d x=%d y=%d w=%d h=%d\n", seg, x, y, w, h);
            if (segmentManager && seg >= 0 && seg < MAX_SEGMENTS) {
                Segment* s = segmentManager->getSegment((uint8_t)seg);
                if (s) {
                    s->x = (int16_t)x; s->y = (int16_t)y;
                    s->width = (uint16_t)w; s->height = (uint16_t)h;
                    s->isActive = (w > 0 && h > 0);
                }
            }

        } else {
            Serial.printf("[UDP] Unknown cmd: %s\n", cmd);
        }
    }


    bool begin() {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            Serial.printf("[UDP] socket() failed: %d\n", errno);
            return false;
        }

        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(UDP_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            Serial.printf("[UDP] bind() failed: %d\n", errno);
            close(sock);
            sock = -1;
            return false;
        }

        // Non-blocking so process() never stalls the loop
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        Serial.printf("[UDP] Listening on port %d (JSON protocol)\n", UDP_PORT);
        return true;
    }

    void process() {
        if (sock < 0) return;

        struct sockaddr_in sender{};
        socklen_t senderLen = sizeof(sender);

        int len = recvfrom(sock, packetBuffer, sizeof(packetBuffer) - 1, 0,
                           (struct sockaddr*)&sender, &senderLen);
        if (len <= 0) return;

        packetBuffer[len] = '\0';
        Serial.printf("[UDP] RX %d bytes from %s: %.120s\n",
                      len, inet_ntoa(sender.sin_addr), packetBuffer);

        dispatchCommand(packetBuffer);
    }

    uint8_t getBrightness() { return brightness; }

    // Returns true once any UDP command has been received (clears IP splash)
    bool hasReceivedCommand() { return _firstCommandReceived; }

    ~UDPHandler() {
        if (sock >= 0) close(sock);
    }
};

#endif // UDP_HANDLER_H
