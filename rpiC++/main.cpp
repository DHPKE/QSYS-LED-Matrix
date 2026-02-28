// main.cpp - Entry point for RPi RGB LED Matrix Controller (C++)

#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fstream>
#include "nlohmann/json.hpp"

#include "led-matrix.h"
#include "segment_manager.h"
#include "udp_handler.h"
#include "text_renderer.h"
#include "web_server.h"
#include "config.h"

using json = nlohmann::json;

using namespace rgb_matrix;

// Global pointers for signal handlers
RGBMatrix* g_matrix = nullptr;
UDPHandler* g_udp_handler = nullptr;
static volatile bool interrupt_received = false;

static void InterruptHandler(int signo) {
    interrupt_received = true;
}

// ─── Network Helpers ─────────────────────────────────────────────────────────

std::string getIP(const std::string& iface) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return "";
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        close(fd);
        return "";
    }
    
    close(fd);
    struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
    return inet_ntoa(addr->sin_addr);
}

bool applyFallbackIP(const std::string& ip, const std::string& netmask,
                    const std::string& gateway, const std::string& iface) {
    // Calculate prefix length from netmask
    struct in_addr nm;
    inet_aton(netmask.c_str(), &nm);
    uint32_t bits = ntohl(nm.s_addr);
    int prefix = __builtin_popcount(bits);
    
    // Build ip command
    std::string cmd_flush = "ip addr flush dev " + iface;
    std::string cmd_add = "ip addr add " + ip + "/" + std::to_string(prefix) + " dev " + iface;
    std::string cmd_up = "ip link set " + iface + " up";
    std::string cmd_route = "ip route add default via " + gateway;
    
    system(cmd_flush.c_str());
    if (system(cmd_add.c_str()) != 0) {
        std::cerr << "[NET] Failed to set IP address" << std::endl;
        return false;
    }
    system(cmd_up.c_str());
    system(cmd_route.c_str());
    
    std::cout << "[NET] ✓ Fallback IP applied: " << ip << "/" << prefix 
             << " gw " << gateway << " on " << iface << std::endl;
    return true;
}

