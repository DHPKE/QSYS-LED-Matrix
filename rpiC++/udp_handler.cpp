// udp_handler.cpp - UDP JSON command parser implementation

#include "udp_handler.h"
#include "config.h"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <libgen.h>

using json = nlohmann::json;

UDPHandler::UDPHandler(SegmentManager* segment_manager,
                       BrightnessCallback brightness_cb,
                       OrientationCallback orientation_cb,
                       RotationCallback rotation_cb)
    : sm_(segment_manager),
      socket_fd_(-1),
      running_(false),
      first_command_received_(false),
      brightness_callback_(brightness_cb),
      orientation_callback_(orientation_cb),
      rotation_callback_(rotation_cb),
      orientation_(LANDSCAPE),
      rotation_(ROTATION_0),
      current_layout_(1),
      brightness_(128),
      group_id_(0) {
    loadConfig();
}

UDPHandler::~UDPHandler() {
    stop();
}

void UDPHandler::start() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "[UDP] Failed to create socket" << std::endl;
        return;
    }
    
    int reuse = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[UDP] Failed to bind to port " << UDP_PORT << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return;
    }
    
    // Set receive timeout for clean shutdown
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    running_ = true;
    listener_thread_ = std::thread(&UDPHandler::run, this);
    
    std::cout << "[UDP] Listening on " << UDP_BIND_ADDR << ":" << UDP_PORT << std::endl;
}

void UDPHandler::stop() {
    running_ = false;
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
}

void UDPHandler::run() {
    char buffer[4096];
    
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        ssize_t len = recvfrom(socket_fd_, buffer, sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&client_addr, &client_len);
        
        if (len < 0) {
            if (!running_) break;
            continue;
        }
        
        buffer[len] = '\0';
        std::string raw(buffer, len);
        
        // Trim whitespace
        size_t start = raw.find_first_not_of(" \t\r\n");
        size_t end = raw.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            raw = raw.substr(start, end - start + 1);
        }
        
        dispatch(raw);
    }
    
    std::cout << "[UDP] Listener thread exited" << std::endl;
}

