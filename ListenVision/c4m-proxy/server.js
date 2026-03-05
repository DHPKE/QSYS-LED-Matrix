/**
 * server.js
 * 
 * C4M LED Matrix Proxy Server
 * 
 * Receives UDP commands in RPi LED Matrix protocol format from Q-SYS,
 * renders content using Canvas, and sends to 40+ C4M cards via SDK.
 * 
 * Features:
 * - Drop-in replacement for RPi - no Q-SYS plugin changes needed
 * - Handles 4-segment layout system
 * - Device grouping (broadcast to all, or specific groups)
 * - 10Hz update capability (uses C4M RAM mode)
 * - Web UI for monitoring and configuration
 * 
 * Usage:
 *   node server.js
 * 
 * Configuration:
 *   Edit config.json to define C4M devices and groups
 */

const dgram = require('dgram');
const http = require('http');
const fs = require('fs').promises;
const path = require('path');
const { RenderEngine, EFFECTS, ALIGN } = require('./render-engine');

// Note: C4M SDK wrapper commented out until ffi-napi is set up
// For now, we'll render and log what would be sent
// const { C4MSDK, LED_TYPE, SEND_TYPE, COLOR, COLOR_TYPE, EFFECT } = require('./c4m-sdk-wrapper');

const CONFIG_FILE = path.join(__dirname, 'config.json');
const DEFAULT_CONFIG = {
  udpPort: 8888,
  webPort: 8080,
  matrixWidth: 64,
  matrixHeight: 32,
  updateRate: 10, // Hz
  devices: [
    // Example device entries - edit config.json to add your 40+ devices
    { id: 1, ip: '10.1.1.101', group: 0, name: 'LED-01' },
    { id: 2, ip: '10.1.1.102', group: 0, name: 'LED-02' },
    { id: 3, ip: '10.1.1.103', group: 1, name: 'LED-03' },
    // ... add more devices
  ],
  groups: {
    0: 'Broadcast',
    1: 'Zone A',
    2: 'Zone B',
    3: 'Zone C',
    4: 'Zone D',
    5: 'Zone E',
    6: 'Zone F',
    7: 'Zone G'
  }
};

class C4MProxyServer {
  constructor() {
    this.config = DEFAULT_CONFIG;
    this.udpServer = dgram.createSocket('udp4');
    this.httpServer = http.createServer(this.handleHTTP.bind(this));
    
    // Render engines per device (or shared for broadcast)
    this.renderers = new Map();
    
    // Statistics
    this.stats = {
      packetsReceived: 0,
      commandsProcessed: 0,
      updatesSent: 0,
      errors: 0,
      lastUpdate: Date.now()
    };
    
    // Initialize SDK (commented out for now)
    // this.sdk = new C4MSDK();
    // this.sdk.initLed(LED_TYPE.C, 0);
  }

  /**
   * Load configuration from file
   */
  async loadConfig() {
    try {
      const data = await fs.readFile(CONFIG_FILE, 'utf8');
      this.config = { ...DEFAULT_CONFIG, ...JSON.parse(data) };
      console.log(`✓ Loaded config: ${this.config.devices.length} devices, ${Object.keys(this.config.groups).length} groups`);
    } catch (error) {
      console.log('⚠ Config file not found, using defaults');
      await this.saveConfig();
    }
  }

  /**
   * Save configuration to file
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
      const renderer = new RenderEngine(this.config.matrixWidth, this.config.matrixHeight);
      this.renderers.set(device.id, renderer);
    }
    console.log(`✓ Initialized ${this.renderers.size} render engines`);
  }

  /**
   * Get devices by group (0 = all/broadcast)
   */
  getDevicesByGroup(group) {
    if (group === 0) {
      return this.config.devices; // Broadcast to all
    }
    return this.config.devices.filter(d => d.group === group);
  }

  /**
   * Start UDP server
   */
  startUDP() {
    this.udpServer.on('error', (err) => {
      console.error('UDP server error:', err);
      this.stats.errors++;
    });

    this.udpServer.on('message', (msg, rinfo) => {
      this.stats.packetsReceived++;
      this.handleUDPMessage(msg, rinfo);
    });

    this.udpServer.on('listening', () => {
      const address = this.udpServer.address();
      console.log(`✓ UDP server listening on ${address.address}:${address.port}`);
    });

    this.udpServer.bind(this.config.udpPort);
  }

  /**
   * Handle incoming UDP message (RPi protocol)
   */
  handleUDPMessage(msg, rinfo) {
    try {
      const data = JSON.parse(msg.toString());
      this.processCommand(data);
      this.stats.commandsProcessed++;
    } catch (error) {
      console.error('Error parsing UDP message:', error.message);
      this.stats.errors++;
    }
  }

