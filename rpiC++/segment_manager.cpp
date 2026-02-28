// segment_manager.cpp - Implementation of segment state manager

#include "segment_manager.h"
#include <algorithm>
#include <cstring>
#include <chrono>
#include <iostream>

// ─── Color Helper ────────────────────────────────────────────────────────────

Color Color::fromHex(const std::string& hex) {
    std::string h = hex;
    if (h[0] == '#') h = h.substr(1);
    if (h.length() != 6) return Color(255, 255, 255);
    
    int r = std::stoi(h.substr(0, 2), nullptr, 16);
    int g = std::stoi(h.substr(2, 2), nullptr, 16);
    int b = std::stoi(h.substr(4, 2), nullptr, 16);
    return Color(r, g, b);
}

// ─── Segment ─────────────────────────────────────────────────────────────────

Segment::Segment(int seg_id, int x_, int y_, int w_, int h_)
    : id(seg_id), x(x_), y(y_), width(w_), height(h_),
      text(""), color(255, 255, 255), bgcolor(0, 0, 0),
      align(ALIGN_CENTER), effect(EFFECT_NONE), effect_speed(50),
      scroll_offset(0), last_scroll_update(0),
      blink_state(true), last_blink_update(0),
      is_active(false), is_dirty(false),
      frame_enabled(false), frame_color(255, 255, 255), frame_width(2),
      font_name("arial") {
}

// ─── SegmentManager ──────────────────────────────────────────────────────────

SegmentManager::SegmentManager()
    : master_blink_state_(true), master_blink_last_update_(0) {
    initDefaultLayout();
}

SegmentManager::~SegmentManager() {
}

void SegmentManager::initDefaultLayout() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    segments_.clear();
    
    // Default: fullscreen on segment 0, others inactive
    segments_.push_back(Segment(0, 0, 0, MATRIX_WIDTH, MATRIX_HEIGHT));
    segments_[0].is_active = true;
    
    segments_.push_back(Segment(1, MATRIX_WIDTH/2, 0, MATRIX_WIDTH/2, MATRIX_HEIGHT));
    segments_.push_back(Segment(2, 0, MATRIX_HEIGHT/2, MATRIX_WIDTH/2, MATRIX_HEIGHT/2));
    segments_.push_back(Segment(3, MATRIX_WIDTH/2, MATRIX_HEIGHT/2, MATRIX_WIDTH/2, MATRIX_HEIGHT/2));
}

uint64_t SegmentManager::millis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

// ─── Read Access ─────────────────────────────────────────────────────────────

Segment* SegmentManager::getSegment(int seg_id) {
    if (seg_id >= 0 && seg_id < (int)segments_.size()) {
        return &segments_[seg_id];
    }
    return nullptr;
}

std::vector<Segment> SegmentManager::snapshot() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return segments_;
}

std::vector<Segment> SegmentManager::getRenderSnapshot(bool& any_dirty) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<Segment> result;
    any_dirty = false;
    
    for (const auto& seg : segments_) {
        if (seg.is_active || seg.is_dirty) {
            result.push_back(seg);
            if (seg.is_dirty) {
                any_dirty = true;
            }
        }
    }
    return result;
}

// ─── Write Access ────────────────────────────────────────────────────────────

void SegmentManager::updateText(int seg_id, const std::string& text,
                                const std::string& color,
                                const std::string& bgcolor,
                                const std::string& align,
                                const std::string& effect,
                                int intensity,
                                const std::string& font) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Segment* seg = getSegment(seg_id);
    if (!seg) return;
    
    seg->text = text.substr(0, MAX_TEXT_LENGTH);
    if (!color.empty()) {
        seg->color = Color::fromHex(color);
    }
    if (!bgcolor.empty()) {
        seg->bgcolor = Color::fromHex(bgcolor);
    }
    if (!align.empty()) {
        seg->align = parseAlign(align);
    }
    if (!effect.empty()) {
        seg->effect = parseEffect(effect);
    }
    if (!font.empty()) {
        std::string f = font;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        seg->font_name = (f == "monospace" || f == "mono") ? "monospace" : "arial";
    }
    // Note: is_active is controlled by layout command only!
    // Updating text doesn't activate segments outside current layout.
    seg->is_dirty = true;
}

