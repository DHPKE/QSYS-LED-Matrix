/**
 * server.js - MQTT-based C4M LED Matrix Proxy
 * 
 * MQTT Topic Structure:
 * 
 * Command topics (Q-SYS → Proxy):
 *   led/display/{id}/cmd/segment     - Set segment text/properties
 *   led/display/{id}/cmd/layout      - Set layout preset
 *   led/display/{id}/cmd/brightness  - Set brightness
 *   led/display/{id}/cmd/clear       - Clear display
 *   led/display/{id}/cmd/test        - Test pattern
 *   led/display/all/cmd/*            - Broadcast to all displays
 *   led/group/{group}/cmd/*          - Send to group
 * 
 * Status topics (Proxy → Q-SYS):
 *   led/display/{id}/status          - Display status
 *   led/display/{id}/error           - Error messages
 *   led/proxy/status                 - Proxy server status
 *   led/proxy/stats                  - Performance statistics
 * 
 * Example message to set segment text:
 * Topic: led/display/42/cmd/segment
 * Payload: {
 *   "seg": 0,
 *   "enabled": true,
 *   "text": "Hello World",
 *   "color": "FF0000",
 *   "effect": 1,
 *   "align": 1
 * }
 */

const mqtt = require('mqtt');
const http = require('http');
const fs = require('fs').promises;
const path = require('path');
const { RenderEngine, EFFECTS, ALIGN } = require('./render-engine');
const WebAdmin = require('./web-admin');

const CONFIG_FILE = path.join(__dirname, 'config.json');
const DEFAULT_CONFIG = {
  mqtt: {
    broker: 'mqtt://localhost:1883',
    username: '',
    password: '',
    clientId: 'c4m-proxy',
    topicPrefix: 'led'
  },
  webPort: 8080,
  matrixWidth: 64,
  matrixHeight: 32,
  updateRate: 10,
  devices: [
    // ID-based addressing (no IPs in Q-SYS!)
    { id: 1, ip: '10.1.1.101', group: 0, name: 'LED-01', enabled: true },
    { id: 2, ip: '10.1.1.102', group: 0, name: 'LED-02', enabled: true },
    { id: 3, ip: '10.1.1.103', group: 1, name: 'LED-03', enabled: true }
  ],
  groups: {
    0: { name: 'All Displays', description: 'Broadcast group' },
    1: { name: 'Zone A', description: 'Main hall' },
    2: { name: 'Zone B', description: 'Lobby' },
    3: { name: 'Zone C', description: 'Meeting rooms' }
  }
};

class C4MProxyMQTT {
  constructor() {
    this.config = DEFAULT_CONFIG;
    this.mqttClient = null;
    this.httpServer = http.createServer(this.handleHTTP.bind(this));
    this.webAdmin = new WebAdmin(this, CONFIG_FILE);
    
    // Device state tracking
    this.renderers = new Map();
    this.deviceStates = new Map(); // Track last known state per device
    
    // Statistics
    this.stats = {
      messagesReceived: 0,
      commandsProcessed: 0,
      updatesSent: 0,
      errors: 0,
      lastUpdate: Date.now(),
      startTime: Date.now()
    };
    
    // Device health tracking
    this.deviceHealth = new Map();
  }

  /**
   * Load configuration
   */
  async loadConfig() {
    try {
      const data = await fs.readFile(CONFIG_FILE, 'utf8');
      this.config = { ...DEFAULT_CONFIG, ...JSON.parse(data) };
      console.log(`✓ Loaded config: ${this.config.devices.length} devices`);
    } catch (error) {
      console.log('⚠ Config not found, using defaults');
      await this.saveConfig();
    }
  }

  /**
   * Save configuration
   */
  async saveConfig() {
    await fs.writeFile(CONFIG_FILE, JSON.stringify(this.config, null, 2));
    console.log('✓ Config saved');
  }

  /**
   * Initialize render engines for each device
   */
  initRenderers() {
    for (const device of this.config.devices) {
      if (!device.enabled) continue;
      
      const renderer = new RenderEngine(this.config.matrixWidth, this.config.matrixHeight);
      this.renderers.set(device.id, renderer);
      
      // Initialize health tracking
      this.deviceHealth.set(device.id, {
        online: false,
        lastUpdate: null,
        updateCount: 0,
        errorCount: 0
      });
    }
    console.log(`✓ Initialized ${this.renderers.size} render engines`);
  }

