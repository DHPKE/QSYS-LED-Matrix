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
        // Segment 1: Default fullscreen (layout preset 1).
        // This matches what showIPOnDisplay() + applyLayoutPreset(1) sets up,
        // so the WebUI canvas immediately shows the correct geometry on first poll.
        segments[0].id = 0;
        segments[0].x = 0;
        segments[0].y = 0;
        segments[0].width = LED_MATRIX_WIDTH;   // full width
        segments[0].height = LED_MATRIX_HEIGHT;  // full height
        strcpy(segments[0].text, "");
        segments[0].color = 0xFFFF;
        segments[0].bgColor = 0x0000;
        segments[0].borderColor = 0xFFFF;
        strcpy(segments[0].fontName, "roboto12");
        segments[0].fontSize = 12;
        segments[0].autoSize = true;
        segments[0].align = ALIGN_CENTER;
        segments[0].effect = EFFECT_NONE;
        segments[0].hasBorder = false;
        segments[0].scrollOffset = 0;
        segments[0].lastScrollUpdate = 0;
        segments[0].blinkState = false;
        segments[0].lastBlinkUpdate = 0;
        segments[0].effectSpeed = DEFAULT_SCROLL_SPEED;
        segments[0].isDirty = false;
        segments[0].isActive = true;
        
        // Segment 2: Right half (horizontal split 1|2) OR Top half (vertical split 1/2)
        segments[1].id = 1;
        segments[1].x = LED_MATRIX_WIDTH / 2;      // 32 pixels
        segments[1].y = 0;
        segments[1].width = LED_MATRIX_WIDTH / 2;  // 32 pixels
        segments[1].height = LED_MATRIX_HEIGHT;     // 32 pixels   // 32 pixels
        strcpy(segments[1].text, "");
        segments[1].color = 0xFFFF;
        segments[1].bgColor = 0x0000;
        segments[1].borderColor = 0xFFFF;
        strcpy(segments[1].fontName, "roboto8");
        segments[1].fontSize = 8;
        segments[1].autoSize = true;
        segments[1].align = ALIGN_CENTER;
        segments[1].effect = EFFECT_NONE;
        segments[1].hasBorder = false;
        segments[1].scrollOffset = 0;
        segments[1].lastScrollUpdate = 0;
        segments[1].blinkState = false;
        segments[1].lastBlinkUpdate = 0;
        segments[1].effectSpeed = DEFAULT_SCROLL_SPEED;
        segments[1].isDirty = false;
        segments[1].isActive = false;
        
        // Segment 3: Bottom left (quad) OR Bottom half (vertical split)
        segments[2].id = 2;
        segments[2].x = 0;
        segments[2].y = LED_MATRIX_HEIGHT / 2;     // 16 pixels
        segments[2].width = LED_MATRIX_WIDTH / 2;  // 32 pixels
        segments[2].height = LED_MATRIX_HEIGHT / 2; // 16 pixels2; // 16 pixels
        strcpy(segments[2].text, "");
        segments[2].color = 0xFFFF;
        segments[2].bgColor = 0x0000;
        segments[2].borderColor = 0xFFFF;
        strcpy(segments[2].fontName, "roboto8");
        segments[2].fontSize = 8;
        segments[2].autoSize = true;
        segments[2].align = ALIGN_CENTER;
        segments[2].effect = EFFECT_NONE;
        segments[2].hasBorder = false;
        segments[2].scrollOffset = 0;
        segments[2].lastScrollUpdate = 0;
        segments[2].blinkState = false;
        segments[2].lastBlinkUpdate = 0;
        segments[2].effectSpeed = DEFAULT_SCROLL_SPEED;
        segments[2].isDirty = false;
        segments[2].isActive = false;
        
        // Segment 4: Bottom right (quad)
        segments[3].id = 3;
        segments[3].x = LED_MATRIX_WIDTH / 2;      // 32 pixels
        segments[3].y = LED_MATRIX_HEIGHT / 2;     // 16 pixels
        segments[3].width = LED_MATRIX_WIDTH / 2;  // 32 pixels
        segments[3].height = LED_MATRIX_HEIGHT / 2; // 16 pixels
        strcpy(segments[3].text, "");
        segments[3].color = 0xFFFF;
        segments[3].bgColor = 0x0000;
        segments[3].borderColor = 0xFFFF;
        strcpy(segments[3].fontName, "roboto6");
        segments[3].fontSize = 6;
        segments[3].autoSize = true;
        segments[3].align = ALIGN_CENTER;
        segments[3].effect = EFFECT_NONE;
        segments[3].hasBorder = false;
        segments[3].scrollOffset = 0;
        segments[3].lastScrollUpdate = 0;
        segments[3].blinkState = false;
        segments[3].lastBlinkUpdate = 0;
        segments[3].effectSpeed = DEFAULT_SCROLL_SPEED;
        segments[3].isDirty = false;
        segments[3].isActive = false;
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
            segments[id].isActive = false;  // deactivate so it stops rendering
            segments[id].isDirty  = true;   // mark dirty so background is erased
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
