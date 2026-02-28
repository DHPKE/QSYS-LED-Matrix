// web_server.cpp - Simple HTTP server for network configuration

#include "web_server.h"
#include "config.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

WebServer::WebServer(int port)
    : port_(port), server_fd_(-1), running_(false) {
}

WebServer::~WebServer() {
    stop();
}

void WebServer::start() {
    if (running_) return;
    
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[WEB] Failed to create socket" << std::endl;
        return;
    }
    
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[WEB] Failed to bind port " << port_ << std::endl;
        close(server_fd_);
        return;
    }
    
    if (listen(server_fd_, 5) < 0) {
        std::cerr << "[WEB] Failed to listen" << std::endl;
        close(server_fd_);
        return;
    }
    
    running_ = true;
    
    pthread_t thread;
    pthread_create(&thread, nullptr, serverThread, this);
    pthread_detach(thread);
    
    std::cout << "[WEB] Config server started on port " << port_ << std::endl;
}

void WebServer::stop() {
    if (!running_) return;
    running_ = false;
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
}

void* WebServer::serverThread(void* arg) {
    WebServer* server = (WebServer*)arg;
    
    while (server->running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server->server_fd_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd >= 0) {
            server->handleClient(client_fd);
            close(client_fd);
        }
    }
    
    return nullptr;
}

void WebServer::handleClient(int client_fd) {
    char buffer[4096];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return;
    
    buffer[bytes] = '\0';
    std::string request(buffer);
    
    // Parse HTTP request
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;
    
    // Extract body for POST
    std::string body;
    size_t body_pos = request.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
        body = request.substr(body_pos + 4);
    }
    
    // Handle request
    std::string response = handleRequest(method, path, body);
    
    // Send response
    send(client_fd, response.c_str(), response.length(), 0);
}

std::string WebServer::handleRequest(const std::string& method, const std::string& path, const std::string& body) {
    if (path == "/" || path == "/index.html") {
        std::string html = getConfigPage();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << html.length() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << html;
        return response.str();
    }
    
    if (path == "/api/config" && method == "GET") {
        std::string json_str = getCurrentConfig();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << json_str.length() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << json_str;
        return response.str();
    }
    
    if (path == "/api/config" && method == "POST") {
        bool success = saveConfig(body);
        std::string json_str = success ? "{\"status\":\"ok\"}" : "{\"status\":\"error\"}";
        std::ostringstream response;
        response << "HTTP/1.1 " << (success ? "200 OK" : "400 Bad Request") << "\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << json_str.length() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << json_str;
        return response.str();
    }
    
    if (path == "/api/reboot" && method == "POST") {
        std::cout << "[WEB] Reboot requested via web UI" << std::endl;
        // Fork to avoid killing the response
        if (fork() == 0) {
            sleep(1); // Give time for response to send
            system("sudo reboot");
            exit(0);
        }
        std::string json_str = "{\"status\":\"ok\"}";
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << json_str.length() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << json_str;
        return response.str();
    }
    
    // 404
    std::string not_found = "<!DOCTYPE html><html><body><h1>404 Not Found</h1></body></html>";
    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << not_found.length() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << not_found;
    return response.str();
}

