// udp_handler.h - UDP JSON command parser

#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include "segment_manager.h"

class UDPHandler {
public:
    using BrightnessCallback = std::function<void(int)>;
    using OrientationCallback = std::function<void(Orientation)>;
    
    UDPHandler(SegmentManager* segment_manager,
               BrightnessCallback brightness_cb = nullptr,
               OrientationCallback orientation_cb = nullptr);
    ~UDPHandler();
    
    void start();
    void stop();
    bool hasReceivedCommand() const { return first_command_received_; }
    int getCurrentLayout() const { return current_layout_; }
    Orientation getOrientation() const { return orientation_; }
    int getGroupId() const { return group_id_; }
    int getBrightness() const { return brightness_; }
    
    void dispatch(const std::string& raw_json);
    
private:
    SegmentManager* sm_;
    int socket_fd_;
    std::thread listener_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> first_command_received_;
    BrightnessCallback brightness_callback_;
    OrientationCallback orientation_callback_;
    
    Orientation orientation_;
    int current_layout_;
    int brightness_;
    int group_id_;
    
    mutable std::mutex config_mutex_;
    
    void run();
    void applyLayout(int preset);
    void loadConfig();
    void saveConfig();
};

#endif // UDP_HANDLER_H
