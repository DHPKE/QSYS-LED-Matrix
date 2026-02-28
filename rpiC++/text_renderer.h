// text_renderer.h - Text rendering with TrueType fonts and auto-sizing

#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <string>
#include "led-matrix.h"
#include "graphics.h"
#include "segment_manager.h"

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;

class TextRenderer {
public:
    TextRenderer(RGBMatrix* matrix, SegmentManager* segment_manager);
    ~TextRenderer();
    
    void renderAll();
    
private:
    RGBMatrix* matrix_;
    FrameCanvas* canvas_;
    SegmentManager* sm_;
    
    FT_Library ft_library_;
    FT_Face ft_face_;
    bool ft_initialized_;
    
    Orientation current_orientation_;
    int canvas_width_;
    int canvas_height_;
    
    uint8_t group_id_cache_;
    Color group_color_cache_;
    
    int render_count_;
    
    struct FontCache {
        std::map<int, FT_Face> faces;
    };
    FontCache font_cache_;
    
    struct TextMeasurement {
        int width;
        int height;
    };
    std::map<std::pair<std::string, int>, TextMeasurement> text_measurement_cache_;
    
    bool initFreeType();
    FT_Face loadFont(int size);
    TextMeasurement measureText(const std::string& text, int font_size);
    std::pair<int, TextMeasurement> fitText(const std::string& text, int max_w, int max_h);
    
    void renderSegment(const Segment& seg);
    void renderGroupIndicator();
    void drawFrame(const Segment& seg);
};

#endif // TEXT_RENDERER_H
