#ifndef SEGMENT_MANAGER_H
#define SEGMENT_MANAGER_H

#include <Arduino.h>
#include "config.h"

enum TextAlign {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

enum TextEffect {
    EFFECT_NONE,
    EFFECT_SCROLL,
    EFFECT_BLINK,
    EFFECT_FADE,
    EFFECT_RAINBOW
};

struct Segment {
    uint8_t id;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    
    char text[MAX_TEXT_LENGTH];
    uint16_t color;
    uint16_t bgColor;
    uint16_t borderColor;
    
    char fontName[16];
    uint8_t fontSize;
    bool autoSize;
    
    TextAlign align;
    TextEffect effect;
    bool hasBorder;
    
    // Scrolling state
    int16_t scrollOffset;
    unsigned long lastScrollUpdate;
    
    // Blinking state
    bool blinkState;
    unsigned long lastBlinkUpdate;
    
    // Effect parameters
    uint16_t effectSpeed;
    
    bool isDirty;
    bool isActive;
};

class SegmentManager {
private:
    Segment segments[MAX_SEGMENTS];
    
public:
    SegmentManager() {
        // Initialize default layout
        initDefaultLayout();
    }
    
    void initDefaultLayout() {
        // Full screen segment
        segments[0] = {0, 0, 0, LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT, 
                      "", 0xFFFF, 0x0000, 0xFFFF,
                      "roboto12", 12, true,
                      ALIGN_CENTER, EFFECT_NONE, false,
                      0, 0, false, 0,
                      DEFAULT_SCROLL_SPEED, false, true};
        
        // Top half segment
        segments[1] = {1, 0, 0, LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT/2,
                      "", 0xFFFF, 0x0000, 0xFFFF,
                      "roboto8", 8, true,
                      ALIGN_CENTER, EFFECT_NONE, false,
                      0, 0, false, 0,
                      DEFAULT_SCROLL_SPEED, false, false};
        
        // Bottom half segment
        segments[2] = {2, 0, LED_MATRIX_HEIGHT/2, LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT/2,
                      "", 0xFFFF, 0x0000, 0xFFFF,
                      "roboto8", 8, true,
                      ALIGN_CENTER, EFFECT_NONE, false,
                      0, 0, false, 0,
                      DEFAULT_SCROLL_SPEED, false, false};
        
        // Custom segment
        segments[3] = {3, 0, 0, LED_MATRIX_WIDTH/2, LED_MATRIX_HEIGHT/2,
                      "", 0xFFFF, 0x0000, 0xFFFF,
                      "roboto6", 6, true,
                      ALIGN_CENTER, EFFECT_NONE, false,
                      0, 0, false, 0,
                      DEFAULT_SCROLL_SPEED, false, false};
    }
    
    Segment* getSegment(uint8_t id) {
        if (id < MAX_SEGMENTS) {
            return &segments[id];
        }
        return nullptr;
    }
    
    void updateSegmentText(uint8_t id, const char* text) {
        if (id < MAX_SEGMENTS) {
            strncpy(segments[id].text, text, MAX_TEXT_LENGTH - 1);
            segments[id].text[MAX_TEXT_LENGTH - 1] = '\0';
            segments[id].isDirty = true;
        }
    }
    
    void clearSegment(uint8_t id) {
        if (id < MAX_SEGMENTS) {
            segments[id].text[0] = '\0';
            segments[id].isDirty = true;
        }
    }
    
    void clearAll() {
        for (int i = 0; i < MAX_SEGMENTS; i++) {
            clearSegment(i);
        }
    }
    
    void setSegmentColor(uint8_t id, uint16_t color) {
        if (id < MAX_SEGMENTS) {
            segments[id].color = color;
            segments[id].isDirty = true;
        }
    }
    
    void setSegmentFont(uint8_t id, const char* fontName) {
        if (id < MAX_SEGMENTS) {
            strncpy(segments[id].fontName, fontName, 15);
            segments[id].fontName[15] = '\0';
            segments[id].isDirty = true;
        }
    }
    
    void setSegmentEffect(uint8_t id, TextEffect effect) {
        if (id < MAX_SEGMENTS) {
            segments[id].effect = effect;
            segments[id].isDirty = true;
        }
    }
    
    void activateSegment(uint8_t id, bool active) {
        if (id < MAX_SEGMENTS) {
            segments[id].isActive = active;
            segments[id].isDirty = true;
        }
    }
    
    void updateEffects() {
        unsigned long now = millis();
        
        for (int i = 0; i < MAX_SEGMENTS; i++) {
            if (!segments[i].isActive) continue;
            
            switch (segments[i].effect) {
                case EFFECT_SCROLL:
                    if (now - segments[i].lastScrollUpdate > (1000 / segments[i].effectSpeed)) {
                        segments[i].scrollOffset++;
                        segments[i].lastScrollUpdate = now;
                        segments[i].isDirty = true;
                    }
                    break;
                    
                case EFFECT_BLINK:
                    if (now - segments[i].lastBlinkUpdate > 500) {
                        segments[i].blinkState = !segments[i].blinkState;
                        segments[i].lastBlinkUpdate = now;
                        segments[i].isDirty = true;
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
};

#endif // SEGMENT_MANAGER_H