  /**
   * Connect to MQTT broker
   */
  async connectMQTT() {
    const options = {
      clientId: this.config.mqtt.clientId,
      clean: true,
      reconnectPeriod: 5000
    };
    
    if (this.config.mqtt.username) {
      options.username = this.config.mqtt.username;
      options.password = this.config.mqtt.password;
    }
    
    return new Promise((resolve, reject) => {
      this.mqttClient = mqtt.connect(this.config.mqtt.broker, options);
      
      this.mqttClient.on('connect', () => {
        console.log(`✓ Connected to MQTT broker: ${this.config.mqtt.broker}`);
        this.subscribeMQTT();
        this.publishStatus('online');
        resolve();
      });
      
      this.mqttClient.on('error', (error) => {
        console.error('✗ MQTT error:', error.message);
        this.stats.errors++;
      });
      
      this.mqttClient.on('message', (topic, message) => {
        this.handleMQTTMessage(topic, message);
      });
      
      this.mqttClient.on('reconnect', () => {
        console.log('⟳ Reconnecting to MQTT broker...');
      });
      
      this.mqttClient.on('offline', () => {
        console.log('⚠ MQTT client offline');
      });
    });
  }

  /**
   * Subscribe to MQTT topics
   */
  subscribeMQTT() {
    const prefix = this.config.mqtt.topicPrefix;
    
    // Subscribe to all command topics
    const topics = [
      `${prefix}/display/+/cmd/+`,      // Individual display commands
      `${prefix}/display/all/cmd/+`,    // Broadcast commands
      `${prefix}/group/+/cmd/+`,        // Group commands
      `${prefix}/proxy/cmd/+`           // Proxy control commands
    ];
    
    this.mqttClient.subscribe(topics, (err) => {
      if (err) {
        console.error('✗ MQTT subscribe error:', err);
      } else {
        console.log('✓ Subscribed to MQTT topics:', topics);
      }
    });
  }

  /**
   * Handle incoming MQTT message
   */
  async handleMQTTMessage(topic, message) {
    this.stats.messagesReceived++;
    
    try {
      const payload = JSON.parse(message.toString());
      const parts = topic.split('/');
      const prefix = this.config.mqtt.topicPrefix;
      
      // Parse topic: led/display/{id}/cmd/{command}
      //          or: led/display/all/cmd/{command}
      //          or: led/group/{group}/cmd/{command}
      
      if (parts[0] !== prefix) return;
      
      const targetType = parts[1]; // 'display', 'group', or 'proxy'
      const targetId = parts[2];    // device ID, 'all', or group number
      const cmd = parts[4];         // command name
      
      console.log(`[MQTT] ${targetType}/${targetId}/${cmd}`);
      
      // Get target devices
      let devices = [];
      
      if (targetType === 'display') {
        if (targetId === 'all') {
          devices = this.config.devices.filter(d => d.enabled);
        } else {
          const id = parseInt(targetId);
          const device = this.config.devices.find(d => d.id === id && d.enabled);
          if (device) devices = [device];
        }
      } else if (targetType === 'group') {
        const groupId = parseInt(targetId);
        devices = this.config.devices.filter(d => d.group === groupId && d.enabled);
      } else if (targetType === 'proxy') {
        await this.handleProxyCommand(cmd, payload);
        return;
      }
      
      if (devices.length === 0) {
        console.warn(`⚠ No devices found for target: ${targetType}/${targetId}`);
        return;
      }
      
      // Process command
      await this.processCommand(cmd, payload, devices);
      this.stats.commandsProcessed++;
      
    } catch (error) {
      console.error('✗ Error handling MQTT message:', error.message);
      this.stats.errors++;
      this.publishError('proxy', error.message);
    }
  }

  /**
   * Process command for target devices
   */
  async processCommand(cmd, payload, devices) {
    switch (cmd) {
      case 'segment':
        await this.handleSetSegment(payload, devices);
        break;
        
      case 'layout':
        await this.handleSetLayout(payload, devices);
        break;
        
      case 'brightness':
        await this.handleSetBrightness(payload, devices);
        break;
        
      case 'rotation':
        await this.handleSetRotation(payload, devices);
        break;
        
      case 'clear':
        await this.handleClear(payload, devices);
        break;
        
      case 'test':
        await this.handleTest(payload, devices);
        break;
        
      default:
        console.warn(`⚠ Unknown command: ${cmd}`);
    }
  }

