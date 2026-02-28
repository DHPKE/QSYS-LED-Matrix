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
    return R"HTMLDOC(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LED Matrix Network Config</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: rgb(23, 136, 202);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .logo-container {
            text-align: center;
            margin-bottom: 20px;
        }
        .logo-container svg {
            width: 200px;
            height: auto;
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
            background: linear-gradient(135deg, rgb(23, 136, 202) 0%, rgb(18, 108, 161) 100%);
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
            box-shadow: 0 10px 20px rgba(23, 136, 202, 0.4);
        }
        .btn-primary:active {
            transform: translateY(0);
        }
        .btn-reboot {
            width: 100%;
            padding: 12px;
            margin-top: 15px;
            background: linear-gradient(135deg, rgb(35, 164, 244) 0%, rgb(23, 136, 202) 100%);
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
            box-shadow: 0 10px 20px rgba(35, 164, 244, 0.4);
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
    <div class="logo-container">
        <svg version="1.0" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 2050 240">
            <path fill="#FFFFFF" d="M1132.82,98.27c9.3-10.68,18.49-21.27,27.56-31.76c1.85-2.12,3.99-4.62,7.01-4.79c1.77-0.11,3.61-0.15,5.49-0.12c12.29,0.12,24.05,0.15,35.3,0.09c1.04-0.01,1.19,0.37,0.47,1.13l-40.57,42.53c-0.39,0.41-0.45,1.03-0.14,1.51l45.38,71.37c0.45,0.71,0.27,1.07-0.57,1.07l-43.28-0.01c-0.47,0-0.82-0.2-1.07-0.6l-27.2-45.13c-0.18-0.29-0.59-0.34-0.83-0.1l-8.05,8.26c-0.32,0.32-0.49,0.76-0.49,1.21l-0.02,35.89c0,0.27-0.21,0.48-0.47,0.48l-36.58-0.04c-0.49,0-0.9-0.42-0.9-0.93V62.9c0-0.81,0.4-1.22,1.19-1.22l36.21,0.04c0.32,0,0.58,0.26,0.58,0.58l0.01,35.6C1131.85,98.89,1132.18,99.01,1132.82,98.27z"/>
            <path fill="#FFFFFF" d="M1012.43,141.61c-0.36-0.01-0.64,0.27-0.65,0.62v35.16c0,0.37-0.3,0.67-0.67,0.67l-37.9-0.01c-0.34,0-0.62-0.28-0.62-0.62l0.01-116.34c0-0.15,0.12-0.27,0.28-0.27c20.27-0.16,40.15-0.13,59.65,0.09c6.38,0.07,12.73,0.98,19.05,2.72c16.02,4.42,25.29,14.33,27.84,29.74c1.19,7.24,0.59,14.73-1.82,22.48C1068.91,143.69,1035.58,142.32,1012.43,141.61z M1013,89.03l-0.05,24.92c0,0.23,0.19,0.42,0.42,0.42l14.88,0.02c8.48,0.01,15.36-5.28,15.37-11.82v-2.07c0.01-6.54-6.85-11.85-15.33-11.87l-14.88-0.02C1013.19,88.61,1013,88.8,1013,89.03z"/>
            <path fill="#FFFFFF" d="M1269.12,151.6c16.44-0.06,33.01-0.05,49.71,0.05c2.23,0.01,4.55,0.11,6.96,0.26c0.55,0.04,0.83,0.33,0.83,0.87l0.12,24.38c0,0.61-0.31,0.9-0.93,0.9l-96.57,0.01c-0.34,0-0.62-0.28-0.62-0.62l0.01-115.92c0-0.18,0.14-0.32,0.32-0.32c23.03-0.04,47.93-0.05,74.68-0.01c6.67,0.01,13.64,0.13,20.92,0.37c0.42,0.01,0.64,0.23,0.65,0.63c0.32,8.58,0.3,16.92-0.06,25.02c-0.04,0.94-0.53,1.4-1.47,1.4h-55.23c-0.35,0-0.62,0.27-0.62,0.61l0.05,15.69c0,0.47,0.36,0.85,0.83,0.85l46.58-0.09c0.22,0,0.33,0.11,0.33,0.34l0.01,26.62c0,0.87-0.42,1.31-1.26,1.31l-45.93,0.08c-0.4,0-0.6,0.19-0.6,0.57l0.04,15.77C1267.86,151.07,1268.42,151.61,1269.12,151.6z"/>
            <path fill="#FFFFFF" d="M954.26,107c7.09,31.42-11.98,62.34-43.02,69.99c-5.32,1.31-13.55,1.89-24.68,1.74c-14.12-0.18-28.26-0.18-42.39,0c-0.65,0.01-0.98-0.32-1.66-0.97v-13.57c0.68-0.47,0.92-0.74,1.38-0.82c13.39-2.17,23.58-8.17,30.59-18.01c11.17-15.69,11.09-35.91-0.43-51.21c-7.22-9.57-17.26-15.35-30.16-17.33c-0.48-0.09-0.72-0.36-0.16-0.83V62.15c-0.54-0.51-0.28-0.77,0.23-0.77c18.61-0.11,36.15-0.09,52.63,0.06C924.41,61.68,948.11,79.71,954.26,107z M849.07,68.2c-0.78,1.7,0,3.63,1.92,4.2c3.8,1.14,7.62,2.23,11.12,4.04c24.26,12.58,33.66,42.99,20.45,67.06c-6.39,11.65-18.12,21.44-31.8,24.56c-1.79,0.4-2.34,1.68-1.67,3.82c0.18,0.58,0.72,0.97,1.32,0.96c14.38-0.11,28.97-0.1,43.78,0.06c5.95,0.06,11.21-0.48,15.77-1.62c21.1-5.28,37.35-23.4,39.67-45.4c1.19-11.32-0.88-21.73-6.21-31.23c-7.23-12.89-18.19-21.44-32.91-25.65c-4.48-1.29-9.97-1.9-16.45-1.85c-13.01,0.11-25.33,0.15-36.96,0.1c-2.41-0.01-4.7,0.04-6.86,0.15C849.7,67.44,849.3,67.7,849.07,68.2z"/>
            <path fill="#FFFFFF" d="M799.67,103.49c-3.37,7.71-4.34,15.57-2.88,23.58c3.53,19.49,18.63,33.44,38.32,36.14c0.42,0.06,0.74,0.43,0.04,0.85v13.73c0.66,0.61,0.36,0.92-0.26,0.91c-17.33-0.07-35.07-0.16-53.24-0.26c-16.8-0.1-30.76-5.97-41.9-17.62c-23.12-24.19-21.74-61.89,3-84.36c13.86-12.57,29.17-15.4,47.85-15.22c15.72,0.16,30.44,0.17,44.16,0.01c0.65-0.01,0.98,0.32,1.6,0.97v13.78c-0.49,0.62-0.99,1.13-1.62,1.11c-1.59-0.01-3.14,0.18-4.62,0.59C815.6,81.65,805.45,90.25,799.67,103.49z M783.4,67.17c-18.83-0.18-35.15,8.48-45.61,23.91c-9.13,13.49-11.44,31.58-5.65,46.71c8.59,22.37,27.7,35.26,51.75,35.09c17.11-0.12,32.18-0.12,45.18,0c0.64,0.01,0.96-0.31-0.05-0.94v-2.58c1-0.58,0.61-1.08,0.06-1.23c-13.46-3.43-23.7-10.56-30.72-21.38c-18.77-28.95-2.49-67.33,30.66-74.47c0.66-0.15,0.99-0.56,1.23-1.24v-2.9c-0.27-0.6-0.58-0.91-1.18-0.89C814.13,67.35,798.92,67.33,783.4,67.17z"/>
        </svg>
    </div>
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
                if (currentMode === 'static') {
                    const newIP = config.staticIP;
                    showStatus('⏳ Applying static IP ' + newIP + '...', 'success');
                } else {
                    showStatus('⏳ Switching to DHCP...', 'success');
                }
                
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                });
                
                const result = await response.json();
                
                if (result.status === 'ok') {
                    if (currentMode === 'static') {
                        const newIP = config.staticIP;
                        showStatus('✓ Static IP applied! Redirecting to http://' + newIP + ':8080', 'success');
                        setTimeout(() => {
                            window.location.href = 'http://' + newIP + ':8080';
                        }, 2000);
                    } else {
                        showStatus('✓ DHCP enabled! Page will reload in 5 seconds...', 'success');
                        setTimeout(() => {
                            window.location.reload();
                        }, 5000);
                    }
                } else {
                    showStatus('Failed to save config', 'error');
                }
            } catch (e) {
                if (currentMode === 'static') {
                    const newIP = config.staticIP;
                    showStatus('Network changed! Redirecting to http://' + newIP + ':8080', 'success');
                    setTimeout(() => {
                        window.location.href = 'http://' + newIP + ':8080';
                    }, 2000);
                } else {
                    showStatus('Network error: ' + e.message, 'error');
                }
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
</html>)HTMLDOC";
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
        
        // Apply network config immediately
        std::string iface = FALLBACK_IFACE;
        if (mode == "static") {
            std::string ip = config["staticIP"];
            std::string subnet = config["subnet"];
            std::string gateway = config["gateway"];
            
            std::string cmd = "sudo nmcli con mod netplan-eth0 ipv4.method manual "
                            "ipv4.addresses " + ip + "/24 ipv4.gateway " + gateway + " && "
                            "sudo nmcli con up netplan-eth0";
            
            std::cout << "[WEB] Applying static IP: " << ip << std::endl;
            int ret = system(cmd.c_str());
            if (ret != 0) {
                std::cout << "[WEB] ⚠️  Failed to apply network config (exit: " << ret << ")" << std::endl;
            } else {
                std::cout << "[WEB] ✓ Network config applied" << std::endl;
            }
        } else {
            // Switch back to DHCP
            std::string cmd = "sudo nmcli con mod netplan-eth0 ipv4.method auto && "
                            "sudo nmcli con up netplan-eth0";
            
            std::cout << "[WEB] Switching to DHCP" << std::endl;
            int ret = system(cmd.c_str());
            if (ret != 0) {
                std::cout << "[WEB] ⚠️  Failed to apply DHCP (exit: " << ret << ")" << std::endl;
            } else {
                std::cout << "[WEB] ✓ DHCP enabled" << std::endl;
            }
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}
