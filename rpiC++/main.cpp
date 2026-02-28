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

#include "led-matrix.h"
#include "segment_manager.h"
#include "udp_handler.h"
#include "text_renderer.h"
#include "web_server.h"
#include "config.h"

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
    
    // Wait for DHCP
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(DHCP_TIMEOUT_S);
    while (std::chrono::steady_clock::now() < deadline) {
        std::string ip = getIP(FALLBACK_IFACE);
        if (!ip.empty() && ip.substr(0, 4) != "127.") {
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
    
    // ── 2. Initialize segment manager ────────────────────────────────────────
    SegmentManager sm;
    
    // ── 3. Setup RGB matrix ──────────────────────────────────────────────────
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
    
    runtime_opt.gpio_slowdown = GPIO_SLOWDOWN;
    runtime_opt.drop_privileges = 1;  // Drop root after init
    
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
    
    // ── 6. Start UDP listener ────────────────────────────────────────────────
    g_udp_handler = new UDPHandler(&sm, on_brightness_change, on_orientation_change);
    g_udp_handler->start();
    
    // Apply initial layout based on loaded orientation
    std::cout << "[MAIN] Initial orientation: " 
             << (g_udp_handler->getOrientation() == PORTRAIT ? "portrait" : "landscape") 
             << std::endl;
    // UDP handler already loaded config, layout will be applied on first command
    
    // ── 7. Start web config server ───────────────────────────────────────────
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
    const int TARGET_FPS = 30;  // Reduced from ~60 to 30fps
    const int FRAME_TIME_MS = 1000 / TARGET_FPS;
    
    while (!interrupt_received) {
        auto now = std::chrono::steady_clock::now();
        
        // Dismiss IP splash on first command
        if (ip_splash_active && g_udp_handler->hasReceivedCommand()) {
            ip_splash_active = false;
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
        }
        
        // Only render if something changed or effects updated
        bool needs_render = effects_updated || sm.isDirty();
        
        auto render_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render);
        if (needs_render && render_elapsed.count() >= FRAME_TIME_MS) {
            try {
                renderer.renderAll();
                last_render = now;
            } catch (const std::exception& e) {
                std::cerr << "[RENDER] Exception: " << e.what() << std::endl;
            }
        }
        
        // Sleep to yield CPU (longer sleep when idle)
        int sleep_ms = needs_render ? 1 : 10;  // 1ms when active, 10ms when idle
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
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
