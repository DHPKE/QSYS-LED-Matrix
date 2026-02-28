// segment_manager.h - Thread-safe segment state manager

#ifndef SEGMENT_MANAGER_H
#define SEGMENT_MANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include "config.h"

struct Color {
    uint8_t r, g, b;
    
    Color() : r(0), g(0), b(0) {}
    Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    
    static Color fromHex(const std::string& hex);
};

struct Segment {
    int id;
    int x, y;
    int width, height;
    std::string text;
    Color color;
    Color bgcolor;
    Align align;
    Effect effect;
    int effect_speed;
    int scroll_offset;
    uint64_t last_scroll_update;
    bool blink_state;
    uint64_t last_blink_update;
    bool is_active;
    bool is_dirty;
    bool frame_enabled;
    Color frame_color;
    int frame_width;
    std::string font_name;  // "arial" or "monospace"
    
    Segment(int seg_id, int x_, int y_, int w_, int h_);
};

class SegmentManager {
public:
    SegmentManager();
    ~SegmentManager();
    
    // Read access (thread-safe)
    Segment* getSegment(int seg_id);
    std::vector<Segment> snapshot();
    std::vector<Segment> getRenderSnapshot(bool& any_dirty);
    
    // Write access (thread-safe)
    void updateText(int seg_id, const std::string& text,
                   const std::string& color = "",
                   const std::string& bgcolor = "",
                   const std::string& align = "",
                   const std::string& effect = "",
                   int intensity = 255,
                   const std::string& font = "");
    void clearSegment(int seg_id);
    void clearAll();
    void markAllDirty();
    void clearDirtyFlags();
    bool isDirty();  // Check if any segment needs rendering
    void configure(int seg_id, int x, int y, int w, int h);
    void activate(int seg_id, bool active);
    void setFrame(int seg_id, bool enabled, const std::string& color = "#FFFFFF", int width = 2);
    
    // Effect updates (call from render loop)
    void updateEffects();
    
    // Mark a specific segment dirty
    void markDirty(int seg_id);

private:
    std::vector<Segment> segments_;
    std::recursive_mutex mutex_;
    bool master_blink_state_;
    uint64_t master_blink_last_update_;
    
    void initDefaultLayout();
    uint64_t millis();
    
    Align parseAlign(const std::string& value);
    Effect parseEffect(const std::string& value);
};

#endif // SEGMENT_MANAGER_H