std::string ensureNetwork() {
    std::cout << "[NET] Waiting up to " << DHCP_TIMEOUT_S << "s for DHCP on " 
             << FALLBACK_IFACE << "..." << std::endl;
    
    // Check if fallback IP is already configured (from previous run or manual config)
    std::string current_ip = getIP(FALLBACK_IFACE);
    bool has_fallback = (current_ip == FALLBACK_IP);
    
    if (has_fallback) {
        std::cout << "[NET] ! Fallback IP " << FALLBACK_IP << " already configured" << std::endl;
        std::cout << "[NET]   Flushing and waiting for DHCP..." << std::endl;
        
        // Flush existing IP to allow DHCP negotiation
        std::string cmd_flush = "ip addr flush dev " + std::string(FALLBACK_IFACE);
        system(cmd_flush.c_str());
        
        // Ensure interface is up
        std::string cmd_up = "ip link set " + std::string(FALLBACK_IFACE) + " up";
        system(cmd_up.c_str());
        
        // Give DHCP client a moment to restart negotiation
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    // Wait for DHCP
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(DHCP_TIMEOUT_S);
    while (std::chrono::steady_clock::now() < deadline) {
        std::string ip = getIP(FALLBACK_IFACE);
        if (!ip.empty() && ip.substr(0, 4) != "127." && ip != FALLBACK_IP) {
            std::cout << "[NET] ✓ DHCP address: " << ip << std::endl;
            return ip;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "[NET] No DHCP lease after " << DHCP_TIMEOUT_S << "s" << std::endl;
    
    // Apply fallback
    if (strlen(FALLBACK_IP) > 0) {
        std::cout << "[NET] Applying fallback static IP: " << FALLBACK_IP << std::endl;
        if (applyFallbackIP(FALLBACK_IP, FALLBACK_NETMASK, FALLBACK_GATEWAY, FALLBACK_IFACE)) {
            return FALLBACK_IP;
        }
    }
    
    std::cout << "[NET] FALLBACK_IP not configured — device may be unreachable" << std::endl;
    return "no IP";
}

// ─── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::cout << "==================================================" << std::endl;
    std::cout << "RPi RGB LED Matrix Controller (C++)" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Matrix: " << MATRIX_WIDTH << "×" << MATRIX_HEIGHT 
             << ", chain=" << MATRIX_CHAIN << std::endl;
    std::cout << "UDP port: " << UDP_PORT << ",  Web port: " << WEB_PORT << std::endl;
    
    // ── 1. Network setup ─────────────────────────────────────────────────────
    std::string device_ip = ensureNetwork();
    
    //── 3. Setup segment manager and load initial config ─────────────────────
    SegmentManager sm;
    
    // Load rotation from config before matrix init
    Rotation initial_rotation = ROTATION_0;
    {
        std::ifstream config_file(CONFIG_FILE);
        if (config_file.is_open()) {
            try {
                json config;
                config_file >> config;
                int rotation_value = config.value("rotation", 0);
                if (rotation_value == 0) initial_rotation = ROTATION_0;
                else if (rotation_value == 90) initial_rotation = ROTATION_90;
                else if (rotation_value == 180) initial_rotation = ROTATION_180;
                else if (rotation_value == 270) initial_rotation = ROTATION_270;
                std::cout << "[INIT] Loaded rotation from config: " << rotation_value << "°" << std::endl;
            } catch (...) {
                std::cout << "[INIT] Could not load rotation from config, using default (0°)" << std::endl;
            }
        }
    }
    
    // ── 4. Setup RGB matrix ──────────────────────────────────────────────────
    RGBMatrix::Options matrix_options;
    RuntimeOptions runtime_opt;
    
    matrix_options.rows = MATRIX_HEIGHT;
    matrix_options.cols = MATRIX_WIDTH;
    matrix_options.chain_length = MATRIX_CHAIN;
    matrix_options.parallel = MATRIX_PARALLEL;
    matrix_options.hardware_mapping = HARDWARE_MAPPING;
    matrix_options.brightness = BRIGHTNESS;
    matrix_options.pwm_bits = PWM_BITS;
    matrix_options.pwm_lsb_nanoseconds = PWM_LSB_NANOSECONDS;
    matrix_options.scan_mode = SCAN_MODE;
    matrix_options.row_address_type = ROW_ADDR_TYPE;
    matrix_options.multiplexing = MULTIPLEXING;
    matrix_options.pwm_dither_bits = PWM_DITHER_BITS;
    matrix_options.led_rgb_sequence = LED_RGB_SEQUENCE;
    matrix_options.limit_refresh_rate_hz = REFRESH_LIMIT;
    matrix_options.disable_hardware_pulsing = true;  // Avoid audio conflict
    matrix_options.show_refresh_rate = false;        // Disable refresh overlay
    matrix_options.inverse_colors = false;           // Normal color display
    
    // Set pixel mapper for rotation
    const char* pixel_mapper = "";
    switch (initial_rotation) {
        case ROTATION_0:
            pixel_mapper = "";  // No rotation
            break;
        case ROTATION_90:
            pixel_mapper = "Rotate:90";
            break;
        case ROTATION_180:
            pixel_mapper = "Rotate:180";
            break;
        case ROTATION_270:
            pixel_mapper = "Rotate:270";
            break;
    }
    matrix_options.pixel_mapper_config = pixel_mapper;
    
    runtime_opt.gpio_slowdown = GPIO_SLOWDOWN;
    runtime_opt.drop_privileges = 1;  // Drop root after init
    runtime_opt.daemon = 0;            // Run in foreground
    runtime_opt.do_gpio_init = true;   // Initialize GPIO properly
    
    g_matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (!g_matrix) {
        std::cerr << "Failed to create RGB matrix" << std::endl;
        return 1;
    }
    
    std::cout << "✓ LED matrix initialized (" << g_matrix->width() << "×" 
             << g_matrix->height() << ")" << std::endl;
    
    // ── 4. Brightness callback ───────────────────────────────────────────────
    auto on_brightness_change = [](int value_255) {
        if (g_matrix) {
            int pct = std::max(0, std::min(100, (value_255 * 100) / 255));
            g_matrix->SetBrightness(pct);
            std::cout << "[MAIN] Brightness → " << pct << "%" << std::endl;
        }
    };
    
    // ── 5. Orientation callback ──────────────────────────────────────────────
    auto on_orientation_change = [&sm](Orientation orient) {
        std::cout << "[MAIN] Orientation → " 
                 << (orient == PORTRAIT ? "portrait" : "landscape") << std::endl;
        
        // Reapply current layout for new orientation
        if (g_udp_handler) {
            int current_layout = g_udp_handler->getCurrentLayout();
            // This will be handled in UDP dispatch, just mark dirty
            sm.markAllDirty();
        }
    };
    
    // ── 6. Rotation callback ─────────────────────────────────────────────────
    auto on_rotation_change = [](Rotation rotation) {
        int angle = static_cast<int>(rotation);
        std::cout << "[MAIN] Rotation changed to " << angle << "° (will apply on next restart)" << std::endl;
    };
    
    // ── 7. Start UDP listener ────────────────────────────────────────────────
    g_udp_handler = new UDPHandler(&sm, on_brightness_change, on_orientation_change, on_rotation_change);
    g_udp_handler->start();
    
    // Note: rotation is already applied during matrix init from loaded config
    
    // Apply initial layout based on loaded orientation
    std::cout << "[MAIN] Initial orientation: " 
             << (g_udp_handler->getOrientation() == PORTRAIT ? "portrait" : "landscape") 
             << std::endl;
    // UDP handler already loaded config, layout will be applied on first command
    
    // ── 8. Start web config server ───────────────────────────────────────────
    WebServer web_server(WEB_PORT);
    web_server.start();
    
    // ── 8. IP splash screen ──────────────────────────────────────────────────
    TextRenderer renderer(g_matrix, &sm);
    
    // ── 8. IP splash screen ──────────────────────────────────────────────────
    bool ip_splash_active = true;
    sm.updateText(0, device_ip, "FFFFFF", "000000", "C", "none");
    sm.setFrame(0, true, "FFFFFF", 1);
    sm.markDirty(0);
    std::cout << "[SPLASH] Showing IP address: " << device_ip << std::endl;
    
    // ── 9. Setup signal handlers ─────────────────────────────────────────────
    signal(SIGINT, InterruptHandler);
    signal(SIGTERM, InterruptHandler);
    
    std::cout << "==================================================" << std::endl;
    std::cout << "System ready — press Ctrl+C to stop" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    // ── 10. Main render loop ─────────────────────────────────────────────────
    auto last_effect = std::chrono::steady_clock::now();
    auto last_render = std::chrono::steady_clock::now();
    const int TARGET_FPS = 20;  // Lower FPS for lower CPU
    const int FRAME_TIME_MS = 1000 / TARGET_FPS;
    const int MIN_RENDER_INTERVAL_MS = 50;  // 20fps max - balance between smooth and low CPU
    
    while (!interrupt_received) {
        auto now = std::chrono::steady_clock::now();
        
        // Check for test mode
        static bool test_mode_active = false;
        static bool test_mode_was_active = false;
        static int test_bar_offset = 0;
        std::ifstream testfile("/tmp/led-matrix-testmode");
        if (testfile.is_open()) {
            char c;
            testfile >> c;
            test_mode_active = (c == '1');
            testfile.close();
        }
        
        // Clear all segments when entering test mode
        if (test_mode_active && !test_mode_was_active) {
            std::cout << "[TEST] Entering test mode - clearing display..." << std::endl;
            sm.clearAll();
            // Clear matrix multiple times to ensure it's black
            for (int i = 0; i < 5; i++) {
                g_matrix->Clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            test_bar_offset = 0; // Reset bar position
            std::cout << "[TEST] Display cleared, starting test pattern" << std::endl;
        }
        test_mode_was_active = test_mode_active;
        
        if (test_mode_active) {
            // Skip first few frames after entering test mode to let clear take effect
            static int frames_since_test_start = 0;
            static auto test_mode_start_time = std::chrono::steady_clock::now();
            static bool initial_clear_done = false;
            
            if (!test_mode_was_active) {
                frames_since_test_start = 0;
                test_mode_start_time = now;
                initial_clear_done = false;
            }
            frames_since_test_start++;
            
            // Keep clearing for first 1 second (30 frames at 30fps)
            auto elapsed_since_start = std::chrono::duration_cast<std::chrono::milliseconds>(now - test_mode_start_time);
            if (elapsed_since_start.count() < 1000) {
                sm.clearAll(); // Keep segments clear
                g_matrix->Clear(); // Keep clearing
                std::this_thread::sleep_for(std::chrono::milliseconds(33));
                continue;
            }
            
            // After blackout, ensure segments are cleared once before starting pattern
            if (!initial_clear_done) {
                sm.clearAll();
                initial_clear_done = true;
            }
            
            // Render test pattern: 4-state cycle (hostname top, blank, IP bottom, blank)
            
            // Reset cycle state when entering test mode
            static bool test_mode_just_started = false;
            if (!test_mode_was_active) {
                test_mode_just_started = true;
            }
            
            // Get current hostname (refresh every 10 seconds)
            static std::string hostname = "led-matrix";
            static auto last_hostname_fetch = std::chrono::steady_clock::now();
            auto hostname_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_hostname_fetch);
            if (hostname_elapsed.count() >= 10) {
                FILE* pipe = popen("hostname", "r");
                if (pipe) {
                    char buf[128];
                    if (fgets(buf, sizeof(buf), pipe)) {
                        hostname = buf;
                        if (!hostname.empty() && hostname.back() == '\n') {
                            hostname.pop_back();
                        }
                    }
                    pclose(pipe);
                }
                last_hostname_fetch = now;
            }
            
            // Get current IP address (refresh every 10 seconds)
            static std::string test_device_ip = device_ip;
            static auto last_ip_fetch = std::chrono::steady_clock::now();
            auto ip_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_ip_fetch);
            if (ip_elapsed.count() >= 10) {
                // Get IP from eth1 (same as ensureNetwork)
                FILE* pipe = popen("ip -4 addr show eth1 2>/dev/null | grep -oP '(?<=inet\\s)\\d+(\\.\\d+){3}'", "r");
                if (pipe) {
                    char buf[64];
                    if (fgets(buf, sizeof(buf), pipe)) {
                        test_device_ip = buf;
                        if (!test_device_ip.empty() && test_device_ip.back() == '\n') {
                            test_device_ip.pop_back();
                        }
                    }
                    pclose(pipe);
                }
                last_ip_fetch = now;
            }
            
            // 4-state cycle every second: 0=hostname top, 1=blank, 2=IP bottom, 3=blank
            static auto last_cycle_switch = std::chrono::steady_clock::now();
            static int cycle_state = 0;
            static int last_cycle_state = -1;
            
            // Reset cycle on test mode start
            if (test_mode_just_started) {
                cycle_state = 0;
                last_cycle_state = -1;
                last_cycle_switch = now;
                test_mode_just_started = false;
            }
            
            auto cycle_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_cycle_switch);
            if (cycle_elapsed.count() >= 1) {
                cycle_state = (cycle_state + 1) % 4;
                last_cycle_switch = now;
            }
            
            // Draw vertical color bars (full height)
            const int bar_width = MATRIX_WIDTH / 5; // Even wider bars (1/5 = ~13px)
            const int num_bars = 8;
            
            // Colors: Red, Green, Blue, Cyan, Magenta, Yellow, White, Black
            const int colors[][3] = {
                {255, 0, 0},    // Red
                {0, 255, 0},    // Green
                {0, 0, 255},    // Blue
                {0, 255, 255},  // Cyan
                {255, 0, 255},  // Magenta
                {255, 255, 0},  // Yellow
                {255, 255, 255},// White
                {0, 0, 0}       // Black
            };
            
            // Move bars from left to right (slower for smoother appearance)
            static int frame_counter = 0;
            frame_counter++;
            if (frame_counter % 4 == 0) { // Update bar position every 4 frames (slower)
                test_bar_offset = (test_bar_offset + 1) % (bar_width * num_bars);
            }
            
            for (int x = 0; x < MATRIX_WIDTH; x++) {
                int bar_index = ((x + test_bar_offset) / bar_width) % num_bars;
                for (int y = 0; y < MATRIX_HEIGHT; y++) {
                    g_matrix->SetPixel(x, y, colors[bar_index][0], colors[bar_index][1], colors[bar_index][2]);
                }
            }
            
            // Update segments when cycle state changes
            if (cycle_state != last_cycle_state) {
                sm.clearAll();
                
                if (cycle_state == 0) {
                    // Hostname in upper half
                    sm.configure(0, 0, 0, MATRIX_WIDTH, MATRIX_HEIGHT / 2);
                    sm.activate(0, true);
                    sm.setFrame(0, false, "FFFFFF", 1); // Disable frame
                    sm.updateText(0, hostname, "000000", "010101", "C", "none");
                } else if (cycle_state == 2) {
                    // IP in lower half
                    sm.configure(0, 0, MATRIX_HEIGHT / 2, MATRIX_WIDTH, MATRIX_HEIGHT / 2);
                    sm.activate(0, true);
                    sm.setFrame(0, false, "FFFFFF", 1); // Disable frame
                    sm.updateText(0, test_device_ip, "000000", "010101", "C", "none");
                }
                // States 1 and 3 are blank (no segments active)
                
                last_cycle_state = cycle_state;
            }
            
            // Mark dirty every frame so text renders on top of bars
            sm.markAllDirty();
            
            // Render text on top of bars
            renderer.renderAll();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // 30fps
            continue; // Skip normal rendering
        }
        
        // Dismiss IP splash on first command
        if (ip_splash_active && g_udp_handler->hasReceivedCommand()) {
            std::cout << "[SPLASH] Dismissing splash - clearing segment 0 and disabling frame..." << std::endl;
            ip_splash_active = false;
            sm.clearSegment(0);  // Clear splash text
            std::cout << "[SPLASH] clearSegment(0) called" << std::endl;
            sm.setFrame(0, false, "FFFFFF", 1);  // Disable splash frame
            std::cout << "[SPLASH] setFrame(0, false) called" << std::endl;
            
            // Force full canvas clear to remove frame pixels
            g_matrix->Clear();
            std::cout << "[SPLASH] Canvas cleared" << std::endl;
            
            sm.markAllDirty();  // Force re-render
            std::cout << "[SPLASH] First command received — IP splash dismissed" << std::endl;
        }
        
        // Update effects at fixed interval
        bool effects_updated = false;
        auto effect_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_effect);
        if (effect_elapsed.count() >= EFFECT_INTERVAL) {
            sm.updateEffects();
            effects_updated = true;
            last_effect = now;
            
            // Render on effect timer (like Python version)
            // This gives consistent 10fps rendering regardless of command flood
            try {
                renderer.renderAll();
                last_render = now;
            } catch (const std::exception& e) {
                std::cerr << "[RENDER] Exception: " << e.what() << std::endl;
            }
        }
        
        // Sleep to yield CPU and allow matrix clean refresh cycles
        // Match Python: sleep for full EFFECT_INTERVAL
        std::this_thread::sleep_for(std::chrono::milliseconds(EFFECT_INTERVAL));
    }
    
    // ── Cleanup ──────────────────────────────────────────────────────────────
    std::cout << "\nShutting down..." << std::endl;
    
    if (g_udp_handler) {
        g_udp_handler->stop();
        delete g_udp_handler;
        g_udp_handler = nullptr;
    }
    
    if (g_matrix) {
        g_matrix->Clear();
        delete g_matrix;
        g_matrix = nullptr;
    }
    
    std::cout << "Clean exit." << std::endl;
    return 0;
}