  /**
   * Handle proxy control commands
   */
  async handleProxyCommand(cmd, payload) {
    switch (cmd) {
      case 'reload':
        await this.loadConfig();
        this.initRenderers();
        this.publishStatus('reloaded');
        break;
        
      case 'stats':
        this.publishStats();
        break;
        
      case 'devices':
        this.publishDeviceList();
        break;
        
      default:
        console.warn(`⚠ Unknown proxy command: ${cmd}`);
    }
  }

  /**
   * Handle set segment command
   */
  async handleSetSegment(payload, devices) {
    const { seg, enabled, text, color, effect, align } = payload;
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      renderer.setSegment(seg, {
        enabled: enabled !== undefined ? enabled : true,
        text: text || '',
        color: color || '#FF0000',
        effect: effect || EFFECTS.STATIC,
        align: align !== undefined ? align : ALIGN.LEFT
      });
      
      // Store state
      this.storeDeviceState(device.id, { seg, enabled, text, color, effect, align });
    }
    
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Handle set layout command
   */
  async handleSetLayout(payload, devices) {
    const { preset } = payload;
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      renderer.setLayout(preset);
      this.storeDeviceState(device.id, { layout: preset });
    }
    
    console.log(`✓ Layout preset ${preset} applied to ${devices.length} devices`);
  }

  /**
   * Handle set brightness command
   */
  async handleSetBrightness(payload, devices) {
    const { brightness } = payload;
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      renderer.setBrightness(brightness);
      this.storeDeviceState(device.id, { brightness });
    }
    
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Handle set rotation command
   */
  async handleSetRotation(payload, devices) {
    const { rotation } = payload;
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      renderer.setRotation(rotation);
      this.storeDeviceState(device.id, { rotation });
    }
    
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Handle clear command
   */
  async handleClear(payload, devices) {
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      for (let i = 0; i < 4; i++) {
        renderer.setSegment(i, { enabled: false, text: '' });
      }
      
      this.storeDeviceState(device.id, { cleared: true });
    }
    
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Handle test command
   */
  async handleTest(payload, devices) {
    const colors = ['#FF0000', '#00FF00', '#0000FF', '#FFFF00'];
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      for (let i = 0; i < 4; i++) {
        renderer.setSegment(i, {
          enabled: true,
          text: `Seg ${i} - ${device.name}`,
          color: colors[i],
          effect: EFFECTS.STATIC,
          align: ALIGN.CENTER
        });
      }
    }
    
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Render and send to devices
   */
  async renderAndSendToDevices(devices) {
    const sendPromises = devices.map(device => this.sendToDevice(device));
    const results = await Promise.allSettled(sendPromises);
    
    // Track successes/failures
    results.forEach((result, i) => {
      const device = devices[i];
      const health = this.deviceHealth.get(device.id);
      
      if (result.status === 'fulfilled') {
        health.online = true;
        health.lastUpdate = Date.now();
        health.updateCount++;
        this.stats.updatesSent++;
      } else {
        health.errorCount++;
        this.stats.errors++;
        console.error(`✗ Failed to send to ${device.name}:`, result.reason);
        this.publishError(device.id, result.reason);
      }
    });
    
    this.stats.lastUpdate = Date.now();
  }

  /**
   * Send rendered content to a single C4M device
   */
  async sendToDevice(device) {
    const renderer = this.renderers.get(device.id);
    if (!renderer) return;
    
    try {
      // Render to canvas
      const canvas = renderer.render();
      
      console.log(`→ Sending to ${device.name} (ID:${device.id}, IP:${device.ip})`);
      
      // TODO: Implement C4M SDK sending
      // Same as before - commented out until SDK is set up
      
      // Publish success status
      this.publishDeviceStatus(device.id, 'updated', {
        segments: renderer.getState().segments.filter(s => s.enabled).length
      });
      
    } catch (error) {
      console.error(`✗ Error sending to ${device.name}:`, error.message);
      throw error;
    }
  }

  /**
   * Store device state for status reporting
   */
  storeDeviceState(deviceId, state) {
    if (!this.deviceStates.has(deviceId)) {
      this.deviceStates.set(deviceId, {});
    }
    Object.assign(this.deviceStates.get(deviceId), state, { 
      timestamp: Date.now() 
    });
  }

  /**
   * Publish proxy status
   */
  publishStatus(status) {
    const topic = `${this.config.mqtt.topicPrefix}/proxy/status`;
    const payload = {
      status,
      timestamp: Date.now(),
      uptime: process.uptime(),
      devices: this.config.devices.length
    };
    
    this.mqttClient.publish(topic, JSON.stringify(payload), { retain: true });
  }

  /**
   * Publish statistics
   */
  publishStats() {
    const topic = `${this.config.mqtt.topicPrefix}/proxy/stats`;
    const uptime = (Date.now() - this.stats.startTime) / 1000;
    const rate = this.stats.updatesSent / uptime;
    
    const payload = {
      ...this.stats,
      uptime,
      updateRate: rate.toFixed(2)
    };
    
    this.mqttClient.publish(topic, JSON.stringify(payload));
  }

  /**
   * Publish device status
   */
  publishDeviceStatus(deviceId, status, data = {}) {
    const topic = `${this.config.mqtt.topicPrefix}/display/${deviceId}/status`;
    const health = this.deviceHealth.get(deviceId);
    
    const payload = {
      id: deviceId,
      status,
      timestamp: Date.now(),
      health: health || {},
      state: this.deviceStates.get(deviceId) || {},
      ...data
    };
    
    this.mqttClient.publish(topic, JSON.stringify(payload));
  }

  /**
   * Publish error message
   */
  publishError(target, message) {
    const topic = target === 'proxy'
      ? `${this.config.mqtt.topicPrefix}/proxy/error`
      : `${this.config.mqtt.topicPrefix}/display/${target}/error`;
    
    const payload = {
      error: message,
      timestamp: Date.now()
    };
    
    this.mqttClient.publish(topic, JSON.stringify(payload));
  }

  /**
   * Publish device list
   */
  publishDeviceList() {
    const topic = `${this.config.mqtt.topicPrefix}/proxy/devices`;
    
    const devices = this.config.devices.map(d => ({
      id: d.id,
      name: d.name,
      group: d.group,
      enabled: d.enabled,
      health: this.deviceHealth.get(d.id) || {}
    }));
    
    this.mqttClient.publish(topic, JSON.stringify(devices));
  }

  /**
   * Start HTTP web server
   */
  startHTTP() {
    this.httpServer.listen(this.config.webPort, () => {
      console.log(`✓ Web UI available at http://localhost:${this.config.webPort}`);
    });
  }

  /**
   * Handle HTTP requests
   */
  async handleHTTP(req, res) {
    // Delegate to web admin
    return this.webAdmin.handleRequest(req, res);
  }

  /**
   * Get statistics
   */
  getStats() {
    const uptime = (Date.now() - this.stats.startTime) / 1000;
    const rate = this.stats.updatesSent / uptime;
    
    return {
      ...this.stats,
      uptime,
      updateRate: rate.toFixed(2),
      devices: this.config.devices.length,
      devicesOnline: Array.from(this.deviceHealth.values()).filter(h => h.online).length
    };
  }

  /**
   * Generate web UI
   */
  generateWebUI() {
    return `
<!DOCTYPE html>
<html>
<head>
  <title>C4M LED Matrix Proxy (MQTT)</title>
  <style>
    body { font-family: monospace; background: #1a1a1a; color: #00ff00; padding: 20px; }
    .section { border: 1px solid #00ff00; padding: 15px; margin: 15px 0; }
    .stats { display: grid; grid-template-columns: 250px 1fr; gap: 10px; }
    .device { padding: 8px; border-bottom: 1px solid #333; display: grid; grid-template-columns: 50px 1fr 150px 150px; gap: 10px; }
    .device:hover { background: #222; }
    h1, h2 { color: #00ffff; }
    .online { color: #00ff00; }
    .offline { color: #666; }
    .error { color: #ff0000; }
    .mqtt-topic { color: #ffff00; font-size: 0.9em; }
    code { background: #333; padding: 2px 6px; border-radius: 3px; }
  </style>
</head>
<body>
  <h1>C4M LED Matrix Proxy Server (MQTT)</h1>
  
  <div class="section">
    <h2>MQTT Broker</h2>
    <div class="stats">
      <span>Broker:</span><span>${this.config.mqtt.broker}</span>
      <span>Client ID:</span><span>${this.config.mqtt.clientId}</span>
      <span>Topic Prefix:</span><span class="mqtt-topic">${this.config.mqtt.topicPrefix}</span>
      <span>Status:</span><span id="mqtt-status" class="online">Connected</span>
    </div>
  </div>
  
  <div class="section">
    <h2>Statistics</h2>
    <div class="stats" id="stats">
      <span>Uptime:</span><span id="uptime">-</span>
      <span>Messages Received:</span><span id="messages">-</span>
      <span>Commands Processed:</span><span id="commands">-</span>
      <span>Updates Sent:</span><span id="updates">-</span>
      <span>Update Rate:</span><span id="rate">-</span>
      <span>Errors:</span><span id="errors">-</span>
      <span>Last Update:</span><span id="lastUpdate">-</span>
      <span>Devices Online:</span><span id="devicesOnline">-</span>
    </div>
  </div>
  
  <div class="section">
    <h2>Devices (${this.config.devices.length})</h2>
    <div id="devices"></div>
  </div>
  
  <div class="section">
    <h2>MQTT Topic Examples</h2>
    <p>Command topics (Q-SYS → Proxy):</p>
    <pre>
# Set segment on display ID 42
${this.config.mqtt.topicPrefix}/display/42/cmd/segment
Payload: {"seg":0,"text":"Hello","color":"FF0000"}

# Broadcast to all displays
${this.config.mqtt.topicPrefix}/display/all/cmd/segment

# Send to group 1 (Zone A)
${this.config.mqtt.topicPrefix}/group/1/cmd/segment

# Test pattern on display 5
${this.config.mqtt.topicPrefix}/display/5/cmd/test
    </pre>
  </div>
  
  <script>
    function updateStats() {
      fetch('/api/stats')
        .then(r => r.json())
        .then(data => {
          document.getElementById('uptime').textContent = data.uptime.toFixed(0) + 's';
          document.getElementById('messages').textContent = data.messagesReceived;
          document.getElementById('commands').textContent = data.commandsProcessed;
          document.getElementById('updates').textContent = data.updatesSent;
          document.getElementById('rate').textContent = data.updateRate + ' Hz';
          document.getElementById('errors').textContent = data.errors;
          document.getElementById('lastUpdate').textContent = new Date(data.lastUpdate).toLocaleTimeString();
          document.getElementById('devicesOnline').textContent = data.devicesOnline + '/' + data.devices;
        });
      
      fetch('/api/devices')
        .then(r => r.json())
        .then(devices => {
          const html = devices.map(d => {
            const status = d.health.online ? 'online' : 'offline';
            const statusText = d.health.online 
              ? \`✓ Online (${d.health.updateCount} updates)\`
              : '✗ Offline';
            const lastUpdate = d.health.lastUpdate 
              ? new Date(d.health.lastUpdate).toLocaleTimeString()
              : 'Never';
            
            return '<div class="device">' +
              '<span class="' + status + '">●</span> ' +
              '<strong>' + d.name + '</strong> ' +
              '<span>ID: ' + d.id + ' (Group: ' + d.group + ')</span>' +
              '<span class="' + status + '">' + statusText + '</span>' +
              '<span>' + lastUpdate + '</span>' +
              '</div>';
          }).join('');
          document.getElementById('devices').innerHTML = html;
        });
    }
    
    updateStats();
    setInterval(updateStats, 1000);
  </script>
</body>
</html>
    `;
  }

  /**
   * Start the server
   */
  async start() {
    console.log('\n🚀 C4M LED Matrix Proxy Server (MQTT)');
    console.log('=====================================\n');
    
    await this.loadConfig();
    this.initRenderers();
    await this.connectMQTT();
    this.startHTTP();
    
    // Publish initial status
    this.publishStats();
    this.publishDeviceList();
    
    console.log('\n✓ Server ready\n');
    console.log(`   MQTT Broker: ${this.config.mqtt.broker}`);
    console.log(`   MQTT Prefix: ${this.config.mqtt.topicPrefix}`);
    console.log(`   Web Port:    ${this.config.webPort}`);
    console.log(`   Devices:     ${this.config.devices.length}`);
    console.log(`   Groups:      ${Object.keys(this.config.groups).length}`);
    console.log('\n');
  }

  /**
   * Shutdown gracefully
   */
  async shutdown() {
    console.log('\n📴 Shutting down...');
    
    this.publishStatus('offline');
    
    if (this.mqttClient) {
      this.mqttClient.end();
    }
    
    this.httpServer.close();
    
    console.log('✓ Shutdown complete');
    process.exit(0);
  }
}

// Main
const server = new C4MProxyMQTT();

process.on('SIGINT', () => server.shutdown());
process.on('SIGTERM', () => server.shutdown());

server.start().catch(error => {
  console.error('✗ Fatal error:', error);
  process.exit(1);
});
