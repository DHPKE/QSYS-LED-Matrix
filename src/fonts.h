#ifndef FONTS_H
#define FONTS_H

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

// Font mapping structure
struct FontInfo {
    const char* name;
    const GFXfont* font;
    uint8_t height;
};

// Available fonts
const FontInfo FONTS[] = {
    {"arial", &FreeSans12pt7b, 12},        // Arial sans-serif (auto-sized)
    {"verdana", &FreeSans12pt7b, 12},      // Verdana sans-serif (auto-sized)
    {"digital12", &FreeMonoBold12pt7b, 12},
    {"digital24", &FreeSans24pt7b, 24},
    {"mono9", &FreeMonoBold9pt7b, 9},
    {"mono12", &FreeMonoBold12pt7b, 12},
};

const int FONT_COUNT = sizeof(FONTS) / sizeof(FontInfo);

// Get font by name
const GFXfont* getFontByName(const char* name) {
    for (int i = 0; i < FONT_COUNT; i++) {
        if (strcmp(FONTS[i].name, name) == 0) {
            return FONTS[i].font;
        }
    }
    return &FreeSans9pt7b; // Default font
}

// Get font height by name
uint8_t getFontHeight(const char* name) {
    for (int i = 0; i < FONT_COUNT; i++) {
        if (strcmp(FONTS[i].name, name) == 0) {
            return FONTS[i].height;
        }
    }
    return 9; // Default height
}

#endif // FONTS_H
