/**
 * web-admin.js
 * 
 * Enhanced web UI for C4M proxy with device configuration
 */

const http = require('http');
const url = require('url');
const fs = require('fs').promises;
const path = require('path');

class WebAdmin {
  constructor(server, configFile) {
    this.server = server;
    this.configFile = configFile;
  }

  /**
   * Handle HTTP requests
   */
  async handleRequest(req, res) {
    const parsedUrl = url.parse(req.url, true);
    const pathname = parsedUrl.pathname;

    // Serve admin UI
    if (pathname === '/' || pathname === '/admin') {
      return this.serveAdminUI(req, res);
    }

    // API endpoints
    if (pathname === '/api/config' && req.method === 'GET') {
      return this.getConfig(req, res);
    }

    if (pathname === '/api/config' && req.method === 'POST') {
      return this.updateConfig(req, res);
    }

    if (pathname === '/api/devices' && req.method === 'GET') {
      return this.getDevices(req, res);
    }

    if (pathname === '/api/devices' && req.method === 'POST') {
      return this.addDevice(req, res);
    }

    if (pathname === '/api/devices' && req.method === 'PUT') {
      return this.updateDevice(req, res);
    }

    if (pathname === '/api/devices' && req.method === 'DELETE') {
      return this.deleteDevice(req, res);
    }

    if (pathname === '/api/stats') {
      return this.getStats(req, res);
    }

    if (pathname === '/api/test') {
      return this.testDevice(req, res);
    }

    // 404
    res.writeHead(404);
    res.end('Not Found');
  }

  /**
   * Serve admin UI HTML
   */
  async serveAdminUI(req, res) {
    res.writeHead(200, { 'Content-Type': 'text/html' });
    res.end(this.generateAdminHTML());
  }