  /**
   * Process command from Q-SYS (matching RPi UDP protocol)
   */
  async processCommand(data) {
    const { cmd, group = 0 } = data;
    
    console.log(`[CMD] ${cmd} (group: ${group})`);
    
    // Get target devices
    const devices = this.getDevicesByGroup(group);
    
    switch (cmd) {
      case 'set_segment':
        await this.handleSetSegment(data, devices);
        break;
        
      case 'set_layout':
        await this.handleSetLayout(data, devices);
        break;
        
      case 'set_brightness':
        await this.handleSetBrightness(data, devices);
        break;
        
      case 'set_rotation':
        await this.handleSetRotation(data, devices);
        break;
        
      case 'clear':
        await this.handleClear(data, devices);
        break;
        
      case 'test':
        await this.handleTest(data, devices);
        break;
        
      case 'reboot':
        console.log('⚠ Reboot command received but not supported for C4M cards');
        break;
        
      default:
        console.warn(`⚠ Unknown command: ${cmd}`);
    }
  }

  /**
   * Handle set_segment command
   */
  async handleSetSegment(data, devices) {
    const { seg, enabled, text, color, effect, align } = data;
    
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
    }
    
    // Trigger render and send
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Handle set_layout command
   */
  async handleSetLayout(data, devices) {
    const { preset } = data;
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      renderer.setLayout(preset);
    }
    
