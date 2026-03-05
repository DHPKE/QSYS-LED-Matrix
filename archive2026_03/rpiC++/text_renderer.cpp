// text_renderer.cpp - Text rendering implementation

#include "text_renderer.h"
#include "udp_handler.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

extern UDPHandler* g_udp_handler;  // Declared in main.cpp

TextRenderer::TextRenderer(RGBMatrix* matrix, SegmentManager* segment_manager)
    : matrix_(matrix),
      canvas_(matrix->CreateFrameCanvas()),
      sm_(segment_manager),
      ft_initialized_(false),
      current_orientation_(LANDSCAPE),
      canvas_width_(MATRIX_WIDTH),
      canvas_height_(MATRIX_HEIGHT),
      last_layout_(0),
      group_id_cache_(0),
      group_color_cache_(0, 0, 0),
      render_count_(0) {
    
    ft_initialized_ = initFreeType();
    if (!ft_initialized_) {
        std::cerr << "[RENDER] FreeType initialization failed" << std::endl;
    }
}

TextRenderer::~TextRenderer() {
    if (ft_initialized_) {
        // Clean up cached font faces
        for (auto& pair : font_cache_) {
            FT_Done_Face(pair.second);
        }
        FT_Done_FreeType(ft_library_);
    }
}

bool TextRenderer::initFreeType() {
    if (FT_Init_FreeType(&ft_library_)) {
        return false;
    }
    
    // Try to load default font
    if (FT_New_Face(ft_library_, FONT_PATH, 0, &ft_face_)) {
        if (FT_New_Face(ft_library_, FONT_PATH_FALLBACK, 0, &ft_face_)) {
            FT_Done_FreeType(ft_library_);
            return false;
        }
    }
    
    return true;
}

FT_Face TextRenderer::loadFont(const std::string& font_name, int size) {
    // Check cache first
    FontCacheKey key = {font_name, size};
    auto it = font_cache_.find(key);
    if (it != font_cache_.end()) {
        return it->second;
    }
    
    // Determine font path
    const char* font_path;
    if (font_name == "monospace" || font_name == "mono") {
        font_path = FONT_MONO_PATH;
    } else {
        // Default to Arial
        font_path = FONT_PATH;
    }
    
    // Load new face at this size
    FT_Face face;
    if (FT_New_Face(ft_library_, font_path, 0, &face)) {
        // Fallback to DejaVu Sans
        if (FT_New_Face(ft_library_, FONT_PATH_FALLBACK, 0, &face)) {
            std::cerr << "[RENDER] Failed to load font: " << font_path << std::endl;
            return nullptr;
        }
    }
    
    FT_Set_Pixel_Sizes(face, 0, size);
    font_cache_[key] = face;
    return face;
}

TextRenderer::TextMeasurement TextRenderer::measureText(const std::string& text, const std::string& font_name, int font_size) {
    // Check cache
    auto key = std::make_pair(text + ":" + font_name, font_size);
    auto it = text_measurement_cache_.find(key);
    if (it != text_measurement_cache_.end()) {
        return it->second;
    }
    
    // Measure text
    FT_Face face = loadFont(font_name, font_size);
    if (!face) {
        return {0, 0};
    }
    
    int total_width = 0;
    int max_height = 0;
    
    for (char c : text) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }
        total_width += face->glyph->advance.x >> 6;
        int h = face->glyph->bitmap.rows;
        if (h > max_height) max_height = h;
    }
    
    TextMeasurement result = {total_width, max_height};
    text_measurement_cache_[key] = result;
    return result;
}

std::pair<int, TextRenderer::TextMeasurement> TextRenderer::fitText(const std::string& text, const std::string& font_name, int max_w, int max_h) {
    for (int i = 0; i < FONT_SIZES_COUNT; i++) {
        int size = FONT_SIZES[i];
        TextMeasurement meas = measureText(text, font_name, size);
        
        if (meas.width <= max_w && meas.height <= max_h) {
            return {size, meas};
        }
    }
    
    // Fallback to smallest
    int size = FONT_SIZES[FONT_SIZES_COUNT - 1];
    return {size, measureText(text, font_name, size)};
}

