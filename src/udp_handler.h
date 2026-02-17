#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

// Use POSIX BSD sockets â€” works on ALL ESP32 network interfaces
// (Ethernet, WiFi, or both) via the lwIP stack directly.
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "config.h"
#include "segment_manager.h"

class UDPHandler {
private:
    int sock = -1;
    SegmentManager* segmentManager;
    uint8_t brightness;
    char packetBuffer[UDP_BUFFER_SIZE];

    // Convert hex color string (RRGGBB) to RGB565
    uint16_t parseColor(const char* colorStr) {
        if (colorStr[0] == '#') colorStr++;
        uint32_t color = strtoul(colorStr, nullptr, 16);
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

public:
    UDPHandler(SegmentManager* manager) : segmentManager(manager), brightness(DEFAULT_BRIGHTNESS) {}

    // Open a non-blocking UDP socket bound to UDP_PORT on all interfaces.
    bool begin() {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            Serial.printf("ERROR: socket() failed: %d\n", errno);
            return false;
        }

        // Allow port reuse
        int yes = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        // Non-blocking so process() can be polled from loop()
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(UDP_PORT);
        addr.sin_addr.s_addr = INADDR_ANY; // all interfaces

        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            Serial.printf("ERROR: bind() failed: %d\n", errno);
            close(sock);
            sock = -1;
            return false;
        }