void UDPHandler::dispatch(const std::string& raw_json) {
    // Check if test mode is active - if so, ignore all UDP commands
    std::ifstream testfile("/tmp/led-matrix-testmode");
    if (testfile.is_open()) {
        char c;
        testfile >> c;
        testfile.close();
        if (c == '1') {
            // Test mode active - ignore all commands silently
            return;
        }
    }
    
    // Reduced logging - only log on startup or errors
    // std::cout << "[UDP] Received: " << raw_json << std::endl;
    
    try {
        json doc = json::parse(raw_json);
        first_command_received_ = true;
        
        std::string cmd = doc.value("cmd", "");
        
        // Check group filtering
        int cmd_group = doc.value("group", 0);
        int my_group = group_id_;
        
        if (cmd_group != 0 && my_group != 0 && cmd_group != my_group) {
            std::cout << "[UDP] Ignoring command for group " << cmd_group 
                     << " (this panel is group " << my_group << ")" << std::endl;
            return;
        }
        
        // Reduced logging - only on group changes or layout commands
        // std::cout << "[UDP] Executing command: " << cmd 
        //          << " (group=" << cmd_group << ", my_group=" << my_group << ")" << std::endl;
        
        // Auto-disable frame on segment 1 when first command arrives (unless it's a frame command)
        if (cmd != "frame" && doc.contains("seg")) {
            int seg = doc.value("seg", 0);
            if (seg == 1) {
                std::cout << "[UDP] Auto-disabling frame on segment 1 (cmd: " << cmd << ")" << std::endl;
                sm_->setFrame(seg, false, "FFFFFF", 2);
            }
        }
        
        if (cmd == "text") {
            int seg = doc.value("seg", 0);
            std::string text = doc.value("text", "");
            std::string color = doc.value("color", "FFFFFF");
            std::string bgcolor = doc.value("bgcolor", "000000");
            std::string align = doc.value("align", "C");
            std::string effect = doc.value("effect", "none");
            int intensity = doc.value("intensity", 255);
            std::string font = doc.value("font", "arial");  // "arial" or "monospace"
            
            sm_->updateText(seg, text, color, bgcolor, align, effect, intensity, font);
            
        } else if (cmd == "layout") {
            int preset = doc.value("preset", 1);
            applyLayout(preset);
            
        } else if (cmd == "clear") {
            int seg = doc.value("seg", 0);
            sm_->clearSegment(seg);
            
        } else if (cmd == "clear_all") {
            sm_->clearAll();
            
        } else if (cmd == "brightness") {
            int val = doc.value("value", -1);
            if (val >= 0 && val <= 255) {
                std::lock_guard<std::mutex> lock(config_mutex_);
                brightness_ = val;
                if (brightness_callback_) {
                    brightness_callback_(val);
                }
                sm_->markAllDirty();
                saveConfig();
            }
            
        } else if (cmd == "orientation") {
            // DEPRECATED: orientation command now maps to rotation for backward compatibility
            std::string value = doc.value("value", "landscape");
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            
            Rotation new_rotation;
            if (value == "portrait") {
                new_rotation = ROTATION_90;  // Portrait = 90° rotation
                std::cout << "[UDP] Orientation 'portrait' command → mapped to rotation 90°" << std::endl;
            } else {
                new_rotation = ROTATION_0;   // Landscape = 0° rotation
                std::cout << "[UDP] Orientation 'landscape' command → mapped to rotation 0°" << std::endl;
            }
            
            {
                std::lock_guard<std::mutex> lock(config_mutex_);
                rotation_ = new_rotation;
                orientation_ = (value == "portrait") ? PORTRAIT : LANDSCAPE;  // Keep for legacy
            }
            
            if (rotation_callback_) {
                rotation_callback_(new_rotation);
            }
            
            std::cout << "[UDP] ⚠ WARNING: 'orientation' command is deprecated, use 'rotation' instead" << std::endl;
            
            // Reapply current layout for new rotation
            applyLayout(current_layout_);
            saveConfig();
            saveConfig();
            
        } else if (cmd == "rotation") {
            int value = doc.value("value", 0);
            Rotation new_rotation = ROTATION_0;
            
            if (value == 0) new_rotation = ROTATION_0;
            else if (value == 90) new_rotation = ROTATION_90;
            else if (value == 180) new_rotation = ROTATION_180;
            else if (value == 270) new_rotation = ROTATION_270;
            else {
                std::cerr << "[UDP] Invalid rotation value: " << value << " (must be 0, 90, 180, or 270)" << std::endl;
                return;
            }
            
            {
                std::lock_guard<std::mutex> lock(config_mutex_);
                rotation_ = new_rotation;
            }
            
            if (rotation_callback_) {
                rotation_callback_(new_rotation);
            }
            
            std::cout << "[UDP] Rotation set to " << value << "°" << std::endl;
            saveConfig();
            
        } else if (cmd == "group") {
            int value = doc.value("value", 0);
            if (value >= 0 && value <= 8) {
                std::lock_guard<std::mutex> lock(config_mutex_);
                group_id_ = value;
                sm_->markAllDirty();
                saveConfig();
            }
            
        } else if (cmd == "config") {
            int seg = doc.value("seg", 0);
            int x = doc.value("x", 0);
            int y = doc.value("y", 0);
            int w = doc.value("w", 64);
            int h = doc.value("h", 32);
            sm_->configure(seg, x, y, w, h);
            
        } else if (cmd == "frame") {
            int seg = doc.value("seg", 0);
            bool enabled = doc.value("enabled", false);
            std::string color = doc.value("color", "FFFFFF");
            int width = doc.value("width", 2);
            sm_->setFrame(seg, enabled, color, width);
            
        } else {
            std::cerr << "[UDP] Unknown cmd: " << cmd << std::endl;
        }
        
    } catch (const json::exception& e) {
        std::cerr << "[UDP] JSON parse error: " << e.what() << std::endl;
    }
}

