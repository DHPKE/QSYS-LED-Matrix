#ifndef FONTS_H
#define FONTS_H

#include <Adafruit_GFX.h>

// ── Font 1 → FreeSansBold  (bold strokes, high contrast on LED matrix)
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// ── Font 2 → FreeSans (regular weight, wider letterforms)
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>

// ── Font 3 → FreeMonoBold (fixed-width bold, good for digits)
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

// ── Tiny fallback for very small segments
#include <Fonts/TomThumb.h>

// ─────────────────────────────────────────────────────────────────────────────
// Font ID integer mapping (matches Q-SYS protocol)
// 1 = FreeSans Bold  (FreeSansBold)
// 2 = FreeSans       (FreeSans)
// 3 = FreeMono Bold  (FreeMonoBold)
// ─────────────────────────────────────────────────────────────────────────────

// Returns the best GFX font for a given font-id (1-3) and pixel-height budget.
// All returned fonts are bold or bold-weight to ensure sharp edges on LED matrix.
inline const GFXfont* getFontById(uint8_t fontId, uint8_t pixelHeight) {
    switch (fontId) {
        case 2: // Verdana — regular sans
            if (pixelHeight <= 12) return &FreeSans9pt7b;
            if (pixelHeight <= 16) return &FreeSans12pt7b;
            if (pixelHeight <= 22) return &FreeSans18pt7b;
            return &FreeSans24pt7b;

        case 3: // Impact — condensed mono bold
            if (pixelHeight <= 12) return &FreeMonoBold9pt7b;
            if (pixelHeight <= 16) return &FreeMonoBold12pt7b;
            if (pixelHeight <= 22) return &FreeMonoBold18pt7b;
            return &FreeMonoBold24pt7b;

        default: // 1 = Arial — bold sans (default)
            if (pixelHeight <= 12) return &FreeSansBold9pt7b;
            if (pixelHeight <= 16) return &FreeSansBold12pt7b;
            if (pixelHeight <= 22) return &FreeSansBold18pt7b;
            return &FreeSansBold24pt7b;
    }
}

// Parse a font name string OR integer string → font id (1/2/3)
inline uint8_t parseFontId(const char* name) {
    if (!name || name[0] == '\0') return 1;
    // Integer form: "1", "2", "3"
    if (name[0] >= '1' && name[0] <= '3' && name[1] == '\0') return (uint8_t)(name[0] - '0');
    // Name form — canonical names first, then legacy aliases
    if (strncasecmp(name, "freesansbold", 12) == 0) return 1;
    if (strncasecmp(name, "freesans",      8) == 0) return 2;  // must be after freesansbold
    if (strncasecmp(name, "freemonobold", 12) == 0) return 3;
    if (strncasecmp(name, "freemono",      8) == 0) return 3;
    // Legacy aliases
    if (strncasecmp(name, "arial",   5) == 0) return 1;
    if (strncasecmp(name, "bold",    4) == 0) return 1;
    if (strncasecmp(name, "verdana", 7) == 0) return 2;
    if (strncasecmp(name, "sans",    4) == 0) return 2;
    if (strncasecmp(name, "impact",  6) == 0) return 3;
    if (strncasecmp(name, "roboto",  6) == 0) return 1;
    if (strncasecmp(name, "mono",    4) == 0) return 3;
    if (strncasecmp(name, "digital", 7) == 0) return 3;
    return 1; // default = Arial
}

// Convenience: get font by name + pixel height
inline const GFXfont* getFontByName(const char* name, uint8_t pixelHeight = 12) {
    return getFontById(parseFontId(name), pixelHeight);
}

#endif // FONTS_H