        Serial.printf("âœ“ UDP socket bound on port %d (all interfaces)\n", UDP_PORT);
        return true;
    }

    // Poll for incoming packets â€” call from loop().
    void process() {
        if (sock < 0) return;

        struct sockaddr_in sender;
        socklen_t senderLen = sizeof(sender);

        int len = recvfrom(sock, packetBuffer, UDP_BUFFER_SIZE - 1, 0,
                           (struct sockaddr*)&sender, &senderLen);
        if (len <= 0) return; // EAGAIN / no packet

        packetBuffer[len] = '\0';

        // Strip trailing CR/LF
        for (int i = 0; i < len; i++) {
            if (packetBuffer[i] == '\n' || packetBuffer[i] == '\r') {
                packetBuffer[i] = '\0';
                break;
            }
        }

        Serial.printf("UDP from %s: %s\n",
                      inet_ntoa(sender.sin_addr), packetBuffer);
        dispatchCommand(packetBuffer);
    }

    // ---------------------------------------------------------------
    // Command routing
    // ---------------------------------------------------------------
    void dispatchCommand(char* cmd) {
        if      (strncmp(cmd, "TEXT|",       5)  == 0) parseTextCommand(cmd);
        else if (strncmp(cmd, "CLEAR|",      6)  == 0) parseClearCommand(cmd);
        else if (strcmp (cmd, "CLEAR_ALL")       == 0) segmentManager->clearAll();
        else if (strncmp(cmd, "BRIGHTNESS|", 11) == 0) parseBrightnessCommand(cmd);
        else if (strncmp(cmd, "CONFIG|",     7)  == 0) parseConfigCommand(cmd);
        else    Serial.printf("ERROR: Unknown command: %s\n", cmd);
    }

    // TEXT|segment|content|color|font|size|align|effect|bgcolor|intensity
    void parseTextCommand(char* cmd) {
        char* tokens[10] = {nullptr};
        int tokenCount = 0;
        char* token = strtok(cmd, "|");
        while (token && tokenCount < 10) { tokens[tokenCount++] = token; token = strtok(nullptr, "|"); }

        if (tokenCount < 3) { Serial.println("ERROR: TEXT needs at least segment+content"); return; }

        uint8_t segmentId = atoi(tokens[1]);
        if (segmentId >= MAX_SEGMENTS) { Serial.printf("ERROR: bad segment %d\n", segmentId); return; }

        char* content   = tokens[2];
        char* color     = (tokenCount > 3) ? tokens[3] : nullptr;
        char* font      = (tokenCount > 4) ? tokens[4] : nullptr;
        char* size      = (tokenCount > 5) ? tokens[5] : nullptr;
        char* align     = (tokenCount > 6) ? tokens[6] : nullptr;
        char* effect    = (tokenCount > 7) ? tokens[7] : nullptr;
        char* bgcolor   = (tokenCount > 8) ? tokens[8] : nullptr;
        char* intensity = (tokenCount > 9) ? tokens[9] : nullptr;

        Segment* seg = segmentManager->getSegment(segmentId);
        if (!seg) return;

        segmentManager->updateSegmentText(segmentId, content);
        segmentManager->activateSegment(segmentId, true);

        if (color)   seg->color   = parseColor(color);
        if (bgcolor) seg->bgColor = parseColor(bgcolor);
        if (font)    segmentManager->setSegmentFont(segmentId, font);

        if (size) {
            if (strcmp(size, "auto") == 0) {
                seg->autoSize = true;
            } else {
                int v = atoi(size);
                if (v > 0 && v <= 32) { seg->fontSize = v; seg->autoSize = false; }
            }
        }

        if (align) {
            if      (strcmp(align, "L") == 0) seg->align = ALIGN_LEFT;
            else if (strcmp(align, "C") == 0) seg->align = ALIGN_CENTER;
            else if (strcmp(align, "R") == 0) seg->align = ALIGN_RIGHT;
        }

        if (effect) {
            if      (strcmp(effect, "scroll")  == 0) seg->effect = EFFECT_SCROLL;
            else if (strcmp(effect, "blink")   == 0) seg->effect = EFFECT_BLINK;
            else if (strcmp(effect, "fade")    == 0) seg->effect = EFFECT_FADE;
            else if (strcmp(effect, "rainbow") == 0) seg->effect = EFFECT_RAINBOW;
            else                                     seg->effect = EFFECT_NONE;
        }

        if (intensity) {
            int v = atoi(intensity);
            if (v >= 0 && v <= 255) { /* per-segment intensity stored for future use */ }
        }

        seg->isDirty = true;
    }

    // CLEAR|segment
    void parseClearCommand(char* cmd) {
        strtok(cmd, "|");
        char* token = strtok(nullptr, "|");
        if (token) {
            uint8_t id = atoi(token);
            if (id < MAX_SEGMENTS) {
                segmentManager->clearSegment(id);
                segmentManager->activateSegment(id, false);
            } else {
                Serial.printf("ERROR: bad segment for CLEAR: %d\n", id);
            }
        }
    }

    // BRIGHTNESS|value
    void parseBrightnessCommand(char* cmd) {
        strtok(cmd, "|");
        char* token = strtok(nullptr, "|");
        if (token) {
            int v = atoi(token);
            if (v >= 0 && v <= 255) brightness = (uint8_t)v;
        }
    }

    // CONFIG|segment|id|property|value
    void parseConfigCommand(char* cmd) {
        char* tokens[5] = {nullptr};
        int tokenCount = 0;
        char* token = strtok(cmd, "|");
        while (token && tokenCount < 5) { tokens[tokenCount++] = token; token = strtok(nullptr, "|"); }

        if (tokenCount < 2) return;

        if (strcmp(tokens[1], "brightness") == 0 && tokenCount >= 3) {
            int v = atoi(tokens[2]);
            if (v >= 0 && v <= 255) brightness = (uint8_t)v;
        } else if (strcmp(tokens[1], "segment") == 0 && tokenCount >= 5) {
            uint8_t id = atoi(tokens[2]);
            if (id >= MAX_SEGMENTS) return;
            Segment* seg = segmentManager->getSegment(id);
            if (!seg) return;
            int v = atoi(tokens[4]);
            if      (strcmp(tokens[3], "x")      == 0) seg->x      = v;
            else if (strcmp(tokens[3], "y")      == 0) seg->y      = v;
            else if (strcmp(tokens[3], "width")  == 0) seg->width  = v;
            else if (strcmp(tokens[3], "height") == 0) seg->height = v;
            seg->isDirty = true;
        }
    }

    uint8_t getBrightness() { return brightness; }
};

#endif // UDP_HANDLER_H
