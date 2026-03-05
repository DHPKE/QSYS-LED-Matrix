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
    
    bool begin() {
        return udp.begin(UDP_PORT);
    }
    
    // Parse TEXT command with bounds checking
    // Format: TEXT|segment|content|color|font|size|align|effect
    void parseTextCommand(char* cmd) {
        char* tokens[8] = {nullptr};
        int tokenCount = 0;
        
        // Safely tokenize with bounds checking
        char* token = strtok(cmd, "|");
        while (token != nullptr && tokenCount < 8) {
            tokens[tokenCount++] = token;
            token = strtok(nullptr, "|");
        }
        
        // Validate minimum required tokens (command, segment, content)
        if (tokenCount < 3) {
            Serial.println("ERROR: TEXT command requires at least segment and content");
            return;
        }
        
        // Parse tokens safely
        uint8_t segmentId = atoi(tokens[1]);
        char* content = tokens[2];
        char* color = (tokenCount > 3) ? tokens[3] : nullptr;
        char* font = (tokenCount > 4) ? tokens[4] : nullptr;
        char* size = (tokenCount > 5) ? tokens[5] : nullptr;
        char* align = (tokenCount > 6) ? tokens[6] : nullptr;
        char* effect = (tokenCount > 7) ? tokens[7] : nullptr;
        
        // Validate segment ID
        if (segmentId >= MAX_SEGMENTS) {
            Serial.print("ERROR: Invalid segment ID: ");
            Serial.println(segmentId);
            return;
        }
        
        // Apply to segment
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
                    int sizeVal = atoi(size);
                    if (sizeVal > 0 && sizeVal <= 32) {
                        seg->fontSize = sizeVal;
                        seg->autoSize = false;
                    }
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
    
    // Parse CLEAR command with validation
    void parseClearCommand(char* cmd) {
        char* token = strtok(cmd, "|");
        if (!token) return;
        
        token = strtok(nullptr, "|"); // Get segment ID
        if (token) {
            uint8_t segmentId = atoi(token);
            if (segmentId < MAX_SEGMENTS) {
                segmentManager->clearSegment(segmentId);
            } else {
                Serial.print("ERROR: Invalid segment ID for CLEAR: ");
                Serial.println(segmentId);
            }
        }
    }
    
    // Parse BRIGHTNESS command
    // Format: BRIGHTNESS|value
    void parseBrightnessCommand(char* cmd) {
        char* token = strtok(cmd, "|");
        if (!token) return;
        
        token = strtok(nullptr, "|"); // Get value
        if (token) {
            int value = atoi(token);
            // Validate range
            if (value >= 0 && value <= 255) {
                brightness = (uint8_t)value;
            }
        }
    }
    
    // Parse CONFIG command (legacy support)
    // Format: CONFIG|brightness|value
    void parseConfigCommand(char* cmd) {
        char* token = strtok(cmd, "|");
        if (!token) return;
        
        token = strtok(nullptr, "|"); // Get config type
        if (token && strcmp(token, "brightness") == 0) {
            token = strtok(nullptr, "|"); // Get value
            if (token) {
                int value = atoi(token);
                if (value >= 0 && value <= 255) {
                    brightness = (uint8_t)value;
                }
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
            // Bounds check packet size
            if (packetSize > UDP_BUFFER_SIZE - 1) {
                Serial.print("WARNING: UDP packet too large (");
                Serial.print(packetSize);
                Serial.println(" bytes), truncating");
                packetSize = UDP_BUFFER_SIZE - 1;
            }
            
            int len = udp.read(packetBuffer, packetSize);
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
                
                // Parse command with proper routing
                if (strncmp(packetBuffer, "TEXT|", 5) == 0) {
                    parseTextCommand(packetBuffer);
                } else if (strncmp(packetBuffer, "CLEAR|", 6) == 0) {
                    parseClearCommand(packetBuffer);
                } else if (strcmp(packetBuffer, "CLEAR_ALL") == 0) {
                    segmentManager->clearAll();
                } else if (strncmp(packetBuffer, "BRIGHTNESS|", 11) == 0) {
                    parseBrightnessCommand(packetBuffer);
                } else if (strncmp(packetBuffer, "CONFIG|", 7) == 0) {
                    parseConfigCommand(packetBuffer);
                } else {
                    Serial.print("ERROR: Unknown command: ");
                    Serial.println(packetBuffer);
                }
            }
        }
    }
    
    uint8_t getBrightness() {
        return brightness;
    }
};

#endif // UDP_HANDLER_H
