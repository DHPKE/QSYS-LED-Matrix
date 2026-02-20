#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>
#include "segment_manager.h"
#include "fonts.h"

class TextRenderer {
private:
    MatrixPanel_I2S_DMA* dma_display;
    SegmentManager*      segmentManager;

    // ── Font auto-size ──────────────────────────────────────────────────────────
    // Pick the largest GFX font whose rendered text fits within (maxW x maxH)
    // with a guaranteed 1-pixel margin on every side around the actual glyphs.
    // getFontById bucket thresholds: <=12->9pt  <=16->12pt  <=22->18pt  else->24pt
    // We probe at heights that land in each distinct bucket (30, 22, 16, 12),
    // then fall back to TomThumb if even 12pt does not fit.
    const GFXfont* fitFont(uint8_t fontId, const char* text,
                           uint16_t maxW, uint16_t maxH,
                           uint8_t& outPixelH) {
        // Discrete pixel-height candidates, largest first.
        // Each value triggers a different font file in getFontById.
        static const uint8_t heights[] = { 30, 22, 16, 12, 0 }; // 0 = TomThumb

        // Enforce 1-pixel margin on every side: subtract 2 from each dimension
        // (getTextBounds returns the tight glyph bbox, so this guarantees no
        // glyph pixel ever touches the segment border).
        const uint16_t MARGIN = 2; // 1 px each side
        if (maxW <= MARGIN || maxH <= MARGIN) { outPixelH = 0; return &TomThumb; }
        const uint16_t fitW = maxW - MARGIN;
        const uint16_t fitH = maxH - MARGIN;

        for (int i = 0; ; i++) {
            uint8_t ph = heights[i];
            const GFXfont* f = (ph == 0) ? &TomThumb : getFontById(fontId, ph);
            dma_display->setFont(f);
            int16_t x1, y1; uint16_t tw, th;
            dma_display->getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
            if (tw <= fitW && th <= fitH) {
                outPixelH = ph;
                return f;
            }
            if (ph == 0) {
                // TomThumb is the absolute last resort
                outPixelH = 0;
                return f;
            }
        }
    }

public:
    TextRenderer(MatrixPanel_I2S_DMA* display, SegmentManager* manager)
        : dma_display(display), segmentManager(manager) {}

    void renderSegment(Segment* seg) {
        seg->isDirty = false;

        if (!seg->isActive) {
            dma_display->fillRect(seg->x, seg->y, seg->width, seg->height, 0x0000);
            return;
        }

        if (strlen(seg->text) == 0) {
            dma_display->fillRect(seg->x, seg->y, seg->width, seg->height, seg->bgColor);
            return;
        }

        if (seg->effect == EFFECT_BLINK && !seg->blinkState) return;

        if (seg->effect != EFFECT_SCROLL || seg->scrollOffset == 0) {
            Serial.printf("RENDER seg%d: '%s' x=%d y=%d w=%d h=%d fx=%d\n",
                          seg->id, seg->text, seg->x, seg->y, seg->width, seg->height, seg->effect);
        }

        // â”€â”€ Clear background â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        dma_display->fillRect(seg->x, seg->y, seg->width, seg->height, seg->bgColor);

        if (seg->hasBorder)
            dma_display->drawRect(seg->x, seg->y, seg->width, seg->height, seg->borderColor);

        // â”€â”€ Font selection â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        // PAD = 1 used only for text cursor placement.
        // fitFont() already subtracts its own 1-px glyph margin from maxW/maxH.
        const int PAD = 1;
        uint16_t maxW = seg->width;
        uint16_t maxH = seg->height;

        uint8_t fontId   = parseFontId(seg->fontName);
        uint8_t pixelH   = 0;
        const GFXfont* font;

        if (seg->autoSize) {
            font = fitFont(fontId, seg->text, maxW, maxH, pixelH);
        } else {
            pixelH = seg->fontSize;
            font   = getFontById(fontId, pixelH);
            dma_display->setFont(font);
        }

        dma_display->setFont(font);

        // â”€â”€ Measure exact text bounds â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        int16_t x1, y1; uint16_t tw, th;
        dma_display->getTextBounds(seg->text, 0, 0, &x1, &y1, &tw, &th);

        if (seg->effect != EFFECT_SCROLL || seg->scrollOffset == 0)
            Serial.printf("  font=%s(%d) tw=%u th=%u\n", seg->fontName, pixelH, tw, th);

        // â”€â”€ Horizontal position (alignment) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        int16_t textX;
        switch (seg->align) {
            case ALIGN_LEFT:
                textX = seg->x + PAD - x1;
                break;
            case ALIGN_RIGHT:
                textX = seg->x + seg->width - (int16_t)tw - PAD - x1;
                break;
            default: // CENTER
                textX = seg->x + ((int16_t)seg->width - (int16_t)tw) / 2 - x1;
                break;
        }

        // â”€â”€ Vertical centering (baseline correction) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        // y1 from getTextBounds is negative (offset from cursor to top of glyph).
        // We want the glyph block centred in the segment.
        int16_t textY = seg->y + ((int16_t)seg->height - (int16_t)th) / 2 - y1;

        // â”€â”€ Scroll offset â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        if (seg->effect == EFFECT_SCROLL) {
            textX -= seg->scrollOffset;
            if (textX + (int16_t)tw < seg->x) seg->scrollOffset = 0;
        }

        // â”€â”€ Draw â€” single-pixel sharp text (no anti-aliasing on HUB75) â”€â”€â”€â”€â”€â”€â”€
        dma_display->setTextWrap(false);
        dma_display->setTextColor(seg->color);
        dma_display->setCursor(textX, textY);
        dma_display->print(seg->text);
    }

    void renderAll() {
        for (int i = 0; i < MAX_SEGMENTS; i++) {
            Segment* seg = segmentManager->getSegment(i);
            if (seg && seg->isDirty) renderSegment(seg);
        }
    }

    void clearDisplay() { dma_display->fillScreen(0); }
};

#endif // TEXT_RENDERER_H
