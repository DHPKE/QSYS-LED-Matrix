#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include "config.h"
#include "segment_manager.h"

class UDPHandler {
private:
    WiFiUDP udp;
    SegmentManager* segmentManager;
    uint8_t brightness;
    char packetBuffer[UDP_BUFFER_SIZE];
    
    // Parse hex color to uint16_t (RGB565)
    uint16_t parseColor(const char* colorStr) {
        if (colorStr[0] == '#') colorStr++;
        
        uint32_t color = strtoul(colorStr, nullptr, 16);
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        // Convert to RGB565
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    // Parse TEXT command
    // Format: TEXT|segment|content|color|font|size|align|effect
    void parseTextCommand(char* cmd) {
        char* token;
        int tokenIndex = 0;
        uint8_t segmentId = 0;
        char* content = nullptr;
        char* color = nullptr;
        char* font = nullptr;
        char* size = nullptr;
        char* align = nullptr;
        char* effect = nullptr;
        
        token = strtok(cmd, "|");
        while (token != nullptr && tokenIndex < 8) {
            switch (tokenIndex) {
                case 0: // Command (TEXT)
                    break;
                case 1: // Segment ID
                    segmentId = atoi(token);
                    break;
                case 2: // Content
                    content = token;
                    break;
                case 3: // Color
                    color = token;
                    break;
                case 4: // Font
                    font = token;
                    break;
                case 5: // Size
                    size = token;
                    break;
                case 6: // Align
                    align = token;
                    break;
                case 7: // Effect
                    effect = token;
                    break;
            }
            tokenIndex++;
            token = strtok(nullptr, "|");
        }
        
        // Apply to segment
        if (segmentId < MAX_SEGMENTS && content != nullptr) {
            Segment* seg = segmentManager->getSegment(segmentId);
            if (seg) {
                segmentManager->updateSegmentText(segmentId, content);
                segmentManager->activateSegment(segmentId, true);
                
                if (color) {
                    seg->color = parseColor(color);
                }
                
                if (font) {
                    segmentManager->setSegmentFont(segmentId, font);
                }
                
                if (size) {
                    if (strcmp(size, "auto") == 0) {
                        seg->autoSize = true;
                    } else {
                        seg->fontSize = atoi(size);
                        seg->autoSize = false;
                    }
                }
                
                if (align) {
                    if (strcmp(align, "L") == 0) seg->align = ALIGN_LEFT;
                    else if (strcmp(align, "C") == 0) seg->align = ALIGN_CENTER;
                    else if (strcmp(align, "R") == 0) seg->align = ALIGN_RIGHT;
                }
                
                if (effect) {
                    if (strcmp(effect, "scroll") == 0) seg->effect = EFFECT_SCROLL;
                    else if (strcmp(effect, "blink") == 0) seg->effect = EFFECT_BLINK;
                    else if (strcmp(effect, "fade") == 0) seg->effect = EFFECT_FADE;
                    else if (strcmp(effect, "rainbow") == 0) seg->effect = EFFECT_RAINBOW;
                    else seg->effect = EFFECT_NONE;
                }
                
                seg->isDirty = true;
            }
        }
    }
    
    // Parse CLEAR command
    void parseClearCommand(char* cmd) {
        char* token = strtok(cmd, "|");
        token = strtok(nullptr, "|"); // Get segment ID
        
        if (token) {
            uint8_t segmentId = atoi(token);
            segmentManager->clearSegment(segmentId);
        }
    }
    
    // Parse CONFIG/BRIGHTNESS command
    void parseConfigCommand(char* cmd) {
        char* token = strtok(cmd, "|");
        token = strtok(nullptr, "|"); // Get config type
        
        if (token && strcmp(token, "brightness") == 0) {
            token = strtok(nullptr, "|"); // Get value
            if (token) {
                brightness = atoi(token);
            }
        }
    }
    
public:
    UDPHandler(SegmentManager* manager) : segmentManager(manager), brightness(DEFAULT_BRIGHTNESS) {}
    
    bool begin() {
        return udp.begin(UDP_PORT);
    }
    
    void process() {
        int packetSize = udp.parsePacket();
        if (packetSize > 0) {
            int len = udp.read(packetBuffer, UDP_BUFFER_SIZE - 1);
            if (len > 0) {
                packetBuffer[len] = '\0'; // Null terminate
                
                // Remove newline/carriage return
                for (int i = 0; i < len; i++) {
                    if (packetBuffer[i] == '\n' || packetBuffer[i] == '\r') {
                        packetBuffer[i] = '\0';
                        break;
                    }
                }
                
                Serial.print("UDP packet received: ");
                Serial.println(packetBuffer);
                
                // Parse command
                if (strncmp(packetBuffer, "TEXT|", 5) == 0) {
                    parseTextCommand(packetBuffer);
                } else if (strncmp(packetBuffer, "CLEAR|", 6) == 0) {
                    parseClearCommand(packetBuffer);
                } else if (strcmp(packetBuffer, "CLEAR_ALL") == 0) {
                    segmentManager->clearAll();
                } else if (strncmp(packetBuffer, "CONFIG|", 7) == 0 || 
                          strncmp(packetBuffer, "BRIGHTNESS|", 11) == 0) {
                    parseConfigCommand(packetBuffer);
                }
            }
        }
    }
    
    uint8_t getBrightness() {
        return brightness;
    }
};

#endif // UDP_HANDLER_H
