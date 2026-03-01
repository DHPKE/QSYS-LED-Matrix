"""
web_server.py - Simple HTTP server for network configuration
Based on the C++ version - minimal, focused on network settings only.
"""

import logging
import socket
import subprocess
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
from threading import Thread
from urllib.parse import parse_qs, urlparse

logger = logging.getLogger(__name__)

# Simple network config page HTML
HTML_PAGE = r"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title id="pageTitle">LED Matrix Config</title>
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
            width: 50%;
            max-width: 450px;
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
        .btn-test {
            width: 100%;
            padding: 12px;
            margin-top: 10px;
            background: linear-gradient(135deg, rgb(255, 193, 7) 0%, rgb(255, 152, 0) 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
        }
        .btn-test:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(255, 193, 7, 0.4);
        }
        .btn-test:active {
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
        <svg version="1.0" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 657.1 240">
            <path fill="#FFFFFF" d="M435.25,98.27c9.3-10.68,18.49-21.27,27.56-31.76c1.85-2.12,3.99-4.62,7.01-4.79c1.77-0.11,3.61-0.15,5.49-0.12c12.29,0.12,24.05,0.15,35.3,0.09c1.04-0.01,1.19,0.37,0.47,1.13l-40.57,42.53c-0.39,0.41-0.45,1.03-0.14,1.51l45.38,71.37c0.45,0.71,0.27,1.07-0.57,1.07l-43.28-0.01c-0.47,0-0.82-0.2-1.07-0.6l-27.2-45.13c-0.18-0.29-0.59-0.34-0.83-0.1l-8.05,8.26c-0.32,0.32-0.49,0.76-0.49,1.21l-0.02,35.89c0,0.27-0.21,0.48-0.47,0.48l-36.58-0.04c-0.49,0-0.9-0.42-0.9-0.93V62.9c0-0.81,0.4-1.22,1.19-1.22l36.21,0.04c0.32,0,0.58,0.26,0.58,0.58l0.01,35.6C434.28,98.89,434.61,99.01,435.25,98.27z"/>
            <path fill="#FFFFFF" d="M314.86,141.61c-0.36-0.01-0.64,0.27-0.65,0.62v35.16c0,0.37-0.3,0.67-0.67,0.67l-37.9-0.01c-0.34,0-0.62-0.28-0.62-0.62l0.01-116.34c0-0.15,0.12-0.27,0.28-0.27c20.27-0.16,40.15-0.13,59.65,0.09c6.38,0.07,12.73,0.98,19.05,2.72c16.02,4.42,25.29,14.33,27.84,29.74c1.19,7.24,0.59,14.73-1.82,22.48C371.34,143.69,338.01,142.32,314.86,141.61z M315.43,89.03l-0.05,24.92c0,0.23,0.19,0.42,0.42,0.42l14.88,0.02c8.48,0.01,15.36-5.28,15.37-11.82v-2.07c0.01-6.54-6.85-11.85-15.33-11.87l-14.88-0.02C315.62,88.61,315.43,88.8,315.43,89.03z"/>
            <path fill="#FFFFFF" d="M571.55,151.6c16.44-0.06,33.01-0.05,49.71,0.05c2.23,0.01,4.55,0.11,6.96,0.26c0.55,0.04,0.83,0.33,0.83,0.87l0.12,24.38c0,0.61-0.31,0.9-0.93,0.9l-96.57,0.01c-0.34,0-0.62-0.28-0.62-0.62l0.01-115.92c0-0.18,0.14-0.32,0.32-0.32c23.03-0.04,47.93-0.05,74.68-0.01c6.67,0.01,13.64,0.13,20.92,0.37c0.42,0.01,0.64,0.23,0.65,0.63c0.32,8.58,0.3,16.92-0.06,25.02c-0.04,0.94-0.53,1.4-1.47,1.4h-55.23c-0.35,0-0.62,0.27-0.62,0.61l0.05,15.69c0,0.47,0.36,0.85,0.83,0.85l46.58-0.09c0.22,0,0.33,0.11,0.33,0.34l0.01,26.62c0,0.87-0.42,1.31-1.26,1.31l-45.93,0.08c-0.4,0-0.6,0.19-0.6,0.57l0.04,15.77C570.29,151.07,570.85,151.61,571.55,151.6z"/>
            <path fill="#FFFFFF" d="M256.69,107c7.09,31.42-11.98,62.34-43.02,69.99c-5.32,1.31-13.55,1.89-24.68,1.74c-14.12-0.18-28.26-0.18-42.39,0c-0.65,0.01-0.98-0.32-1.66-0.97v-13.57c0.68-0.47,0.92-0.74,1.38-0.82c13.39-2.17,23.58-8.17,30.59-18.01c11.17-15.69,11.09-35.91-0.43-51.21c-7.22-9.57-17.26-15.35-30.16-17.33c-0.48-0.09-0.72-0.36-0.16-0.83V62.15c-0.54-0.51-0.28-0.77,0.23-0.77c18.61-0.11,36.15-0.09,52.63,0.06C226.84,61.68,250.54,79.71,256.69,107z M151.5,68.2c-0.78,1.7,0,3.63,1.92,4.2c3.8,1.14,7.62,2.23,11.12,4.04c24.26,12.58,33.66,42.99,20.45,67.06c-6.39,11.65-18.12,21.44-31.8,24.56c-1.79,0.4-2.34,1.68-1.67,3.82c0.18,0.58,0.72,0.97,1.32,0.96c14.38-0.11,28.97-0.1,43.78,0.06c5.95,0.06,11.21-0.48,15.77-1.62c21.1-5.28,37.35-23.4,39.67-45.4c1.19-11.32-0.88-21.73-6.21-31.23c-7.23-12.89-18.19-21.44-32.91-25.65c-4.48-1.29-9.97-1.9-16.45-1.85c-13.01,0.11-25.33,0.15-36.96,0.1c-2.41-0.01-4.7,0.04-6.86,0.15C152.13,67.44,151.73,67.7,151.5,68.2z"/>
            <path fill="#FFFFFF" d="M102.1,103.49c-3.37,7.71-4.34,15.57-2.88,23.58c3.53,19.49,18.63,33.44,38.32,36.14c0.42,0.06,0.74,0.43,0.04,0.85v13.73c0.66,0.61,0.36,0.92-0.26,0.91c-17.33-0.07-35.07-0.16-53.24-0.26c-16.8-0.1-30.76-5.97-41.9-17.62c-23.12-24.19-21.74-61.89,3-84.36c13.86-12.57,29.17-15.4,47.85-15.22c15.72,0.16,30.44,0.17,44.16,0.01c0.65-0.01,0.98,0.32,1.6,0.97V76c-0.49,0.62-0.99,1.13-1.62,1.11c-1.59-0.01-3.14,0.18-4.62,0.59C118.03,81.65,107.88,90.25,102.1,103.49z M85.83,67.17C67,66.99,50.68,75.65,40.22,91.08c-9.13,13.49-11.44,31.58-5.65,46.71c8.59,22.37,27.7,35.26,51.75,35.09c17.11-0.12,32.18-0.12,45.18,0c0.64,0.01,0.96-0.31-0.05-0.94v-2.58c1-0.58,0.61-1.08,0.06-1.23c-13.46-3.43-23.7-10.56-30.72-21.38c-18.77-28.95-2.49-67.33,30.66-74.47c0.66-0.15,0.99-0.56,1.23-1.24v-2.9c-0.27-0.6-0.58-0.91-1.18-0.89C116.56,67.35,101.35,67.33,85.83,67.17z"/>
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
                <label>Hostname</label>
                <input type="text" id="hostname" placeholder="led-matrix" pattern="[a-zA-Z0-9-]+">
            </div>
            
            <div class="form-group">
                <label>Network Mode</label>
                <div class="toggle-group">
                    <button type="button" class="toggle-btn active" data-mode="dhcp">DHCP</button>
                    <button type="button" class="toggle-btn" data-mode="static">Static</button>
                </div>
            </div>
            
            <div id="staticFields">
                <div class="form-group">
                    <label>IP Address</label>
                    <input type="text" id="ipAddr" placeholder="192.168.1.100" pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
                </div>
                
                <div class="form-group">
                    <label>Subnet Mask</label>
                    <input type="text" id="netmask" placeholder="255.255.255.0" pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
                </div>
                
                <div class="form-group">
                    <label>Gateway</label>
                    <input type="text" id="gateway" placeholder="192.168.1.1" pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
                </div>
                
                <div class="form-group">
                    <label>DNS Server</label>
                    <input type="text" id="dns" placeholder="8.8.8.8" pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
                </div>
            </div>
            
            <button type="submit" class="btn-primary">💾 Save Configuration</button>
        </form>
        
        <button type="button" class="btn-reboot" onclick="rebootDevice()">🔄 Reboot Device</button>
        <button type="button" class="btn-test" id="testModeBtn" onclick="toggleTestMode()">🎨 Toggle Test Mode</button>
        
        <div class="status" id="status"></div>
    </div>
    
    <script>
        // Load current configuration
        async function loadConfig() {
            try {
                const resp = await fetch('/api/config');
                const data = await resp.json();
                document.getElementById('ipDisplay').textContent = data.current_ip || 'Unknown';
                document.getElementById('hostname').value = data.hostname || 'led-matrix';
                
                if (data.mode === 'static') {
                    setMode('static');
                    document.getElementById('ipAddr').value = data.ip || '';
                    document.getElementById('netmask').value = data.netmask || '';
                    document.getElementById('gateway').value = data.gateway || '';
                    document.getElementById('dns').value = data.dns || '';
                }
            } catch (e) {
                console.error('Failed to load config:', e);
            }
        }
        
        // Toggle between DHCP and Static
        document.querySelectorAll('.toggle-btn').forEach(btn => {
            btn.addEventListener('click', function() {
                setMode(this.dataset.mode);
            });
        });
        
        function setMode(mode) {
            document.querySelectorAll('.toggle-btn').forEach(b => b.classList.remove('active'));
            document.querySelector(`[data-mode="${mode}"]`).classList.add('active');
            
            if (mode === 'static') {
                document.getElementById('staticFields').classList.add('show');
            } else {
                document.getElementById('staticFields').classList.remove('show');
            }
        }
        
        // Save configuration
        document.getElementById('configForm').addEventListener('submit', async function(e) {
            e.preventDefault();
            
            const mode = document.querySelector('.toggle-btn.active').dataset.mode;
            const config = {
                hostname: document.getElementById('hostname').value,
                mode: mode
            };
            
            if (mode === 'static') {
                config.ip = document.getElementById('ipAddr').value;
                config.netmask = document.getElementById('netmask').value;
                config.gateway = document.getElementById('gateway').value;
                config.dns = document.getElementById('dns').value;
            }
            
            try {
                const resp = await fetch('/api/config', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(config)
                });
                
                const result = await resp.json();
                
                const status = document.getElementById('status');
                if (result.success) {
                    status.className = 'status success';
                    status.textContent = '✓ Configuration saved! Reboot to apply changes.';
                } else {
                    status.className = 'status error';
                    status.textContent = '✗ Failed: ' + (result.error || 'Unknown error');
                }
            } catch (e) {
                const status = document.getElementById('status');
                status.className = 'status error';
                status.textContent = '✗ Connection error: ' + e.message;
            }
        });
        
        // Reboot device
        async function rebootDevice() {
            if (!confirm('Reboot the device now? This will disconnect you.')) return;
            
            try {
                await fetch('/api/reboot', { method: 'POST' });
                
                const status = document.getElementById('status');
                status.className = 'status success';
                status.textContent = '🔄 Rebooting... Please wait 30 seconds.';
            } catch (e) {
                console.log('Reboot initiated (connection lost as expected)');
            }
        }
        
        // Toggle test mode
        async function toggleTestMode() {
            try {
                const resp = await fetch('/api/testmode', { method: 'POST' });
                const result = await resp.json();
                
                const status = document.getElementById('status');
                const btn = document.getElementById('testModeBtn');
                
                if (result.success) {
                    status.className = 'status success';
                    status.textContent = '✓ ' + result.message;
                    btn.textContent = result.enabled ? '⏹️ Disable Test Mode' : '🎨 Enable Test Mode';
                } else {
                    status.className = 'status error';
                    status.textContent = '✗ Failed: ' + (result.error || 'Unknown error');
                }
            } catch (e) {
                const status = document.getElementById('status');
                status.className = 'status error';
                status.textContent = '✗ Connection error: ' + e.message;
            }
        }
        
        // Load on page load
        loadConfig();
    </script>