void TextRenderer::renderAll() {
    // Get snapshot
    bool any_dirty;
    std::vector<Segment> snapshots = sm_->getRenderSnapshot(any_dirty);
    
    if (!any_dirty) {
        return;
    }
    
    // Update canvas dimensions if orientation changed
    if (g_udp_handler) {
        Orientation orient = g_udp_handler->getOrientation();
        int current_layout = g_udp_handler->getCurrentLayout();
        
        // Check if orientation changed
        if (orient != current_orientation_) {
            current_orientation_ = orient;
            if (orient == PORTRAIT) {
                canvas_width_ = MATRIX_HEIGHT;
                canvas_height_ = MATRIX_WIDTH;
            } else {
                canvas_width_ = MATRIX_WIDTH;
                canvas_height_ = MATRIX_HEIGHT;
            }
            std::cout << "[RENDER] Canvas resized to " << canvas_width_ << "×" 
                     << canvas_height_ << " for " 
                     << (orient == PORTRAIT ? "portrait" : "landscape") << " mode" << std::endl;
            
            // Full clear needed when orientation changes
            canvas_->Fill(0, 0, 0);
        }
        // Check if layout changed
        else if (current_layout != last_layout_) {
            last_layout_ = current_layout;
            // Full clear needed when layout changes to remove old segments
            canvas_->Fill(0, 0, 0);
            std::cout << "[RENDER] Layout changed to " << current_layout << " - canvas cleared" << std::endl;
        }
        // Otherwise: NO full clear - segments fill their own backgrounds
    }
    
    // Render all segments that are in the current layout
    int rendered_count = 0;
    for (const auto& seg : snapshots) {
        if (!seg.is_active) continue;
        
        // Skip segments not in current layout (1x1 dummy rects)
        if (seg.width <= 1 || seg.height <= 1) continue;
        
        renderSegment(seg);
        rendered_count++;
    }
    
    // Render group indicator
    renderGroupIndicator();
    
    // Swap canvas
    canvas_ = matrix_->SwapOnVSync(canvas_);
    
    // Clear dirty flags
    sm_->clearDirtyFlags();
    
    // Logging (throttled)
    render_count_++;
    if (render_count_ % 500 == 0) {
        std::cout << "[RENDER] Rendered " << rendered_count << " segments (count: " 
                 << render_count_ << ")" << std::endl;
    }
}