std::string WebServer::getConfigPage() {
    return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LED Matrix Network Config</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 16px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 40px;
            max-width: 500px;
            width: 100%;
        }
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
        }
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .form-group {
            margin-bottom: 25px;
        }
        label {
            display: block;
            color: #555;
            font-weight: 600;
            margin-bottom: 8px;
            font-size: 14px;
        }
        input, select {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 15px;
            transition: all 0.2s;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        .toggle-group {
            display: flex;
            gap: 10px;
            margin-top: 8px;
        }
        .toggle-btn {
            flex: 1;
            padding: 10px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            background: white;
            cursor: pointer;
            font-weight: 600;
            transition: all 0.2s;
        }
        .toggle-btn:hover {
            border-color: #667eea;
        }
        .toggle-btn.active {
            background: #667eea;
            color: white;
            border-color: #667eea;
        }
        #staticFields {
            margin-top: 15px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 8px;
            display: none;
        }
        #staticFields.show {
            display: block;
        }
        .btn-primary {
            width: 100%;
            padding: 14px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
        }
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }
        .btn-primary:active {
            transform: translateY(0);
        }
        .btn-reboot {
            width: 100%;
            padding: 12px;
            margin-top: 15px;
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
        }
        .btn-reboot:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(245, 87, 108, 0.3);
        }
        .btn-reboot:active {
            transform: translateY(0);
        }
        .status {
            margin-top: 20px;
            padding: 12px;
            border-radius: 8px;
            text-align: center;
            font-weight: 600;
            display: none;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            display: block;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            display: block;
        }
        .current-ip {
            background: #e7f3ff;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 25px;
            border-left: 4px solid #667eea;
        }
        .current-ip strong {
            color: #667eea;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌐 Network Config</h1>
        <div class="subtitle">LED Matrix Controller</div>
        
        <div class="current-ip" id="currentIP">
            <strong>Current IP:</strong> <span id="ipDisplay">Loading...</span>
        </div>
        
        <form id="configForm">
            <div class="form-group">
                <label>Network Mode</label>
                <div class="toggle-group">
                    <button type="button" class="toggle-btn active" data-mode="dhcp">DHCP</button>
                    <button type="button" class="toggle-btn" data-mode="static">Static</button>
                </div>
            </div>
            
            <div id="staticFields">
                <div class="form-group">
                    <label>Static IP Address</label>
                    <input type="text" id="staticIP" placeholder="10.1.1.22" pattern="\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}">
                </div>
                <div class="form-group">
                    <label>Subnet Mask</label>
                    <input type="text" id="subnet" placeholder="255.255.255.0" pattern="\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}">
                </div>
                <div class="form-group">
                    <label>Gateway</label>
                    <input type="text" id="gateway" placeholder="10.1.1.1" pattern="\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}">
                </div>
            </div>
            
            <div class="form-group">
                <label>UDP Port</label>
                <input type="number" id="udpPort" placeholder="21324" min="1024" max="65535">
            </div>
            
            <button type="submit" class="btn-primary">💾 Save & Apply</button>
        </form>
        
        <button id="rebootBtn" class="btn-reboot" onclick="rebootDevice()">🔄 Reboot Device</button>
        
        <div class="status" id="status"></div>
    </div>
    
    <script>
        const form = document.getElementById('configForm');
        const toggleBtns = document.querySelectorAll('.toggle-btn');
        const staticFields = document.getElementById('staticFields');
        const status = document.getElementById('status');
        let currentMode = 'dhcp';
        
        // Load current config
        fetch('/api/config')
            .then(r => r.json())
            .then(data => {
                document.getElementById('ipDisplay').textContent = data.currentIP || 'Unknown';
                document.getElementById('udpPort').value = data.udpPort || 21324;
                
                if (data.mode === 'static') {
                    currentMode = 'static';
                    toggleBtns[1].click();
                    document.getElementById('staticIP').value = data.staticIP || '';
                    document.getElementById('subnet').value = data.subnet || '255.255.255.0';
                    document.getElementById('gateway').value = data.gateway || '';
                }
            })
            .catch(e => console.error('Failed to load config:', e));
        
        // Toggle DHCP/Static
        toggleBtns.forEach(btn => {
            btn.addEventListener('click', () => {
                toggleBtns.forEach(b => b.classList.remove('active'));
                btn.classList.add('active');
                currentMode = btn.dataset.mode;
                staticFields.classList.toggle('show', currentMode === 'static');
            });
        });
        
        // Form submit
        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const config = {
                mode: currentMode,
                udpPort: parseInt(document.getElementById('udpPort').value) || 21324
            };
            
            if (currentMode === 'static') {
                config.staticIP = document.getElementById('staticIP').value;
                config.subnet = document.getElementById('subnet').value;
                config.gateway = document.getElementById('gateway').value;
                
                if (!config.staticIP || !config.subnet || !config.gateway) {
                    showStatus('Please fill all static IP fields', 'error');
                    return;
                }
            }
            
            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                });
                
                const result = await response.json();
                
                if (result.status === 'ok') {
                    showStatus('✓ Config saved! Reboot required: sudo reboot', 'success');
                } else {
                    showStatus('Failed to save config', 'error');
                }
            } catch (e) {
                showStatus('Network error: ' + e.message, 'error');
            }
        });
        
        function showStatus(message, type) {
            status.textContent = message;
            status.className = 'status ' + type;
            setTimeout(() => {
                status.className = 'status';
            }, 5000);
        }
        
        async function rebootDevice() {
            if (!confirm('Reboot the LED Matrix controller? Display will be offline for ~30 seconds.')) {
                return;
            }
            
            try {
                showStatus('🔄 Rebooting...', 'success');
                await fetch('/api/reboot', { method: 'POST' });
                setTimeout(() => {
                    showStatus('⏳ Device rebooting... Page will reload in 45 seconds', 'success');
                    setTimeout(() => {
                        window.location.reload();
                    }, 45000);
                }, 500);
            } catch (e) {
                showStatus('Reboot command sent (connection closed)', 'success');
                setTimeout(() => {
                    window.location.reload();
                }, 45000);
            }
        }
    </script>
</body>
</html>)";
}

std::string WebServer::getCurrentConfig() {
    json config;
    
    // Read current config from file
    std::ifstream file(CONFIG_FILE);
    if (file.is_open()) {
        try {
            file >> config;
        } catch (...) {
            config = json::object();
        }
        file.close();
    }
    
    // Get current IP
    std::string current_ip = "Unknown";
    FILE* pipe = popen("hostname -I | awk '{print $1}'", "r");
    if (pipe) {
        char buf[128];
        if (fgets(buf, sizeof(buf), pipe)) {
            current_ip = buf;
            // Trim newline
            if (!current_ip.empty() && current_ip.back() == '\n') {
                current_ip.pop_back();
            }
        }
        pclose(pipe);
    }
    
    config["currentIP"] = current_ip;
    
    // Set defaults if missing
    if (!config.contains("mode")) config["mode"] = "dhcp";
    if (!config.contains("udpPort")) config["udpPort"] = UDP_PORT;
    
    return config.dump();
}

bool WebServer::saveConfig(const std::string& json_str) {
    try {
        json config = json::parse(json_str);
        
        // Validate
        if (!config.contains("mode") || !config.contains("udpPort")) {
            return false;
        }
        
        std::string mode = config["mode"];
        if (mode != "dhcp" && mode != "static") {
            return false;
        }
        
        if (mode == "static") {
            if (!config.contains("staticIP") || !config.contains("subnet") || !config.contains("gateway")) {
                return false;
            }
        }
        
        // Save to file
        std::ofstream file(CONFIG_FILE);
        if (!file.is_open()) {
            return false;
        }
        
        file << config.dump(4);
        file.close();
        
        std::cout << "[WEB] Network config saved to " << CONFIG_FILE << std::endl;
        std::cout << "[WEB] ⚠️  Reboot required for network changes to take effect" << std::endl;
        
        return true;
        
    } catch (...) {
        return false;
    }
}
