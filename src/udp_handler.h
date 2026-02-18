#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

// UDP handler: JSON protocol over POSIX BSD sockets (lwIP)
// Accepts commands from Q-SYS plugin via UDP on port UDP_PORT.
//
// Supported JSON commands:
//   {"cmd":"text","seg":0,"text":"Hello","color":"FFFFFF","bgcolor":"000000",
//    "font":"arial","size":"auto","align":"C","effect":"none","intensity":255}
//   {"cmd":"clear","seg":0}
//   {"cmd":"clear_all"}
//   {"cmd":"brightness","value":200}
//   {"cmd":"config","seg":0,"x":0,"y":0,"w":64,"h":32}

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <ArduinoJson.h>
#include "config.h"
#include "segment_manager.h"

// Convert 24-bit RGB hex string ("RRGGBB") to RGB565 uint16_t
static uint16_t rgb888to565(uint32_t c) {
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >>  8) & 0xFF;
    uint8_t b = (c      ) & 0xFF;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static uint16_t hexToColor565(const char* hex) {
    if (!hex || hex[0] == '\0') return 0x0000;
    const char* p = (hex[0] == '#') ? hex + 1 : hex;
    uint32_t c = (uint32_t)strtol(p, nullptr, 16);
    return rgb888to565(c);
}

static TextAlign parseAlign(const char* a) {
    if (!a) return ALIGN_CENTER;
    if (a[0] == 'L' || a[0] == 'l') return ALIGN_LEFT;
    if (a[0] == 'R' || a[0] == 'r') return ALIGN_RIGHT;
    return ALIGN_CENTER;
}

static TextEffect parseEffect(const char* e) {
    if (!e) return EFFECT_NONE;
    if (strcasecmp(e, "scroll") == 0) return EFFECT_SCROLL;
    if (strcasecmp(e, "blink") == 0)  return EFFECT_BLINK;
    if (strcasecmp(e, "fade") == 0)   return EFFECT_FADE;
    if (strcasecmp(e, "rainbow") == 0) return EFFECT_RAINBOW;
    return EFFECT_NONE;
}

class UDPHandler {
private:
    int sock = -1;
    SegmentManager* segmentManager;
    uint8_t brightness;
    char packetBuffer[UDP_BUFFER_SIZE];

public:
    UDPHandler(SegmentManager* sm) : segmentManager(sm), brightness(255) {}

    // Public so web test handler in main.cpp can call it directly
    void dispatchCommand(const char* raw) {
        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, raw);
        if (err) {
            Serial.printf("[UDP] JSON parse error: %s  raw: %.120s\n", err.c_str(), raw);
            return;
        }

        const char* cmd = doc["cmd"] | "";
        Serial.printf("[UDP] cmd=%s\n", cmd);

        if (strcmp(cmd, "text") == 0) {
            int         seg    = doc["seg"]       | 0;
            const char* txt    = doc["text"]      | "";
            const char* col    = doc["color"]     | "FFFFFF";
            const char* bg     = doc["bgcolor"]   | "000000";
            const char* fnt    = doc["font"]      | "arial";
            const char* sz     = doc["size"]      | "auto";
            const char* aln    = doc["align"]     | "C";
            const char* fx     = doc["effect"]    | "none";
            int         intens = doc["intensity"] | 255;

            Serial.printf("[UDP] TEXT seg%d \"%s\" col=%s bg=%s font=%s sz=%s al=%s fx=%s i=%d\n",
                          seg, txt, col, bg, fnt, sz, aln, fx, intens);

            if (segmentManager && seg >= 0 && seg < MAX_SEGMENTS) {
                Segment* s = segmentManager->getSegment((uint8_t)seg);
                if (s) {
                    strncpy(s->text, txt, MAX_TEXT_LENGTH - 1);
                    s->text[MAX_TEXT_LENGTH - 1] = '\0';
                    s->color   = hexToColor565(col);
                    s->bgColor = hexToColor565(bg);
                    strncpy(s->fontName, fnt, 15);
                    s->fontName[15] = '\0';
                    s->autoSize = (strcasecmp(sz, "auto") == 0);
                    if (!s->autoSize) {
                        if (strcasecmp(sz, "small") == 0)       s->fontSize = 6;
                        else if (strcasecmp(sz, "medium") == 0) s->fontSize = 12;
                        else if (strcasecmp(sz, "large") == 0)  s->fontSize = 18;
                        else s->fontSize = (uint8_t)atoi(sz);
                    }
                    s->align   = parseAlign(aln);
                    s->effect  = parseEffect(fx);
                    s->isActive = true;
                    s->isDirty  = true;
                }
            }

        } else if (strcmp(cmd, "clear") == 0) {
            int seg = doc["seg"] | 0;
            Serial.printf("[UDP] CLEAR seg%d\n", seg);
            if (segmentManager) segmentManager->clearSegment((uint8_t)seg);

        } else if (strcmp(cmd, "clear_all") == 0) {
            Serial.println("[UDP] CLEAR ALL");
            if (segmentManager) segmentManager->clearAll();

        } else if (strcmp(cmd, "brightness") == 0) {
            int val = doc["value"] | -1;
            if (val >= 0 && val <= 255) {
                brightness = (uint8_t)val;
                Serial.printf("[UDP] BRIGHTNESS %d\n", brightness);
            }

        } else if (strcmp(cmd, "config") == 0) {
            int seg = doc["seg"] | 0;
            int x   = doc["x"]   | 0;
            int y   = doc["y"]   | 0;
            int w   = doc["w"]   | 64;
            int h   = doc["h"]   | 32;
            Serial.printf("[UDP] CONFIG seg%d x=%d y=%d w=%d h=%d\n", seg, x, y, w, h);
            if (segmentManager && seg >= 0 && seg < MAX_SEGMENTS) {
                Segment* s = segmentManager->getSegment((uint8_t)seg);
                if (s) {
                    s->x      = (int16_t)x;
                    s->y      = (int16_t)y;
                    s->width  = (uint16_t)w;
                    s->height = (uint16_t)h;
                    // Do NOT activate or mark dirty here.
                    // A segment only renders when a "text" command arrives.
                    // A "clear" command will deactivate it.
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

    ~UDPHandler() {
        if (sock >= 0) close(sock);
    }
};

#endif // UDP_HANDLER_H