void TextRenderer::renderSegment(const Segment& seg) {
    // Skip background fill if bgcolor is (1,1,1) - transparent marker for test mode
    bool skip_background = (seg.bgcolor.r == 1 && seg.bgcolor.g == 1 && seg.bgcolor.b == 1);
    
    if (!skip_background) {
        // Fill background
        for (int y = seg.y; y < seg.y + seg.height; y++) {
            for (int x = seg.x; x < seg.x + seg.width; x++) {
                canvas_->SetPixel(x, y, seg.bgcolor.r, seg.bgcolor.g, seg.bgcolor.b);
            }
        }
    }
    
    if (seg.text.empty()) {
        if (seg.frame_enabled) {
            drawFrame(seg);
        }
        return;
    }
    
    // Handle blink effect
    if (seg.effect == EFFECT_BLINK && !seg.blink_state) {
        if (seg.frame_enabled) {
            drawFrame(seg);
        }
        return;
    }
    
    // Auto-fit font (1px margin)
    int avail_w = std::max(1, seg.width - 2);
    int avail_h = std::max(1, seg.height - 2);
    
    auto [font_size, meas] = fitText(seg.text, seg.font_name, avail_w, avail_h);
    FT_Face face = loadFont(seg.font_name, font_size);
    if (!face) {
        return;
    }
    
    // Calculate text position
    int tx, ty;
    
    if (seg.align == ALIGN_LEFT) {
        tx = seg.x + 1;
    } else if (seg.align == ALIGN_RIGHT) {
        tx = seg.x + seg.width - meas.width - 1;
    } else {  // CENTER
        tx = seg.x + (seg.width - meas.width) / 2;
    }
    
    ty = seg.y + (seg.height - meas.height) / 2;
    
    // Handle scroll effect
    if (seg.effect == EFFECT_SCROLL) {
        int total_scroll = meas.width + seg.width;
        int offset = seg.scroll_offset % total_scroll;
        tx = seg.x + seg.width - offset;
    }
    
    // Render each character
    int pen_x = tx;
    int pen_y = ty + meas.height;  // Baseline
    
    for (char c : seg.text) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }
        
        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap& bitmap = slot->bitmap;
        
        int glyph_x = pen_x + slot->bitmap_left;
        int glyph_y = pen_y - slot->bitmap_top;
        
        // Render glyph with binary threshold (sharp rendering)
        for (unsigned int by = 0; by < bitmap.rows; by++) {
            for (unsigned int bx = 0; bx < bitmap.width; bx++) {
                int px = glyph_x + bx;
                int py = glyph_y + by;
                
                // Clip to segment bounds
                if (px < seg.x || px >= seg.x + seg.width ||
                    py < seg.y || py >= seg.y + seg.height) {
                    continue;
                }
                
                unsigned char gray = bitmap.buffer[by * bitmap.width + bx];
                
                // Binary threshold at 128 for sharp edges
                if (gray > 128) {
                    canvas_->SetPixel(px, py, seg.color.r, seg.color.g, seg.color.b);
                }
            }
        }
        
        pen_x += slot->advance.x >> 6;
    }
    
    // Draw frame if enabled
    if (seg.frame_enabled) {
        drawFrame(seg);
    }
}

void TextRenderer::drawFrame(const Segment& seg) {
    for (int offset = 0; offset < seg.frame_width; offset++) {
        // Top edge
        for (int x = seg.x + offset; x <= seg.x + seg.width - 1 - offset; x++) {
            canvas_->SetPixel(x, seg.y + offset, 
                            seg.frame_color.r, seg.frame_color.g, seg.frame_color.b);
        }
        // Bottom edge
        for (int x = seg.x + offset; x <= seg.x + seg.width - 1 - offset; x++) {
            canvas_->SetPixel(x, seg.y + seg.height - 1 - offset,
                            seg.frame_color.r, seg.frame_color.g, seg.frame_color.b);
        }
        // Left edge
        for (int y = seg.y + offset; y <= seg.y + seg.height - 1 - offset; y++) {
            canvas_->SetPixel(seg.x + offset, y,
                            seg.frame_color.r, seg.frame_color.g, seg.frame_color.b);
        }
        // Right edge
        for (int y = seg.y + offset; y <= seg.y + seg.height - 1 - offset; y++) {
            canvas_->SetPixel(seg.x + seg.width - 1 - offset, y,
                            seg.frame_color.r, seg.frame_color.g, seg.frame_color.b);
        }
    }
}

void TextRenderer::renderGroupIndicator() {
    if (!g_udp_handler) return;
    
    int group_id = g_udp_handler->getGroupId();
    
    // Update cache if group changed
    if (group_id != group_id_cache_) {
        group_id_cache_ = group_id;
        const GroupColor& gc = GROUP_COLORS[group_id];
        group_color_cache_ = Color(gc.r, gc.g, gc.b);
    }
    
    // Skip if no group or black
    if (group_id == 0 || (group_color_cache_.r == 0 && 
                          group_color_cache_.g == 0 && 
                          group_color_cache_.b == 0)) {
        return;
    }
    
    // Draw colored square in bottom-left corner
    int x1 = 0;
    int y1 = canvas_height_ - GROUP_INDICATOR_SIZE;
    int x2 = GROUP_INDICATOR_SIZE - 1;
    int y2 = canvas_height_ - 1;
    
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            canvas_->SetPixel(x, y, group_color_cache_.r, 
                            group_color_cache_.g, group_color_cache_.b);
        }
    }
}