void UDPHandler::applyLayout(int preset) {
    if (preset < 1 || preset > 14) {
        std::cerr << "[UDP] Unknown layout preset " << preset << std::endl;
        return;
    }
    
    // Skip if layout didn't actually change
    if (current_layout_ == preset) {
        return;  // No-op, already on this layout
    }
    
    current_layout_ = preset;
    
    // Select base layout based on rotation (rotation changes effective canvas dimensions)
    // 0° and 180° use landscape (64×32), 90° and 270° use portrait (32×64)
    const std::vector<LayoutRect>* zones;
    bool use_portrait_layout = (rotation_ == ROTATION_90 || rotation_ == ROTATION_270);
    
    if (use_portrait_layout) {
        zones = &LAYOUT_PORTRAIT[preset];
    } else {
        zones = &LAYOUT_LANDSCAPE[preset];
    }
    
    std::cout << "[UDP] LAYOUT preset=" << preset 
             << " (" << zones->size() << " segment(s))"
             << " rotation=" << static_cast<int>(rotation_) << "°"
             << " [using " << (use_portrait_layout ? "portrait" : "landscape") << " coords]" << std::endl;
    
    for (int i = 0; i < MAX_SEGMENTS; i++) {
        if (i < (int)zones->size()) {
            const LayoutRect& rect = (*zones)[i];
            sm_->configure(i, rect.x, rect.y, rect.w, rect.h);
            sm_->activate(i, true);
        } else {
            sm_->activate(i, false);
        }
    }
    
    // Always disable frame on segment 1 after layout change
    std::cout << "[UDP] Disabling frame on segment 1 after layout change" << std::endl;
    sm_->setFrame(1, false, "FFFFFF", 2);
}

void UDPHandler::loadConfig() {
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open()) {
        std::cout << "[CONFIG] No config file found, using defaults" << std::endl;
        return;
    }
    
    try {
        json config;
        file >> config;
        
        std::string orient = config.value("orientation", "landscape");
        orientation_ = (orient == "portrait") ? PORTRAIT : LANDSCAPE;
        
        int rotation_value = config.value("rotation", 0);
        if (rotation_value == 0) rotation_ = ROTATION_0;
        else if (rotation_value == 90) rotation_ = ROTATION_90;
        else if (rotation_value == 180) rotation_ = ROTATION_180;
        else if (rotation_value == 270) rotation_ = ROTATION_270;
        else rotation_ = ROTATION_0;
        
        group_id_ = config.value("group_id", 0);
        brightness_ = config.value("brightness", 128);
        
        std::cout << "[CONFIG] Loaded orientation: " << orient 
                 << ", rotation: " << rotation_value << "°"
                 << ", group_id: " << group_id_ 
                 << ", brightness: " << brightness_ << std::endl;
    } catch (const json::exception& e) {
        std::cerr << "[CONFIG] Failed to parse: " << e.what() << std::endl;
    }
}

void UDPHandler::saveConfig() {
    // Create directory if it doesn't exist
    char* path_copy = strdup(CONFIG_FILE);
    char* dir = dirname(path_copy);
    mkdir(dir, 0755);
    free(path_copy);
    
    json config;
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config["orientation"] = (orientation_ == PORTRAIT) ? "portrait" : "landscape";
        config["rotation"] = static_cast<int>(rotation_);
        config["group_id"] = group_id_;
        config["brightness"] = brightness_;
    }
    
    std::ofstream file(CONFIG_FILE);
    if (file.is_open()) {
        file << config.dump(2);
        std::cout << "[CONFIG] Saved to " << CONFIG_FILE << std::endl;
    } else {
        std::cerr << "[CONFIG] Failed to save" << std::endl;
    }
}