  /**
   * Get configuration
   */
  async getConfig(req, res) {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(this.server.config));
  }

  /**
   * Update configuration
   */
  async updateConfig(req, res) {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', async () => {
      try {
        const newConfig = JSON.parse(body);
        
        // Validate config
        if (!newConfig.mqtt || !newConfig.mqtt.broker) {
          throw new Error('Invalid configuration: missing mqtt.broker');
        }

        // Merge with existing config
        this.server.config = { ...this.server.config, ...newConfig };
        
        // Save to file
        await this.server.saveConfig();
        
        // Reinitialize if needed
        if (newConfig.devices) {
          this.server.initRenderers();
        }

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ success: true, message: 'Configuration updated' }));
      } catch (error) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ success: false, error: error.message }));
      }
    });
  }

  /**
   * Get devices
   */
  async getDevices(req, res) {
    const devices = this.server.config.devices.map(d => ({
      ...d,
      health: this.server.deviceHealth.get(d.id) || {},
      state: this.server.deviceStates.get(d.id) || {}
    }));

    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(devices));
  }

  /**
   * Add device
   */
  async addDevice(req, res) {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', async () => {
      try {
        const device = JSON.parse(body);
        
        // Validate
        if (!device.id || !device.ip || !device.name) {
          throw new Error('Missing required fields: id, ip, name');
        }

        // Check for duplicate ID
        if (this.server.config.devices.find(d => d.id === device.id)) {
          throw new Error(`Device with ID ${device.id} already exists`);
        }

        // Add defaults
        device.group = device.group || 0;
        device.enabled = device.enabled !== false;

        // Add to config
        this.server.config.devices.push(device);
        
        // Save
        await this.server.saveConfig();
        
        // Initialize renderer
        this.server.initRenderers();

        res.writeHead(201, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ success: true, device }));
      } catch (error) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ success: false, error: error.message }));
      }
    });
  }

  /**
   * Update device
   */
  async updateDevice(req, res) {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', async () => {
      try {
        const updates = JSON.parse(body);
        
        if (!updates.id) {
          throw new Error('Missing device id');
        }

        // Find device
        const deviceIndex = this.server.config.devices.findIndex(d => d.id === updates.id);
        if (deviceIndex === -1) {
          throw new Error(`Device ${updates.id} not found`);
        }

        // Update
        this.server.config.devices[deviceIndex] = {
          ...this.server.config.devices[deviceIndex],
          ...updates
        };

        // Save
        await this.server.saveConfig();
        
        // Reinitialize if needed
        this.server.initRenderers();

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ 
          success: true, 
          device: this.server.config.devices[deviceIndex] 
        }));
      } catch (error) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ success: false, error: error.message }));
      }
    });
  }

  /**
   * Delete device
   */
  async deleteDevice(req, res) {
    const parsedUrl = url.parse(req.url, true);
    const deviceId = parseInt(parsedUrl.query.id);

    if (!deviceId) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: false, error: 'Missing device id' }));
      return;
    }

    try {
      // Find and remove
      const deviceIndex = this.server.config.devices.findIndex(d => d.id === deviceId);
      if (deviceIndex === -1) {
        throw new Error(`Device ${deviceId} not found`);
      }

      const removed = this.server.config.devices.splice(deviceIndex, 1)[0];

      // Save
      await this.server.saveConfig();
      
      // Cleanup renderer
      this.server.renderers.delete(deviceId);
      this.server.deviceHealth.delete(deviceId);
      this.server.deviceStates.delete(deviceId);

      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: true, removed }));
    } catch (error) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: false, error: error.message }));
    }
  }

  /**
   * Get statistics
   */
  async getStats(req, res) {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(this.server.getStats()));
  }

  /**
   * Test device
   */
  async testDevice(req, res) {
    const parsedUrl = url.parse(req.url, true);
    const deviceId = parseInt(parsedUrl.query.id);

    if (!deviceId) {
      res.writeHead(400, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: false, error: 'Missing device id' }));
      return;
    }

    try {
      const device = this.server.config.devices.find(d => d.id === deviceId);
      if (!device) {
        throw new Error(`Device ${deviceId} not found`);
      }

      // Send test pattern
      await this.server.handleTest({}, [device]);

      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: true, message: 'Test pattern sent' }));
    } catch (error) {
      res.writeHead(500, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ success: false, error: error.message }));
    }
  }

  /**
   * Generate admin HTML
   */
  generateAdminHTML() {
    return `
<!DOCTYPE html>
<html>
<head>
  <title>C4M Proxy Admin</title>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      background: #0a0a0a;
      color: #00ff00;
      padding: 20px;
    }
    .container { max-width: 1400px; margin: 0 auto; }
    
    h1 { color: #00ffff; margin-bottom: 10px; }
    h2 { color: #00ffff; margin: 30px 0 15px; font-size: 1.3em; }
    
    .section {
      background: #1a1a1a;
      border: 1px solid #00ff00;
      border-radius: 8px;
      padding: 20px;
      margin-bottom: 20px;
    }
    
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 15px 0; }
    .stat-card { background: #222; padding: 15px; border-radius: 6px; border-left: 3px solid #00ff00; }
    .stat-label { font-size: 0.9em; color: #888; }
    .stat-value { font-size: 1.8em; color: #00ff00; margin-top: 5px; }
    
    .btn {
      background: #00ff00;
      color: #000;
      border: none;
      padding: 10px 20px;
      border-radius: 4px;
      cursor: pointer;
      font-weight: bold;
      font-size: 14px;
      margin-right: 10px;
    }
    .btn:hover { background: #00cc00; }
    .btn-danger { background: #ff3333; color: #fff; }
    .btn-danger:hover { background: #cc0000; }
    .btn-secondary { background: #666; color: #fff; }
    .btn-secondary:hover { background: #555; }
    
    table { width: 100%; border-collapse: collapse; margin-top: 15px; }
    th, td { padding: 12px; text-align: left; border-bottom: 1px solid #333; }
    th { background: #222; color: #00ffff; font-weight: normal; }
    tr:hover { background: #222; }
    
    .status-online { color: #00ff00; }
    .status-offline { color: #666; }
    .status-error { color: #ff3333; }
    
    input, select, textarea {
      background: #222;
      border: 1px solid #00ff00;
      color: #00ff00;
      padding: 8px 12px;
      border-radius: 4px;
      font-size: 14px;
      font-family: monospace;
      width: 100%;
      margin-bottom: 10px;
    }
    
    label {
      display: block;
      margin-bottom: 5px;
      color: #888;
      font-size: 0.9em;
    }
    
    .form-row { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 15px; }
    .form-group { margin-bottom: 15px; }
    
    .modal {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0, 0, 0, 0.9);
      z-index: 1000;
      padding: 40px 20px;
      overflow-y: auto;
    }
    .modal.active { display: block; }
    .modal-content {
      background: #1a1a1a;
      border: 2px solid #00ff00;
      border-radius: 8px;
      max-width: 600px;
      margin: 0 auto;
      padding: 30px;
    }
    .modal-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
    .modal-close { background: none; border: none; color: #00ff00; font-size: 24px; cursor: pointer; }
    
    .toast {
      position: fixed;
      top: 20px;
      right: 20px;
      background: #00ff00;
      color: #000;
      padding: 15px 20px;
      border-radius: 6px;
      font-weight: bold;
      display: none;
      z-index: 2000;
    }
    .toast.show { display: block; animation: slideIn 0.3s; }
    .toast.error { background: #ff3333; color: #fff; }
    
    @keyframes slideIn {
      from { transform: translateX(400px); opacity: 0; }
      to { transform: translateX(0); opacity: 1; }
    }
    
    .actions { text-align: right; margin-top: 20px; }
    
    code {
      background: #000;
      padding: 2px 6px;
      border-radius: 3px;
      font-family: 'Courier New', monospace;
      font-size: 0.9em;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🔧 C4M Proxy Admin Panel</h1>
    <p style="color: #888; margin-bottom: 30px;">MQTT-based LED Matrix Controller Management</p>
    
    <!-- Stats Section -->
    <div class="section">
      <h2>📊 Statistics</h2>
      <div class="grid" id="stats">
        <div class="stat-card">
          <div class="stat-label">Messages Received</div>
          <div class="stat-value" id="stat-messages">-</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Updates Sent</div>
          <div class="stat-value" id="stat-updates">-</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Update Rate</div>
          <div class="stat-value" id="stat-rate">-</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Devices Online</div>
          <div class="stat-value" id="stat-online">-</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Errors</div>
          <div class="stat-value" id="stat-errors">-</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Uptime</div>
          <div class="stat-value" id="stat-uptime">-</div>
        </div>
      </div>
    </div>
    
    <!-- MQTT Config Section -->
    <div class="section">
      <h2>🌐 MQTT Configuration</h2>
      <div class="form-row">
        <div class="form-group">
          <label>Broker URL</label>
          <input type="text" id="mqtt-broker" placeholder="mqtt://localhost:1883">
        </div>
        <div class="form-group">
          <label>Topic Prefix</label>
          <input type="text" id="mqtt-prefix" placeholder="led">
        </div>
      </div>
      <div class="form-row">
        <div class="form-group">
          <label>Username (optional)</label>
          <input type="text" id="mqtt-username">
        </div>
        <div class="form-group">
          <label>Password (optional)</label>
          <input type="password" id="mqtt-password">
        </div>
      </div>
      <button class="btn" onclick="saveMQTTConfig()">Save MQTT Config</button>
    </div>
    
    <!-- Devices Section -->
    <div class="section">
      <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px;">
        <h2 style="margin: 0;">🖥️ Devices (<span id="device-count">0</span>)</h2>
        <div style="display: flex; gap: 10px; align-items: center;">
          <label style="margin: 0; color: #00ffff;">Number of Devices:</label>
          <input type="number" id="num-devices" min="1" max="100" placeholder="40" style="width: 80px; margin: 0;">
          <button class="btn" onclick="setDeviceCount()">Generate</button>
          <button class="btn" onclick="showAddDevice()">+ Add Single</button>
        </div>
      </div>
      
      <table id="devices-table">
        <thead>
          <tr>
            <th>Status</th>
            <th>ID</th>
            <th>Name</th>
            <th>IP Address</th>
            <th>Group</th>
            <th>Health</th>
            <th>Actions</th>
          </tr>
        </thead>
        <tbody id="devices-tbody">
          <tr>
            <td colspan="7" style="text-align: center; color: #666;">Loading...</td>
          </tr>
        </tbody>
      </table>
    </div>
    
    <!-- Groups Section -->
    <div class="section">
      <h2>👥 Groups</h2>
      <div id="groups-list">Loading...</div>
    </div>
  </div>
  
  <!-- Add/Edit Device Modal -->
  <div id="device-modal" class="modal">
    <div class="modal-content">
      <div class="modal-header">
        <h2 id="modal-title">Add Device</h2>
        <button class="modal-close" onclick="closeModal()">&times;</button>
      </div>
      
      <div class="form-group">
        <label>Device ID *</label>
        <input type="number" id="device-id" placeholder="1-255" min="1" max="255">
      </div>
      
      <div class="form-group">
        <label>Device Name *</label>
        <input type="text" id="device-name" placeholder="Main-Entrance">
      </div>
      
      <div class="form-group">
        <label>IP Address *</label>
        <input type="text" id="device-ip" placeholder="10.1.1.101">
      </div>
      
      <div class="form-row">
        <div class="form-group">
          <label>Group</label>
          <select id="device-group">
            <option value="0">0 - All Displays</option>
            <option value="1">1 - Zone A</option>
            <option value="2">2 - Zone B</option>
            <option value="3">3 - Zone C</option>
            <option value="4">4 - Zone D</option>
          </select>
        </div>
        <div class="form-group">
          <label>Enabled</label>
          <select id="device-enabled">
            <option value="true">Yes</option>
            <option value="false">No</option>
          </select>
        </div>
      </div>
      
      <div class="actions">
        <button class="btn-secondary btn" onclick="closeModal()">Cancel</button>
        <button class="btn" onclick="saveDevice()">Save Device</button>
      </div>
    </div>
  </div>
  
  <!-- Toast Notification -->
  <div id="toast" class="toast"></div>
  
  <script>
    let devices = [];
    let config = {};
    let editingDevice = null;
    
    // Load initial data
    async function loadData() {
      await Promise.all([
        loadStats(),
        loadDevices(),
        loadConfig()
      ]);
    }
    
    // Load statistics
    async function loadStats() {
      try {
        const res = await fetch('/api/stats');
        const data = await res.json();
        
        document.getElementById('stat-messages').textContent = data.messagesReceived || 0;
        document.getElementById('stat-updates').textContent = data.updatesSent || 0;
        document.getElementById('stat-rate').textContent = (data.updateRate || '0') + ' Hz';
        document.getElementById('stat-online').textContent = data.devicesOnline + '/' + data.devices;
        document.getElementById('stat-errors').textContent = data.errors || 0;
        document.getElementById('stat-uptime').textContent = Math.floor(data.uptime || 0) + 's';
      } catch (e) {
        console.error('Failed to load stats:', e);
      }
    }
    
    // Load devices
    async function loadDevices() {
      try {
        const res = await fetch('/api/devices');
        devices = await res.json();
        
        document.getElementById('device-count').textContent = devices.length;
        
        const tbody = document.getElementById('devices-tbody');
        if (devices.length === 0) {
          tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: #666;">No devices configured</td></tr>';
          return;
        }
        
        tbody.innerHTML = devices.map(d => {
          const online = d.health?.online;
          const statusClass = online ? 'status-online' : 'status-offline';
          const statusText = online ? '● Online' : '○ Offline';
          const updateCount = d.health?.updateCount || 0;
          const errorCount = d.health?.errorCount || 0;
          
          return \`
            <tr>
              <td class="\${statusClass}">\${statusText}</td>
              <td><strong>\${d.id}</strong></td>
              <td>\${d.name}</td>
              <td><code>\${d.ip}</code></td>
              <td>Group \${d.group}</td>
              <td>\${updateCount} updates, \${errorCount} errors</td>
              <td>
                <button class="btn-secondary btn" style="padding: 6px 12px; margin: 0 5px 0 0;" onclick="testDevice(\${d.id})">Test</button>
                <button class="btn-secondary btn" style="padding: 6px 12px; margin: 0 5px 0 0;" onclick="editDevice(\${d.id})">Edit</button>
                <button class="btn-danger btn" style="padding: 6px 12px; margin: 0;" onclick="deleteDevice(\${d.id})">Delete</button>
              </td>
            </tr>
          \`;
        }).join('');
      } catch (e) {
        console.error('Failed to load devices:', e);
        showToast('Failed to load devices', true);
      }
    }
    
    // Load config
    async function loadConfig() {
      try {
        const res = await fetch('/api/config');
        config = await res.json();
        
        document.getElementById('mqtt-broker').value = config.mqtt?.broker || '';
        document.getElementById('mqtt-prefix').value = config.mqtt?.topicPrefix || 'led';
        document.getElementById('mqtt-username').value = config.mqtt?.username || '';
        document.getElementById('mqtt-password').value = config.mqtt?.password || '';
        
        // Update groups list
        const groupsList = document.getElementById('groups-list');
        if (config.groups) {
          groupsList.innerHTML = Object.entries(config.groups).map(([id, group]) => {
            const deviceCount = devices.filter(d => d.group === parseInt(id)).length;
            return \`
              <div style="background: #222; padding: 12px; margin-bottom: 10px; border-radius: 6px; border-left: 3px solid #00ff00;">
                <strong>Group \${id}:</strong> \${group.name || group} 
                <span style="color: #666;">(\${deviceCount} devices)</span>
                <br><span style="color: #888; font-size: 0.9em;">\${group.description || ''}</span>
              </div>
            \`;
          }).join('');
        }
      } catch (e) {
        console.error('Failed to load config:', e);
      }
    }
    
    // Save MQTT config
    async function saveMQTTConfig() {
      const mqtt = {
        broker: document.getElementById('mqtt-broker').value,
        topicPrefix: document.getElementById('mqtt-prefix').value,
        username: document.getElementById('mqtt-username').value,
        password: document.getElementById('mqtt-password').value
      };
      
      try {
        const res = await fetch('/api/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mqtt })
        });
        
        const result = await res.json();
        if (result.success) {
          showToast('MQTT configuration saved');
          loadConfig();
        } else {
          showToast(result.error || 'Failed to save', true);
        }
      } catch (e) {
        showToast('Failed to save MQTT config', true);
      }
    }
    
    // Show add device modal
    function showAddDevice() {
      editingDevice = null;
      document.getElementById('modal-title').textContent = 'Add Device';
      document.getElementById('device-id').value = '';
      document.getElementById('device-name').value = '';
      document.getElementById('device-ip').value = '';
      document.getElementById('device-group').value = '0';
      document.getElementById('device-enabled').value = 'true';
      document.getElementById('device-modal').classList.add('active');
    }
    
    // Edit device
    function editDevice(id) {
      const device = devices.find(d => d.id === id);
      if (!device) return;
      
      editingDevice = device;
      document.getElementById('modal-title').textContent = 'Edit Device';
      document.getElementById('device-id').value = device.id;
      document.getElementById('device-name').value = device.name;
      document.getElementById('device-ip').value = device.ip;
      document.getElementById('device-group').value = device.group;
      document.getElementById('device-enabled').value = device.enabled ? 'true' : 'false';
      document.getElementById('device-modal').classList.add('active');
    }
    
    // Save device
    async function saveDevice() {
      const device = {
        id: parseInt(document.getElementById('device-id').value),
        name: document.getElementById('device-name').value,
        ip: document.getElementById('device-ip').value,
        group: parseInt(document.getElementById('device-group').value),
        enabled: document.getElementById('device-enabled').value === 'true'
      };
      
      if (!device.id || !device.name || !device.ip) {
        showToast('Please fill all required fields', true);
        return;
      }
      
      try {
        const method = editingDevice ? 'PUT' : 'POST';
        const res = await fetch('/api/devices', {
          method,
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(device)
        });
        
        const result = await res.json();
        if (result.success) {
          showToast(editingDevice ? 'Device updated' : 'Device added');
          closeModal();
          loadDevices();
        } else {
          showToast(result.error || 'Failed to save device', true);
        }
      } catch (e) {
        showToast('Failed to save device', true);
      }
    }
    
    // Delete device
    async function deleteDevice(id) {
      if (!confirm(\`Delete device ID \${id}? This cannot be undone.\`)) return;
      
      try {
        const res = await fetch(\`/api/devices?id=\${id}\`, {
          method: 'DELETE'
        });
        
        const result = await res.json();
        if (result.success) {
          showToast('Device deleted');
          loadDevices();
        } else {
          showToast(result.error || 'Failed to delete', true);
        }
      } catch (e) {
        showToast('Failed to delete device', true);
      }
    }
    
    // Test device
    async function testDevice(id) {
      try {
        const res = await fetch(\`/api/test?id=\${id}\`);
        const result = await res.json();
        
        if (result.success) {
          showToast(\`Test pattern sent to device \${id}\`);
        } else {
          showToast(result.error || 'Test failed', true);
        }
      } catch (e) {
        showToast('Failed to test device', true);
      }
    }
    
    // Set device count - generate empty device entries
    async function setDeviceCount() {
      const count = parseInt(document.getElementById('num-devices').value);
      
      if (!count || count < 1 || count > 100) {
        showToast('Please enter a number between 1 and 100', true);
        return;
      }
      
      if (!confirm(\`This will create \${count} device entries. Existing devices will be preserved. Continue?\`)) {
        return;
      }
      
      // Number names mapping
      const numberNames = [
        '', 'One', 'Two', 'Three', 'Four', 'Five', 'Six', 'Seven', 'Eight', 'Nine', 'Ten',
        'Eleven', 'Twelve', 'Thirteen', 'Fourteen', 'Fifteen', 'Sixteen', 'Seventeen', 'Eighteen', 'Nineteen', 'Twenty',
        'TwentyOne', 'TwentyTwo', 'TwentyThree', 'TwentyFour', 'TwentyFive', 'TwentySix', 'TwentySeven', 'TwentyEight', 'TwentyNine', 'Thirty',
        'ThirtyOne', 'ThirtyTwo', 'ThirtyThree', 'ThirtyFour', 'ThirtyFive', 'ThirtySix', 'ThirtySeven', 'ThirtyEight', 'ThirtyNine', 'Fourty',
        'FourtyOne', 'FourtyTwo', 'FourtyThree', 'FourtyFour', 'FourtyFive', 'FourtySix', 'FourtySeven', 'FourtyEight', 'FourtyNine', 'Fifty',
        'FiftyOne', 'FiftyTwo', 'FiftyThree', 'FiftyFour', 'FiftyFive', 'FiftySix', 'FiftySeven', 'FiftyEight', 'FiftyNine', 'Sixty',
        'SixtyOne', 'SixtyTwo', 'SixtyThree', 'SixtyFour', 'SixtyFive', 'SixtySix', 'SixtySeven', 'SixtyEight', 'SixtyNine', 'Seventy',
        'SeventyOne', 'SeventyTwo', 'SeventyThree', 'SeventyFour', 'SeventyFive', 'SeventySix', 'SeventySeven', 'SeventyEight', 'SeventyNine', 'Eighty',
        'EightyOne', 'EightyTwo', 'EightyThree', 'EightyFour', 'EightyFive', 'EightySix', 'EightySeven', 'EightyEight', 'EightyNine', 'Ninety',
        'NinetyOne', 'NinetyTwo', 'NinetyThree', 'NinetyFour', 'NinetyFive', 'NinetySix', 'NinetySeven', 'NinetyEight', 'NinetyNine', 'OneHundred'
      ];
      
      try {
        // Get existing device IDs
        const existingIds = new Set(devices.map(d => d.id));
        
        // Generate new devices
        const newDevices = [];
        for (let i = 1; i <= count; i++) {
          if (!existingIds.has(i)) {
            newDevices.push({
              id: i,
              name: numberNames[i] || \`Device-\${i}\`,
              ip: \`10.1.1.\${100 + i}\`,
              group: 0,
              enabled: true
            });
          }
        }
        
        if (newDevices.length === 0) {
          showToast('All device IDs 1-' + count + ' already exist', false);
          return;
        }
        
        // Add new devices
        for (const device of newDevices) {
          await fetch('/api/devices', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(device)
          });
        }
        
        showToast(\`Generated \${newDevices.length} new devices (IDs 1-\${count})\`);
        await loadDevices();
        
      } catch (e) {
        console.error('Failed to generate devices:', e);
        showToast('Failed to generate devices', true);
      }
    }
    
    // Close modal
    function closeModal() {
      document.getElementById('device-modal').classList.remove('active');
    }
    
    // Show toast notification
    function showToast(message, isError = false) {
      const toast = document.getElementById('toast');
      toast.textContent = message;
      toast.className = 'toast show' + (isError ? ' error' : '');
      setTimeout(() => toast.classList.remove('show'), 3000);
    }
    
    // Auto-refresh
    setInterval(() => {
      loadStats();
      loadDevices();
    }, 2000);
    
    // Initial load
    loadData();
  </script>
</body>
</html>
    `;
  }
}

module.exports = WebAdmin;
