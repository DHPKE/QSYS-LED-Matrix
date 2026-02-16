#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>
#include "segment_manager.h"
#include "../lib/fonts/fonts.h"

class TextRenderer {
private:
    MatrixPanel_I2S_DMA* dma_display;
    SegmentManager* segmentManager;
    
    // Convert RGB888 to RGB565
    uint16_t color888to565(uint32_t color888) {
        uint8_t r = (color888 >> 16) & 0xFF;
        uint8_t g = (color888 >> 8) & 0xFF;
        uint8_t b = color888 & 0xFF;
        return dma_display->color565(r, g, b);
    }
    
    // Parse hex color string (RRGGBB)
    uint16_t parseColor(const char* colorStr) {
        if (colorStr[0] == '#') colorStr++; // Skip # if present
        
        uint32_t color = strtoul(colorStr, nullptr, 16);
        return color888to565(color);
    }
    
    // Calculate optimal font size for text to fit in segment
    uint8_t calculateAutoSize(Segment* seg, const char* text) {
        // Simple auto-sizing algorithm
        int textLen = strlen(text);
        if (textLen == 0) return 12;
        
        // Estimate character width (rough approximation)
        int availWidth = seg->width - 4; // 2px padding each side
        int availHeight = seg->height - 4;
        
        // Try fonts from largest to smallest
        const uint8_t sizes[] = {24, 18, 12, 9, 6};
        for (int i = 0; i < 5; i++) {
            int charWidth = sizes[i] * 0.6; // Rough estimate
            int totalWidth = textLen * charWidth;
            
            if (totalWidth <= availWidth && sizes[i] <= availHeight) {
                return sizes[i];
            }
        }
        
        return 6; // Minimum size
    }
    
    // Get appropriate font for size
    const GFXfont* selectFont(uint8_t size, const char* fontName) {
        // Map size to closest available font
        if (size <= 9) {
            return getFontByName("roboto8");
        } else if (size <= 12) {
            return getFontByName("roboto12");
        } else if (size <= 18) {
            return getFontByName("roboto16");
        } else {
            return getFontByName("roboto24");
        }
    }
    
public:
    TextRenderer(MatrixPanel_I2S_DMA* display, SegmentManager* manager) 
        : dma_display(display), segmentManager(manager) {}
    
    void renderSegment(Segment* seg) {
        if (!seg->isActive || strlen(seg->text) == 0) {
            // Clear segment area
            dma_display->fillRect(seg->x, seg->y, seg->width, seg->height, seg->bgColor);
            return;
        }
        
        // Clear segment background
        dma_display->fillRect(seg->x, seg->y, seg->width, seg->height, seg->bgColor);
        
        // Draw border if enabled
        if (seg->hasBorder) {
            dma_display->drawRect(seg->x, seg->y, seg->width, seg->height, seg->borderColor);
        }
        
        // Select and set font
        uint8_t fontSize = seg->autoSize ? calculateAutoSize(seg, seg->text) : seg->fontSize;
        const GFXfont* font = selectFont(fontSize, seg->fontName);
        dma_display->setFont(font);
        
        // Calculate text bounds
        int16_t x1, y1;
        uint16_t w, h;
        dma_display->getTextBounds(seg->text, 0, 0, &x1, &y1, &w, &h);
        
        // Calculate position based on alignment
        int16_t textX, textY;
        int padding = 2;
        
        switch (seg->align) {
            case ALIGN_LEFT:
                textX = seg->x + padding;
                break;
            case ALIGN_CENTER:
                textX = seg->x + (seg->width - w) / 2;
                break;
            case ALIGN_RIGHT:
                textX = seg->x + seg->width - w - padding;
                break;
        }
        
        // Vertical center
        textY = seg->y + (seg->height + h) / 2 - y1;
        
        // Apply scrolling offset
        if (seg->effect == EFFECT_SCROLL) {
            textX -= seg->scrollOffset;
            // Reset scroll when text completely off screen
            if (textX + w < seg->x) {
                seg->scrollOffset = 0;
            }
        }
        
        // Apply blink effect
        if (seg->effect == EFFECT_BLINK && !seg->blinkState) {
            return; // Don't draw when blinked off
        }
        
        // Draw text
        dma_display->setTextColor(seg->color);
        dma_display->setCursor(textX, textY);
        dma_display->print(seg->text);
        
        seg->isDirty = false;
    }
    
    void renderAll() {
        for (int i = 0; i < MAX_SEGMENTS; i++) {
            Segment* seg = segmentManager->getSegment(i);
            if (seg && seg->isDirty) {
                renderSegment(seg);
            }
        }
    }
    
    void clearDisplay() {
        dma_display->fillScreen(0);
    }
};

#endif // TEXT_RENDERER_H
