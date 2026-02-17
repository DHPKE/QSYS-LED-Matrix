#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

// AsyncUDP works over both WiFi and Ethernet on ESP32 (interface-agnostic)
#include <AsyncUDP.h>
#include "config.h"
#include "segment_manager.h"

class UDPHandler {
private:
    AsyncUDP udp;
    SegmentManager* segmentManager;
    uint8_t brightness;
    char packetBuffer[UDP_BUFFER_SIZE];
    
    // Convert hex color to uint16_t (RGB565)
    uint16_t parseColor(const char* colorStr) {
        if (colorStr[0] == '#') colorStr++;
        
        uint32_t color = strtoul(colorStr, nullptr, 16);
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        // Convert to RGB565
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    
public:
    UDPHandler(SegmentManager* manager) : segmentManager(manager), brightness(DEFAULT_BRIGHTNESS) {}
    
    // Begin listening. AsyncUDP registers a packet callback so no poll
    // is needed in loop(). Safe to call after ETH/WiFi is connected.
    bool begin() {
        if (!udp.listen(UDP_PORT)) {
            return false;
        }
        // Packet callback runs in the network task context
        udp.onPacket([this](AsyncUDPPacket packet) {
            size_t len = packet.length();
            if (len == 0) return;
            if (len > UDP_BUFFER_SIZE - 1) {
                Serial.printf("WARNING: UDP packet too large (%u bytes), truncating\n", len);
                len = UDP_BUFFER_SIZE - 1;
            }
            memcpy(packetBuffer, packet.data(), len);
            packetBuffer[len] = '\0';

            // Strip trailing newline / carriage return
            for (size_t i = 0; i < len; i++) {
                if (packetBuffer[i] == '\n' || packetBuffer[i] == '\r') {
                    packetBuffer[i] = '\0';
                    break;
                }
            }

            Serial.print("UDP packet received: ");
            Serial.println(packetBuffer);
            dispatchCommand(packetBuffer);
        });
        return true;
    }

    // process() is a no-op with AsyncUDP (callbacks handle everything),
    // kept for API compatibility with the loop() call.
    void process() {}

    // ---------------------------------------------------------------
    // Command parsing
    // ---------------------------------------------------------------

    // Route the raw command string to the correct handler.
    void dispatchCommand(char* cmd) {
        if (strncmp(cmd, "TEXT|", 5) == 0) {
            parseTextCommand(cmd);
        } else if (strncmp(cmd, "CLEAR|", 6) == 0) {
            parseClearCommand(cmd);
        } else if (strcmp(cmd, "CLEAR_ALL") == 0) {
            segmentManager->clearAll();
        } else if (strncmp(cmd, "BRIGHTNESS|", 11) == 0) {
            parseBrightnessCommand(cmd);
        } else if (strncmp(cmd, "CONFIG|", 7) == 0) {
            parseConfigCommand(cmd);
        } else {
            Serial.print("ERROR: Unknown command: ");
            Serial.println(cmd);
        }
    }

    // Parse TEXT command with bounds checking
    // Format: TEXT|segment|content|color|font|size|align|effect|bgcolor|intensity
    void parseTextCommand(char* cmd) {
        char* tokens[10] = {nullptr};
        int tokenCount = 0;
        
        char* token = strtok(cmd, "|");
        while (token != nullptr && tokenCount < 10) {
            tokens[tokenCount++] = token;
            token = strtok(nullptr, "|");
        }
        
        if (tokenCount < 3) {
            Serial.println("ERROR: TEXT command requires at least segment and content");
            return;
        }
        
        uint8_t segmentId = atoi(tokens[1]);
        char* content  = tokens[2];
        char* color    = (tokenCount > 3) ? tokens[3] : nullptr;
        char* font     = (tokenCount > 4) ? tokens[4] : nullptr;
        char* size     = (tokenCount > 5) ? tokens[5] : nullptr;
        char* align    = (tokenCount > 6) ? tokens[6] : nullptr;
        char* effect   = (tokenCount > 7) ? tokens[7] : nullptr;
        char* bgcolor  = (tokenCount > 8) ? tokens[8] : nullptr;
        char* intensity = (tokenCount > 9) ? tokens[9] : nullptr;
        
        if (segmentId >= MAX_SEGMENTS) {
            Serial.printf("ERROR: Invalid segment ID: %d\n", segmentId);
            return;
        }
        
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
                int sizeVal = atoi(size);
                if (sizeVal > 0 && sizeVal <= 32) {
                    seg->fontSize = sizeVal;
                    seg->autoSize = false;
                }
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
            int val = atoi(intensity);
            if (val >= 0 && val <= 255) {
                // Store per-segment intensity; applied in renderSegment via setBrightness
                seg->fontSize = seg->autoSize ? seg->fontSize : seg->fontSize; // placeholder
                // Intensity is surfaced through the global brightness channel for now
                // (per-segment intensity requires dma_display->drawPixel level control)
            }
        }
        
        seg->isDirty = true;
    }
    
    // Parse CLEAR command: CLEAR|segment
    void parseClearCommand(char* cmd) {
        strtok(cmd, "|");                   // skip "CLEAR"
        char* token = strtok(nullptr, "|"); // segment id
        if (token) {
            uint8_t segmentId = atoi(token);
            if (segmentId < MAX_SEGMENTS) {
                segmentManager->clearSegment(segmentId);
                segmentManager->activateSegment(segmentId, false);
            } else {
                Serial.printf("ERROR: Invalid segment ID for CLEAR: %d\n", segmentId);
            }
        }
    }
    
    // Parse BRIGHTNESS command: BRIGHTNESS|value
    void parseBrightnessCommand(char* cmd) {
        strtok(cmd, "|");
        char* token = strtok(nullptr, "|");
        if (token) {
            int value = atoi(token);
            if (value >= 0 && value <= 255) {
                brightness = (uint8_t)value;
            }
        }
    }
    
    // Parse CONFIG command
    // Format: CONFIG|segment|id|property|value
    void parseConfigCommand(char* cmd) {
        char* tokens[5] = {nullptr};
        int tokenCount = 0;
        
        char* token = strtok(cmd, "|");
        while (token != nullptr && tokenCount < 5) {
            tokens[tokenCount++] = token;
            token = strtok(nullptr, "|");
        }
        
        if (tokenCount < 2) return;
        
        if (strcmp(tokens[1], "brightness") == 0 && tokenCount >= 3) {
            int value = atoi(tokens[2]);
            if (value >= 0 && value <= 255) brightness = (uint8_t)value;
        } else if (strcmp(tokens[1], "segment") == 0 && tokenCount >= 5) {
            uint8_t segmentId = atoi(tokens[2]);
            if (segmentId >= MAX_SEGMENTS) return;
            Segment* seg = segmentManager->getSegment(segmentId);
            if (!seg) return;
            
            char* property = tokens[3];
            int   value    = atoi(tokens[4]);
            
            if      (strcmp(property, "x")      == 0) seg->x      = value;
            else if (strcmp(property, "y")      == 0) seg->y      = value;
            else if (strcmp(property, "width")  == 0) seg->width  = value;
            else if (strcmp(property, "height") == 0) seg->height = value;
            seg->isDirty = true;
        }
    }
    
    uint8_t getBrightness() { return brightness; }
};

#endif // UDP_HANDLER_H
