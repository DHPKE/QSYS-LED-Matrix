#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>
#include "segment_manager.h"
#include "fonts.h"

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
        int textLen = strlen(text);
        if (textLen == 0) return 12;
        
        // Available space with padding
        int availWidth = seg->width - 4; // 2px padding each side
        int availHeight = seg->height - 4;
        
        // Try fonts from largest to smallest
        const uint8_t sizes[] = {24, 18, 12, 9, 6};
        
        for (int i = 0; i < 5; i++) {
            // Get the actual font for this size
            const GFXfont* testFont = selectFont(sizes[i], seg->fontName);
            dma_display->setFont(testFont);
            
            // Get actual text bounds
            int16_t x1, y1;
            uint16_t w, h;
            dma_display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
            
            // Check if text fits in available space
            if (w <= availWidth && h <= availHeight) {
                return sizes[i];
            }
        }
        
        return 6; // Minimum size if nothing fits
    }
    
    // Get appropriate font for size
    const GFXfont* selectFont(uint8_t size, const char* fontName) {
        // Handle auto-sizing for Arial and Verdana fonts
        if (fontName && (strcmp(fontName, "arial") == 0 || strcmp(fontName, "verdana") == 0)) {
            // Select size based on segment height
            if (size <= 9) {
                return &FreeSans9pt7b;
            } else if (size <= 12) {
                return &FreeSans12pt7b;
            } else if (size <= 18) {
                return &FreeSans18pt7b;
            } else {
                return &FreeSans24pt7b;
            }
        }
        
        // Otherwise use the font name directly
        return getFontByName(fontName);
    }
    
public:
    TextRenderer(MatrixPanel_I2S_DMA* display, SegmentManager* manager) 
        : dma_display(display), segmentManager(manager) {}
    
    void renderSegment(Segment* seg) {
        if (!seg->isActive || strlen(seg->text) == 0) {
            // Clear segment area
            dma_display->fillRect(seg->x, seg->y, seg->width, seg->height, seg->bgColor);
            seg->isDirty = false;
            return;
        }

        Serial.printf("RENDER seg%d: '%s' x=%d y=%d w=%d h=%d active=%d\n",
                      seg->id, seg->text, seg->x, seg->y, seg->width, seg->height, seg->isActive);

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
        
        // Calculate text bounds relative to cursor at (0,0).
        // For GFX bitmap fonts y1 is negative (ascent above baseline).
        // w/h are the bounding box dimensions.
        int16_t x1, y1;
        uint16_t w, h;
        dma_display->getTextBounds(seg->text, 0, 0, &x1, &y1, &w, &h);

        Serial.printf("  bounds: x1=%d y1=%d w=%u h=%u fontSize=%d\n", x1, y1, w, h, fontSize);

        // Calculate position based on alignment
        int16_t textX, textY;
        int padding = 2;
        
        // textX: place bounding box left edge at target X, then convert to cursor X
        // cursor_x = target_x - x1
        switch (seg->align) {
            case ALIGN_LEFT:
                textX = seg->x + padding - x1;
                break;
            case ALIGN_CENTER:
                textX = seg->x + (seg->width - (int16_t)w) / 2 - x1;
                break;
            case ALIGN_RIGHT:
                textX = seg->x + seg->width - (int16_t)w - padding - x1;
                break;
            default:
                textX = seg->x + padding - x1;
                break;
        }
        
        // Vertical centering for GFX fonts:
        // The bounding box top is at (cursor_y + y1), bottom at (cursor_y + y1 + h).
        // We want the box centred in the segment:
        //   box_top = seg->y + (seg->height - h) / 2
        //   cursor_y = box_top - y1
        int16_t boxTop = seg->y + ((int16_t)seg->height - (int16_t)h) / 2;
        textY = boxTop - y1;
        
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