</body>
</html>
"""


def _get_current_ip():
    """Get the current IP address from the first UP interface."""
    try:
        out = subprocess.check_output(["ip", "-4", "addr", "show"], text=True)
        interfaces = {}
        current_iface = None
        is_up = False
        
        for line in out.splitlines():
            line_stripped = line.strip()
            
            if not line.startswith(" ") and ":" in line:
                parts = line.split(":")
                if len(parts) >= 2:
                    current_iface = parts[1].strip()
                    is_up = "UP" in line and "LOWER_UP" in line and "NO-CARRIER" not in line
            
            elif line_stripped.startswith("inet ") and current_iface:
                ip = line_stripped.split()[1].split("/")[0]
                if not ip.startswith("127.") and is_up:
                    interfaces[current_iface] = ip
        
        # Priority: eth* > wlan* > others
        for prefix in ["eth", "wlan", ""]:
            for iface in sorted(interfaces.keys()):
                if iface.startswith(prefix):
                    return interfaces[iface]
        
        return "Unknown"
    except Exception:
        return "Unknown"


class WebServerHandler(BaseHTTPRequestHandler):
    """HTTP request handler for the simple config web UI."""
    
    def log_message(self, format, *args):
        """Suppress default request logging."""
        pass
    
    def do_GET(self):
        """Handle GET requests."""
        parsed = urlparse(self.path)
        
        if parsed.path == "/" or parsed.path == "/index.html":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode())
        
        elif parsed.path == "/api/config":
            # Return current configuration
            config = {
                "current_ip": _get_current_ip(),
                "hostname": socket.gethostname(),
                "mode": "dhcp"  # Simplified - assume DHCP
            }
            
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(config).encode())
        
        else:
            self.send_error(404)
    
    def do_POST(self):
        """Handle POST requests."""
        parsed = urlparse(self.path)
        
        if parsed.path == "/api/config":
            # Save network configuration
            try:
                content_length = int(self.headers.get("Content-Length", 0))
                body = self.rfile.read(content_length).decode()
                config = json.loads(body)
                
                # For now, just acknowledge - actual network config would require root
                logger.info(f"[WEB] Config update requested: {config}")
                
                response = {"success": True, "message": "Configuration saved (requires reboot)"}
                
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps(response).encode())
            
            except Exception as e:
                logger.error(f"[WEB] Config update failed: {e}")
                response = {"success": False, "error": str(e)}
                
                self.send_response(500)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps(response).encode())
        
        elif parsed.path == "/api/reboot":
            # Reboot device
            logger.info("[WEB] Reboot requested")
            
            response = {"success": True}
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(response).encode())
            
            # Schedule reboot after response is sent
            try:
                subprocess.Popen(["sudo", "reboot"])
            except Exception as e:
                logger.error(f"[WEB] Reboot failed: {e}")
        
        elif parsed.path == "/api/testmode":
            # Toggle test mode
            try:
                # Read current state
                test_mode_enabled = False
                try:
                    with open("/tmp/led-matrix-testmode", "r") as f:
                        test_mode_enabled = (f.read().strip() == "1")
                except FileNotFoundError:
                    pass
                
                # Toggle state
                test_mode_enabled = not test_mode_enabled
                
                # Write new state
                with open("/tmp/led-matrix-testmode", "w") as f:
                    f.write("1" if test_mode_enabled else "0")
                
                logger.info(f"[WEB] Test mode {'enabled' if test_mode_enabled else 'disabled'}")
                
                response = {
                    "success": True,
                    "enabled": test_mode_enabled,
                    "message": "Test mode " + ("enabled" if test_mode_enabled else "disabled")
                }
                
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps(response).encode())
            
            except Exception as e:
                logger.error(f"[WEB] Test mode toggle failed: {e}")
                response = {"success": False, "error": str(e)}
                
                self.send_response(500)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps(response).encode())
        
        else:
            self.send_error(404)


class WebServer:
    """Simple HTTP server for network configuration."""
    
    def __init__(self, segment_manager, udp_handler, port=8080):
        """
        Initialize web server.
        
        :param segment_manager: SegmentManager instance (not used in simple version)
        :param udp_handler: UDPHandler instance (not used in simple version)
        :param port: HTTP port to listen on
        """
        self._port = port
        self._httpd = None
        self._thread = None
    
    def start(self):
        """Start the web server in a background thread."""
        if self._httpd:
            return
        
        try:
            self._httpd = HTTPServer(("", self._port), WebServerHandler)
            self._thread = Thread(target=self._httpd.serve_forever, daemon=True)
            self._thread.start()
            logger.info(f"[WEB] Config server started on port {self._port}")
        except Exception as e:
            logger.error(f"[WEB] Failed to start server: {e}")
    
    def stop(self):
        """Stop the web server."""
        if self._httpd:
            self._httpd.shutdown()
            self._httpd = None
            logger.info("[WEB] Server stopped")