void SegmentManager::clearSegment(int seg_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Segment* seg = getSegment(seg_id);
    if (seg) {
        seg->text = "";
        seg->is_dirty = true;
    }
}

void SegmentManager::clearAll() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& seg : segments_) {
        seg.text = "";
        seg.is_dirty = true;
    }
}

void SegmentManager::markAllDirty() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& seg : segments_) {
        seg.is_dirty = true;
    }
}

void SegmentManager::clearDirtyFlags() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& seg : segments_) {
        seg.is_dirty = false;
    }
}

bool SegmentManager::isDirty() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const auto& seg : segments_) {
        if (seg.is_dirty) return true;
    }
    return false;
}

void SegmentManager::configure(int seg_id, int x, int y, int w, int h) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Segment* seg = getSegment(seg_id);
    if (seg) {
        seg->x = x;
        seg->y = y;
        seg->width = w;
        seg->height = h;
        // Note: is_active is controlled by activate() call (from layout command)
        seg->is_dirty = true;
        
        // Mark all segments dirty for full redraw
        for (auto& s : segments_) {
            s.is_dirty = true;
        }
    }
}

void SegmentManager::activate(int seg_id, bool active) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Segment* seg = getSegment(seg_id);
    if (seg) {
        seg->is_active = active;
        seg->is_dirty = true;
        
        // Mark all segments dirty for full redraw
        for (auto& s : segments_) {
            s.is_dirty = true;
        }
    }
}

void SegmentManager::setFrame(int seg_id, bool enabled, const std::string& color, int width) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Segment* seg = getSegment(seg_id);
    if (seg) {
        seg->frame_enabled = enabled;
        if (!color.empty()) {
            seg->frame_color = Color::fromHex(color);
        }
        seg->frame_width = std::max(1, std::min(width, 10));
        seg->is_dirty = true;
    }
}

void SegmentManager::markDirty(int seg_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Segment* seg = getSegment(seg_id);
    if (seg) {
        seg->is_dirty = true;
    }
}

// ─── Effect Updates ──────────────────────────────────────────────────────────

void SegmentManager::updateEffects() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    uint64_t now = millis();
    
    // Update master blink state (500ms toggle)
    if (now - master_blink_last_update_ >= 500) {
        master_blink_state_ = !master_blink_state_;
        master_blink_last_update_ = now;
        
        // Mark blinking segments dirty
        for (auto& seg : segments_) {
            if (seg.is_active && seg.effect == EFFECT_BLINK) {
                seg.is_dirty = true;
            }
        }
    }
    
    // Update individual segment effects
    for (auto& seg : segments_) {
        if (!seg.is_active) continue;
        
        if (seg.effect == EFFECT_SCROLL) {
            int interval_ms = (seg.effect_speed > 0) ? (1000 / seg.effect_speed) : 50;
            if (now - seg.last_scroll_update >= (uint64_t)interval_ms) {
                seg.scroll_offset += 1;
                seg.last_scroll_update = now;
                seg.is_dirty = true;
            }
        } else if (seg.effect == EFFECT_BLINK) {
            seg.blink_state = master_blink_state_;
        }
    }
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

Align SegmentManager::parseAlign(const std::string& value) {
    if (value.empty()) return ALIGN_CENTER;
    char c = toupper(value[0]);
    if (c == 'L') return ALIGN_LEFT;
    if (c == 'R') return ALIGN_RIGHT;
    return ALIGN_CENTER;
}

Effect SegmentManager::parseEffect(const std::string& value) {
    if (value.empty()) return EFFECT_NONE;
    std::string v = value;
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    
    if (v == "scroll") return EFFECT_SCROLL;
    if (v == "blink") return EFFECT_BLINK;
    if (v == "fade") return EFFECT_FADE;
    return EFFECT_NONE;
}