    console.log(`✓ Layout preset ${preset} applied to ${devices.length} devices`);
  }

  /**
   * Handle set_brightness command
   */
  async handleSetBrightness(data, devices) {
    const { brightness } = data;
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      renderer.setBrightness(brightness);
    }
    
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Handle set_rotation command
   */
  async handleSetRotation(data, devices) {
    const { rotation } = data;
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      renderer.setRotation(rotation);
    }
    
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Handle clear command
   */
  async handleClear(data, devices) {
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      // Disable all segments
      for (let i = 0; i < 4; i++) {
        renderer.setSegment(i, { enabled: false, text: '' });
      }
    }
    
    await this.renderAndSendToDevices(devices);
  }

  /**
   * Handle test command
   */
  async handleTest(data, devices) {
    const colors = ['#FF0000', '#00FF00', '#0000FF', '#FFFF00'];
    
    for (const device of devices) {
      const renderer = this.renderers.get(device.id);
      if (!renderer) continue;
      
      // Enable all segments with test text
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
   * Render content and send to C4M devices
   */
  async renderAndSendToDevices(devices) {
    const sendPromises = devices.map(device => this.sendToDevice(device));
    await Promise.allSettled(sendPromises);
    
    this.stats.updatesSent += devices.length;
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
      
      // TODO: Convert canvas to C4M program and send via SDK
      // For now, just log what we would send
      console.log(`→ Sending to ${device.name} (${device.ip})`);
      
      // Uncomment when SDK is ready:
      /*
      // Save canvas as temp image
      const tempFile = path.join(__dirname, `temp_${device.id}.png`);
      await renderer.saveToFile(tempFile);
      
      // Create program using SDK
      const hProgram = this.sdk.createProgram(
        this.config.matrixWidth,
        this.config.matrixHeight,
        COLOR_TYPE.FULL,
        0,
        3 // RAM mode for 10Hz updates!
      );
      
      // Create communication info
      const commInfo = this.sdk.createCommInfo({
        ledType: LED_TYPE.C,
        sendType: SEND_TYPE.TCP,
        ip: device.ip,
        ledNumber: 1
      });
      
      // Add program
      this.sdk.addProgram(hProgram, 0, 0, 1);
      
      // Add image area
      const areaRect = this.sdk.createAreaRect(0, 0, this.config.matrixWidth, this.config.matrixHeight);
      this.sdk.addImageTextArea(hProgram, 0, 1, areaRect, 1);
      
      // Add file (rendered image)
      const playProp = this.sdk.createPlayProp({
        inStyle: EFFECT.IMMEDIATE,
        speed: 1,
        delayTime: 1
      });
      
      const result = this.sdk.lib.LV_AddFileToImageTextArea(hProgram, 0, 1, tempFile, playProp);
      if (result !== 0) {
        console.error(`✗ Error adding image: ${this.sdk.getError(result)}`);
        return;
      }
      
      // Send to card
      const sendResult = this.sdk.send(commInfo, hProgram);
      if (sendResult !== 0) {
        console.error(`✗ Error sending to ${device.name}: ${this.sdk.getError(sendResult)}`);
      } else {
        console.log(`✓ Sent to ${device.name}`);
      }
      
      // Cleanup
      this.sdk.deleteProgram(hProgram);
      await fs.unlink(tempFile).catch(() => {});
      */
      
    } catch (error) {
      console.error(`✗ Error sending to ${device.name}:`, error.message);
      this.stats.errors++;
    }
  }

  /**
   * Start HTTP web server for monitoring/config
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
    const url = new URL(req.url, `http://${req.headers.host}`);
    
    if (url.pathname === '/') {
      res.writeHead(200, { 'Content-Type': 'text/html' });
      res.end(this.generateWebUI());
    } else if (url.pathname === '/api/stats') {
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify(this.getStats()));
    } else if (url.pathname === '/api/devices') {
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify(this.config.devices));
    } else if (url.pathname === '/api/config' && req.method === 'GET') {
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify(this.config));
    } else if (url.pathname === '/api/config' && req.method === 'POST') {
      let body = '';
      req.on('data', chunk => body += chunk);
      req.on('end', async () => {
        try {
          this.config = { ...this.config, ...JSON.parse(body) };
          await this.saveConfig();
          res.writeHead(200, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ success: true }));
        } catch (error) {
          res.writeHead(400, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ error: error.message }));
        }
      });
    } else {
      res.writeHead(404);
      res.end('Not Found');
    }
  }

  /**
   * Get statistics
   */
  getStats() {
    const uptime = process.uptime();
    const rate = this.stats.updatesSent / uptime;
    
    return {
      ...this.stats,
      uptime,
      updateRate: rate.toFixed(2),
      devices: this.config.devices.length
    };
  }

  /**
   * Generate simple web UI
   */
  generateWebUI() {
    return `
<!DOCTYPE html>
<html>
<head>
  <title>C4M LED Matrix Proxy</title>
  <style>
    body { font-family: monospace; background: #1a1a1a; color: #00ff00; padding: 20px; }
    .section { border: 1px solid #00ff00; padding: 15px; margin: 15px 0; }
    .stats { display: grid; grid-template-columns: 200px 1fr; gap: 10px; }
    .device { padding: 5px; border-bottom: 1px solid #333; }
    .device:hover { background: #222; }
    h1, h2 { color: #00ffff; }
    .online { color: #00ff00; }
    .offline { color: #ff0000; }
  </style>
</head>
<body>
  <h1>C4M LED Matrix Proxy Server</h1>
  
  <div class="section">
    <h2>Statistics</h2>
    <div class="stats" id="stats">
      <span>Uptime:</span><span id="uptime">-</span>
      <span>Packets Received:</span><span id="packets">-</span>
      <span>Commands Processed:</span><span id="commands">-</span>
      <span>Updates Sent:</span><span id="updates">-</span>
      <span>Update Rate:</span><span id="rate">-</span>
      <span>Errors:</span><span id="errors">-</span>
      <span>Last Update:</span><span id="lastUpdate">-</span>
    </div>
  </div>
  
  <div class="section">
    <h2>Devices (${this.config.devices.length})</h2>
    <div id="devices"></div>
  </div>
  
  <div class="section">
    <h2>Configuration</h2>
    <pre>${JSON.stringify(this.config, null, 2)}</pre>
  </div>
  
  <script>
    function updateStats() {
      fetch('/api/stats')
        .then(r => r.json())
        .then(data => {
          document.getElementById('uptime').textContent = data.uptime.toFixed(0) + 's';
          document.getElementById('packets').textContent = data.packetsReceived;
          document.getElementById('commands').textContent = data.commandsProcessed;
          document.getElementById('updates').textContent = data.updatesSent;
          document.getElementById('rate').textContent = data.updateRate + ' Hz';
          document.getElementById('errors').textContent = data.errors;
          document.getElementById('lastUpdate').textContent = new Date(data.lastUpdate).toLocaleTimeString();
        });
      
      fetch('/api/devices')
        .then(r => r.json())
        .then(devices => {
          const html = devices.map(d => 
            '<div class="device">' +
            '<span class="online">●</span> ' +
            '<strong>' + d.name + '</strong> - ' +
            d.ip + ' (Group: ' + d.group + ')' +
            '</div>'
          ).join('');
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
    console.log('\n🚀 C4M LED Matrix Proxy Server');
    console.log('================================\n');
    
    await this.loadConfig();
    this.initRenderers();
    this.startUDP();
    this.startHTTP();
    
    console.log('\n✓ Server ready\n');
    console.log(`   UDP Port: ${this.config.udpPort}`);
    console.log(`   Web Port: ${this.config.webPort}`);
    console.log(`   Devices:  ${this.config.devices.length}`);
    console.log(`   Groups:   ${Object.keys(this.config.groups).length}`);
    console.log('\n');
  }

  /**
   * Shutdown gracefully
   */
  async shutdown() {
    console.log('\n📴 Shutting down...');
    
    this.udpServer.close();
    this.httpServer.close();
    
    // Cleanup SDK
    // if (this.sdk) { ... }
    
    console.log('✓ Shutdown complete');
    process.exit(0);
  }
}

// Main
const server = new C4MProxyServer();

process.on('SIGINT', () => server.shutdown());
process.on('SIGTERM', () => server.shutdown());

server.start().catch(error => {
  console.error('✗ Fatal error:', error);
  process.exit(1);
});
