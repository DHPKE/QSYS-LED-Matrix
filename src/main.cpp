/*
 * Olimex ESP32 Gateway LED Matrix Text Display
 * WLED-based firmware for 64x32 HUB75 LED Matrix
 * Receives UDP commands from Q-SYS plugin to display dynamic text
 * 
 * Compatible with Arduino IDE
 * 
 * Hardware:
 * - Olimex ESP32 Gateway (ALL revisions A-I)
 * - 64x32 HUB75 LED Matrix Panel
 * 
 * Author: Generated for DHPKE/OlimexLED-Matrix
 * Version: 1.2.0 - Critical fixes applied
 * 
 * CHANGELOG v1.2.0:
 * - FIXED: GPIO17→GPIO32 (Ethernet PHY conflict, rev D+ compatibility)
 * - FIXED: BRIGHTNESS command parsing (now works as documented)
 * - FIXED: UDP command bounds checking (memory safety)
 * - FIXED: Web test command now actually executes commands
 * - ADDED: Watchdog timer (10 second timeout)
 * - ADDED: WiFi configuration compile-time warning
 * - IMPROVED: UDP buffer size reduced to 256 bytes (was wasteful)
 * - IMPROVED: Error messages and validation throughout
 */

#include <Arduino.h>
#include <ETH.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <esp_task_wdt.h>  // Watchdog timer

#include "config.h"
#include "segment_manager.h"
#include "text_renderer.h"
#include "udp_handler.h"

// Watchdog timeout (10 seconds)
#define WDT_TIMEOUT 10

// Global objects
MatrixPanel_I2S_DMA *dma_display = nullptr;
SegmentManager segmentManager;
TextRenderer *textRenderer = nullptr;
UDPHandler *udpHandler = nullptr;
AsyncWebServer server(WEB_SERVER_PORT);

// Ethernet connection flag
static bool eth_connected = false;

// Configuration variables
uint8_t currentBrightness = DEFAULT_BRIGHTNESS;
unsigned long lastEffectUpdate = 0;
const unsigned long EFFECT_UPDATE_INTERVAL = 50; // 50ms for smooth effects

// Function declarations
void setupMatrix();
void setupEthernet();
void WiFiEvent(WiFiEvent_t event);
void setupWebServer();
void setupUDP();
void loadConfiguration();
void saveConfiguration();
void handleRoot(AsyncWebServerRequest *request);
void handleConfig(AsyncWebServerRequest *request);
void handleSegments(AsyncWebServerRequest *request);
void handleTest(AsyncWebServerRequest *request);

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n==================================");
    Serial.println("Olimex LED Matrix Text Display");
    Serial.println("Version: 1.2.0");
    Serial.println("==================================\n");
    
    // Initialize watchdog timer
    Serial.println("Initializing watchdog timer...");
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);
    Serial.println("✓ Watchdog enabled (10s timeout)");
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("ERROR: LittleFS Mount Failed");
    } else {
        Serial.println("✓ LittleFS mounted successfully");
    }
    
    // Load configuration
    loadConfiguration();
    
    // Setup LED Matrix
    setupMatrix();
    
    // Setup Ethernet
    setupEthernet();
    
    // Setup UDP handler
    setupUDP();
    
    // Setup web server
    setupWebServer();
    
    Serial.println("\n==================================");
    Serial.println("System Ready!");
    Serial.println("==================================");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("UDP Port: ");
    Serial.println(UDP_PORT);
    Serial.println("Web Interface: http://" + WiFi.localIP().toString());
    Serial.println("==================================\n");
    
    // Display welcome message
    Segment* seg = segmentManager.getSegment(0);
    if (seg) {
        segmentManager.updateSegmentText(0, "READY");
        seg->color = 0x07E0; // Green
        seg->autoSize = true;
        seg->align = ALIGN_CENTER;
        segmentManager.activateSegment(0, true);
    }
}

void loop() {
    // Feed watchdog timer
    esp_task_wdt_reset();
    
    // Process UDP packets
    if (udpHandler) {
        udpHandler->process();
    }
    
    // Update brightness if changed
    uint8_t newBrightness = udpHandler ? udpHandler->getBrightness() : DEFAULT_BRIGHTNESS;
    if (newBrightness != currentBrightness) {
        currentBrightness = newBrightness;
        dma_display->setBrightness8(currentBrightness);
    }
    
    // Update text effects periodically
    unsigned long now = millis();
    if (now - lastEffectUpdate >= EFFECT_UPDATE_INTERVAL) {
        segmentManager.updateEffects();
        lastEffectUpdate = now;
    }
    
    // Render segments
    if (textRenderer) {
        textRenderer->renderAll();
    }
    
    // Small delay to prevent watchdog issues
    delay(1);
}

void setupMatrix() {
    Serial.println("Initializing LED Matrix...");
    
    // HUB75 configuration
    HUB75_I2S_CFG mxconfig(
        LED_MATRIX_WIDTH,   // Module width
        LED_MATRIX_HEIGHT,  // Module height
        MATRIX_CHAIN        // Chain length
    );
    
    // Pin configuration for Olimex ESP32 Gateway
    mxconfig.gpio.r1 = R1_PIN;
    mxconfig.gpio.g1 = G1_PIN;
    mxconfig.gpio.b1 = B1_PIN;
    mxconfig.gpio.r2 = R2_PIN;
    mxconfig.gpio.g2 = G2_PIN;
    mxconfig.gpio.b2 = B2_PIN;
    
    mxconfig.gpio.a = A_PIN;
    mxconfig.gpio.b = B_PIN;
    mxconfig.gpio.c = C_PIN;
    mxconfig.gpio.d = D_PIN;
    mxconfig.gpio.e = E_PIN;
    
    mxconfig.gpio.lat = LAT_PIN;
    mxconfig.gpio.oe = OE_PIN;
    mxconfig.gpio.clk = CLK_PIN;
    
    // Create display object
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    
    // Initialize display
    if (!dma_display->begin()) {
        Serial.println("ERROR: Matrix initialization failed!");
        return;
    }
    
    Serial.println("✓ LED Matrix initialized");
    
    // Set brightness
    dma_display->setBrightness8(currentBrightness);
    
    // Clear display
    dma_display->clearScreen();
    
    // Create text renderer
    textRenderer = new TextRenderer(dma_display, &segmentManager);
    
    Serial.print("✓ Matrix size: ");
    Serial.print(LED_MATRIX_WIDTH);
    Serial.print("x");
    Serial.println(LED_MATRIX_HEIGHT);
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("ETH Started");
            ETH.setHostname("olimex-led-matrix");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("ETH Connected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.println("✓ Ethernet connected");
            Serial.print("  IP Address: ");
            Serial.println(ETH.localIP());
            Serial.print("  Gateway: ");
            Serial.println(ETH.gatewayIP());
            Serial.print("  Subnet: ");
            Serial.println(ETH.subnetMask());
            Serial.print("  MAC Address: ");
            Serial.println(ETH.macAddress());
            Serial.print("  Link Speed: ");
            Serial.print(ETH.linkSpeed());
            Serial.println("Mbps");
            eth_connected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("ETH Disconnected");
            eth_connected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("ETH Stopped");
            eth_connected = false;
            break;
        default:
            break;
    }
}

void setupEthernet() {
    Serial.println("Initializing Ethernet...");
    
    // Register event handler
    WiFi.onEvent(WiFiEvent);
    
    // Olimex ESP32 Gateway Ethernet configuration
    // PHY Type: LAN8720
    // PHY Address: 0
    // Power Pin: 5 (enable pin)
    // MDC Pin: 23
    // MDIO Pin: 18
    // Clock Mode: ETH_CLOCK_GPIO17_OUT (external oscillator)
    
    if (!ETH.begin(0, 5, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO17_OUT)) {
        Serial.println("ERROR: Ethernet initialization failed!");
        return;
    }
    
    Serial.println("Waiting for Ethernet connection...");
    
    // Wait up to 10 seconds for connection
    int attempts = 0;
    while (!eth_connected && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (!eth_connected) {
        Serial.println("WARNING: Ethernet not connected yet (will continue trying)");
    }
}

void setupUDP() {
    Serial.println("Starting UDP listener...");
    
    udpHandler = new UDPHandler(&segmentManager);
    
    if (udpHandler->begin()) {
        Serial.print("✓ UDP listening on port ");
        Serial.println(UDP_PORT);
    } else {
        Serial.println("ERROR: UDP initialization failed!");
    }
}

void setupWebServer() {
    Serial.println("Starting web server...");
    
    // Serve static files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // API endpoints
    server.on("/api/config", HTTP_GET, handleConfig);
    server.on("/api/segments", HTTP_GET, handleSegments);
    server.on("/api/test", HTTP_POST, handleTest);
    
    // Root handler
    server.on("/", HTTP_GET, handleRoot);
    
    // 404 handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
    
    server.begin();
    Serial.println("✓ Web server started on port 80");
}

void loadConfiguration() {
    Serial.println("Loading configuration...");
    
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("  No config file found, using defaults");
        return;
    }
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.print("  ERROR: Failed to parse config: ");
        Serial.println(error.c_str());
        return;
    }
    
    currentBrightness = doc["brightness"] | DEFAULT_BRIGHTNESS;
    
    Serial.println("✓ Configuration loaded");
}

void saveConfiguration() {
    StaticJsonDocument<512> doc;
    
    doc["brightness"] = currentBrightness;
    doc["udp_port"] = UDP_PORT;
    
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("ERROR: Failed to save configuration");
        return;
    }
    
    serializeJson(doc, file);
    file.close();
    
    Serial.println("✓ Configuration saved");
}

void handleRoot(AsyncWebServerRequest *request) {
    // Build dynamic values
    String ipAddress = ETH.localIP().toString();
    String udpPort = String(UDP_PORT);
    String matrixSize = String(LED_MATRIX_WIDTH) + "x" + String(LED_MATRIX_HEIGHT);
    
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>LED Matrix Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            background: linear-gradient(135deg, #0f0f1e 0%, #1a1a2e 100%);
            color: #e0e0e0;
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
            padding: 30px 20px;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            border-radius: 15px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        }
        
        .header h1 {
            font-size: 2.5em;
            font-weight: 700;
            color: #fff;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
        }
        
        .header .subtitle {
            color: #a0c4ff;
            font-size: 1.1em;
        }
        
        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin-bottom: 20px;
        }
        
        .segments-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-bottom: 20px;
        }
        
        @media (max-width: 768px) {
            .grid {
                grid-template-columns: 1fr;
            }
            .segments-grid {
                grid-template-columns: 1fr;
            }
        }
        
        .card {
            background: rgba(30, 30, 46, 0.9);
            border-radius: 12px;
            padding: 20px;
            box-shadow: 0 4px 16px rgba(0, 0, 0, 0.2);
            border: 1px solid rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
        }
        
        .card.compact {
            padding: 15px;
            transition: all 0.3s ease;
            position: relative;
        }
        
        .card.compact.inactive {
            opacity: 0.4;
            pointer-events: none;
            filter: grayscale(0.7);
        }
        
        .card.compact.inactive::after {
            content: 'INACTIVE';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: rgba(0, 0, 0, 0.85);
            color: #666;
            padding: 8px 20px;
            border-radius: 6px;
            font-weight: 700;
            font-size: 0.9em;
            letter-spacing: 2px;
            z-index: 10;
            border: 2px solid rgba(255, 255, 255, 0.15);
        }
        
        .card h2 {
            font-size: 1.2em;
            margin-bottom: 15px;
            color: #4a9eff;
            border-bottom: 2px solid #4a9eff;
            padding-bottom: 8px;
        }
        
        .card.compact h2 {
            font-size: 1em;
            margin-bottom: 12px;
            padding-bottom: 6px;
        }
        
        .info-grid {
            display: flex;
            gap: 20px;
            align-items: center;
            flex-wrap: wrap;
        }
        
        .info-item {
            display: flex;
            flex-direction: column;
            gap: 6px;
            align-items: flex-start;
        }
        
        .info-label {
            color: #888;
            font-weight: 600;
            font-size: 0.85em;
            white-space: nowrap;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .info-value {
            color: #e0e0e0;
            font-weight: 500;
            font-size: 0.9em;
        }
        
        .info-value input {
            background: rgba(0, 0, 0, 0.3);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 4px;
            color: #e0e0e0;
            padding: 6px 10px;
            font-size: 0.9em;
            font-family: 'Courier New', monospace;
            width: 100%;
            min-width: 140px;
        }
        
        .info-value input:focus {
            outline: none;
            border-color: #2563eb;
            background: rgba(37, 99, 235, 0.1);
        }
        
        .status {
            padding: 8px 15px;
            border-radius: 20px;
            font-size: 0.9em;
            font-weight: 600;
            display: inline-block;
        }
        
        .status.ready { background: #1a472a; color: #4ade80; }
        .status.sending { background: #1e3a8a; color: #60a5fa; }
        .status.error { background: #7f1d1d; color: #f87171; }
        
        .preview-section {
            grid-column: 1 / -1;
            text-align: center;
        }
        
        #preview {
            border: 2px solid #333;
            background: #000;
            border-radius: 8px;
            image-rendering: pixelated;
            image-rendering: -moz-crisp-edges;
            image-rendering: crisp-edges;
            box-shadow: 0 0 20px rgba(74, 158, 255, 0.3);
        }
        
        .form-group {
            margin-bottom: 12px;
        }
        
        .form-group.compact {
            margin-bottom: 8px;
        }
        
        label {
            display: block;
            margin-bottom: 6px;
            color: #a0a0a0;
            font-weight: 600;
            font-size: 0.85em;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        input[type="text"],
        input[type="color"],
        select {
            width: 100%;
            padding: 10px 12px;
            background: rgba(0, 0, 0, 0.4);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            color: #e0e0e0;
            font-size: 0.95em;
            transition: all 0.3s ease;
        }
        
        .compact input[type="text"],
        .compact select {
            padding: 8px 10px;
            font-size: 0.9em;
        }
        
        input[type="text"]:focus,
        select:focus {
            outline: none;
            border-color: #4a9eff;
            box-shadow: 0 0 0 3px rgba(74, 158, 255, 0.1);
        }
        
        input[type="color"] {
            height: 40px;
            cursor: pointer;
            padding: 3px;
        }
        
        .compact input[type="color"] {
            height: 36px;
        }
        
        input[type="range"] {
            width: 100%;
            height: 8px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 5px;
            outline: none;
            -webkit-appearance: none;
        }
        
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            background: #4a9eff;
            border-radius: 50%;
            cursor: pointer;
            box-shadow: 0 2px 8px rgba(74, 158, 255, 0.5);
        }
        
        input[type="range"]::-moz-range-thumb {
            width: 20px;
            height: 20px;
            background: #4a9eff;
            border-radius: 50%;
            cursor: pointer;
            border: none;
        }
        
        .button-group {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 8px;
            margin-top: 12px;
        }
        
        .button-group.compact {
            gap: 6px;
            margin-top: 10px;
        }
        
        button {
            padding: 10px 16px;
            border: none;
            border-radius: 8px;
            font-size: 0.9em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .compact button {
            padding: 8px 12px;
            font-size: 0.85em;
        }
        
        .btn-primary {
            background: #2563eb;
            color: white;
        }
        
        .btn-primary:hover {
            background: #1d4ed8;
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(37, 99, 235, 0.4);
        }
        
        .btn-danger {
            background: #ef4444;
            color: white;
        }
        
        .btn-danger:hover {
            background: #dc2626;
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(239, 68, 68, 0.4);
        }
        
        .btn-clear-all {
            grid-column: 1 / -1;
            background: #dc2626;
            color: white;
        }
        
        .btn-clear-all:hover {
            background: #b91c1c;
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(220, 38, 38, 0.4);
        }
        
        .align-group {
            display: flex;
            gap: 8px;
            margin-top: 5px;
        }
        
        .compact .align-group {
            gap: 6px;
        }
        
        .align-btn {
            flex: 1;
            padding: 6px;
            background: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.2);
            border-radius: 6px;
            color: #888;
            cursor: pointer;
            transition: all 0.2s;
            font-size: 0.85em;
        }
        
        .compact .align-btn {
            padding: 5px;
            font-size: 0.8em;
        }
        
        .align-btn.active {
            background: #2563eb;
            border-color: #2563eb;
            color: white;
        }
        
        .align-btn:hover {
            border-color: #2563eb;
        }
        
        .brightness-display {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-top: 5px;
        }
        
        .brightness-value {
            background: rgba(74, 158, 255, 0.2);
            color: #4a9eff;
            padding: 5px 15px;
            border-radius: 20px;
            font-weight: 700;
            font-size: 1.1em;
        }
        
        .layout-btn {
            padding: 0;
            height: 80px;
            background: rgba(255, 255, 255, 0.05);
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        
        .layout-btn:hover {
            background: rgba(37, 99, 235, 0.2);
            border-color: #2563eb;
            transform: translateY(-2px);
        }
        
        .layout-visual {
            display: grid;
            width: 48px;
            height: 32px;
            gap: 2px;
            background: #000;
            border-radius: 3px;
            padding: 2px;
        }
        
        .layout-visual.split-v {
            grid-template-columns: 1fr 1fr;
        }
        
        .layout-visual.split-h {
            grid-template-rows: 1fr 1fr;
        }
        
        .layout-visual.quad {
            grid-template-columns: 1fr 1fr;
            grid-template-rows: 1fr 1fr;
        }
        
        .layout-visual.full {
            padding: 0;
        }
        
        .layout-segment {
            background: #2563eb;
            border-radius: 2px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 9px;
            font-weight: 700;
            color: white;
        }
        
        .layout-label {
            font-size: 0.75em;
            color: #888;
            font-weight: 500;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>LED Matrix Controller</h1>
            <div class="subtitle">64x32 RGB Display Management</div>
        </div>
        
        <div class="grid">
            <div class="card" style="grid-column: 1 / -1;">
                <div style="display: flex; justify-content: space-between; align-items: flex-start; flex-wrap: wrap; gap: 20px;">
                    <div>
                        <h2 style="margin: 0 0 15px 0;">Network Information</h2>
                        <div style="display: flex; gap: 20px; flex-wrap: wrap;">
                            <div class="info-item">
                                <span class="info-label">IP Address</span>
                                <div class="info-value"><input type="text" id="ip-address" value="{{IP_ADDRESS}}" placeholder="192.168.1.100"></div>
                            </div>
                            <div class="info-item">
                                <span class="info-label">UDP Port</span>
                                <div class="info-value"><input type="number" id="udp-port" value="{{UDP_PORT}}" placeholder="21324" min="1" max="65535" style="width: 100px;"></div>
                            </div>
                            <div class="info-item">
                                <span class="info-label">Hostname</span>
                                <div class="info-value"><input type="text" id="hostname" value="led-matrix" placeholder="led-matrix"></div>
                            </div>
                            <div class="info-item">
                                <span class="info-label">Display Size</span>
                                <span class="info-value" style="padding: 6px 10px; background: rgba(0,0,0,0.3); border-radius: 4px; border: 1px solid rgba(255,255,255,0.1);">{{MATRIX_SIZE}}</span>
                            </div>
                        </div>
                    </div>
                    <div style="align-self: center;">
                        <span id="status" class="status ready">Ready</span>
                    </div>
                </div>
            </div>
            
            <div class="card preview-section">
                <h2>Live Preview</h2>
                <canvas id="preview" width="64" height="32" style="width: 100%; max-width: 640px; height: auto;"></canvas>
            </div>
            
            <div class="card" style="grid-column: 1 / -1;">
                <h2>Segment Layouts</h2>
                <div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px;">
                    <button class="layout-btn" onclick="applyLayout('split-vertical')">
                        <div class="layout-visual split-v">
                            <div class="layout-segment">1</div>
                            <div class="layout-segment">2</div>
                        </div>
                        <span class="layout-label">1 | 2</span>
                    </button>
                    <button class="layout-btn" onclick="applyLayout('split-horizontal')">
                        <div class="layout-visual split-h">
                            <div class="layout-segment">1</div>
                            <div class="layout-segment">2</div>
                        </div>
                        <span class="layout-label">1 / 2</span>
                    </button>
                    <button class="layout-btn" onclick="applyLayout('quad')">
                        <div class="layout-visual quad">
                            <div class="layout-segment">1</div>
                            <div class="layout-segment">2</div>
                            <div class="layout-segment">3</div>
                            <div class="layout-segment">4</div>
                        </div>
                        <span class="layout-label">2x2</span>
                    </button>
                    <button class="layout-btn" onclick="applyLayout('fullscreen')">
                        <div class="layout-visual full">
                            <div class="layout-segment" style="width: 100%; height: 100%; border-radius: 3px;">ALL</div>
                        </div>
                        <span class="layout-label">Full</span>
                    </button>
                </div>
            </div>
            </div>
            
            <div class="segments-grid">
            <div class="card compact" id="segment-card-0">
                <h2>Segment 1</h2>
                <div class="form-group">
                    <label>Text Message</label>
                    <input type="text" id="text0" placeholder="Enter your message...">
                </div>
                <div class="form-group">
                    <label>Text Color</label>
                    <select id="color0">
                        <option value="#FFFFFF" selected>White</option>
                        <option value="#FF0000">Red</option>
                        <option value="#00FF00">Lime</option>
                        <option value="#0000FF">Blue</option>
                        <option value="#FFFF00">Yellow</option>
                        <option value="#FF00FF">Magenta</option>
                        <option value="#00FFFF">Cyan</option>
                        <option value="#FFA500">Orange</option>
                        <option value="#800080">Purple</option>
                        <option value="#008000">Green</option>
                        <option value="#FFC0CB">Pink</option>
                        <option value="#FFD700">Gold</option>
                        <option value="#C0C0C0">Silver</option>
                        <option value="#808080">Gray</option>
                        <option value="#000000">Black</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Background Color</label>
                    <select id="bgcolor0">
                        <option value="#000000" selected>Black</option>
                        <option value="#FFFFFF">White</option>
                        <option value="#FF0000">Red</option>
                        <option value="#00FF00">Lime</option>
                        <option value="#0000FF">Blue</option>
                        <option value="#FFFF00">Yellow</option>
                        <option value="#FF00FF">Magenta</option>
                        <option value="#00FFFF">Cyan</option>
                        <option value="#FFA500">Orange</option>
                        <option value="#800080">Purple</option>
                        <option value="#008000">Green</option>
                        <option value="#FFC0CB">Pink</option>
                        <option value="#FFD700">Gold</option>
                        <option value="#C0C0C0">Silver</option>
                        <option value="#808080">Gray</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Intensity</label>
                    <input type="range" id="intensity0" min="0" max="255" value="255" oninput="document.getElementById('intensity-val0').textContent=this.value">
                    <span id="intensity-val0" style="color: #888; font-size: 0.85em;">255</span>
                </div>
                <div class="form-group">
                    <label>Font Style</label>
                    <select id="font0">
                        <option value="arial">Arial (Sans-Serif)</option>
                        <option value="verdana">Verdana (Sans-Serif)</option>
                        <option value="digital12">Digital 12pt</option>
                        <option value="mono9">Mono 9pt</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Text Alignment</label>
                    <div class="align-group">
                        <button class="align-btn" onclick="setAlign(0, 'L', this)">Left</button>
                        <button class="align-btn active" onclick="setAlign(0, 'C', this)">Center</button>
                        <button class="align-btn" onclick="setAlign(0, 'R', this)">Right</button>
                    </div>
                </div>
                <div class="button-group">
                    <button class="btn-primary" onclick="sendText(0)">Display</button>
                    <button class="btn-primary" onclick="previewText(0)">Preview</button>
                    <button class="btn-danger" onclick="clearSegment(0)">Clear</button>
                </div>
            </div>
            
            <div class="card compact" id="segment-card-1">
                <h2>Segment 2</h2>
                <div class="form-group">
                    <label>Text Message</label>
                    <input type="text" id="text1" placeholder="Enter your message...">
                </div>
                <div class="form-group">
                    <label>Text Color</label>
                    <select id="color1">
                        <option value="#FFFFFF">White</option>
                        <option value="#FF0000">Red</option>
                        <option value="#00FF00" selected>Lime</option>
                        <option value="#0000FF">Blue</option>
                        <option value="#FFFF00">Yellow</option>
                        <option value="#FF00FF">Magenta</option>
                        <option value="#00FFFF">Cyan</option>
                        <option value="#FFA500">Orange</option>
                        <option value="#800080">Purple</option>
                        <option value="#008000">Green</option>
                        <option value="#FFC0CB">Pink</option>
                        <option value="#FFD700">Gold</option>
                        <option value="#C0C0C0">Silver</option>
                        <option value="#808080">Gray</option>
                        <option value="#000000">Black</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Background Color</label>
                    <select id="bgcolor1">
                        <option value="#000000" selected>Black</option>
                        <option value="#FFFFFF">White</option>
                        <option value="#FF0000">Red</option>
                        <option value="#00FF00">Lime</option>
                        <option value="#0000FF">Blue</option>
                        <option value="#FFFF00">Yellow</option>
                        <option value="#FF00FF">Magenta</option>
                        <option value="#00FFFF">Cyan</option>
                        <option value="#FFA500">Orange</option>
                        <option value="#800080">Purple</option>
                        <option value="#008000">Green</option>
                        <option value="#FFC0CB">Pink</option>
                        <option value="#FFD700">Gold</option>
                        <option value="#C0C0C0">Silver</option>
                        <option value="#808080">Gray</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Intensity</label>
                    <input type="range" id="intensity1" min="0" max="255" value="255" oninput="document.getElementById('intensity-val1').textContent=this.value">
                    <span id="intensity-val1" style="color: #888; font-size: 0.85em;">255</span>
                </div>
                <div class="form-group">
                    <label>Font Style</label>
                    <select id="font1">
                        <option value="arial">Arial (Sans-Serif)</option>
                        <option value="verdana">Verdana (Sans-Serif)</option>
                        <option value="digital12">Digital 12pt</option>
                        <option value="mono9">Mono 9pt</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Text Alignment</label>
                    <div class="align-group">
                        <button class="align-btn" onclick="setAlign(1, 'L', this)">Left</button>
                        <button class="align-btn active" onclick="setAlign(1, 'C', this)">Center</button>
                        <button class="align-btn" onclick="setAlign(1, 'R', this)">Right</button>
                    </div>
                </div>
                <div class="button-group">
                    <button class="btn-primary" onclick="sendText(1)">Display</button>
                    <button class="btn-primary" onclick="previewText(1)">Preview</button>
                    <button class="btn-danger" onclick="clearSegment(1)">Clear</button>
                </div>
            </div>
            
            <div class="card compact" id="segment-card-2">
                <h2>Segment 3</h2>
                <div class="form-group">
                    <label>Text Message</label>
                    <input type="text" id="text2" placeholder="Enter your message...">
                </div>
                <div class="form-group">
                    <label>Text Color</label>
                    <select id="color2">
                        <option value="#FFFFFF">White</option>
                        <option value="#FF0000" selected>Red</option>
                        <option value="#00FF00">Lime</option>
                        <option value="#0000FF">Blue</option>
                        <option value="#FFFF00">Yellow</option>
                        <option value="#FF00FF">Magenta</option>
                        <option value="#00FFFF">Cyan</option>
                        <option value="#FFA500">Orange</option>
                        <option value="#800080">Purple</option>
                        <option value="#008000">Green</option>
                        <option value="#FFC0CB">Pink</option>
                        <option value="#FFD700">Gold</option>
                        <option value="#C0C0C0">Silver</option>
                        <option value="#808080">Gray</option>
                        <option value="#000000">Black</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Background Color</label>
                    <select id="bgcolor2">
                        <option value="#000000" selected>Black</option>
                        <option value="#FFFFFF">White</option>
                        <option value="#FF0000">Red</option>
                        <option value="#00FF00">Lime</option>
                        <option value="#0000FF">Blue</option>
                        <option value="#FFFF00">Yellow</option>
                        <option value="#FF00FF">Magenta</option>
                        <option value="#00FFFF">Cyan</option>
                        <option value="#FFA500">Orange</option>
                        <option value="#800080">Purple</option>
                        <option value="#008000">Green</option>
                        <option value="#FFC0CB">Pink</option>
                        <option value="#FFD700">Gold</option>
                        <option value="#C0C0C0">Silver</option>
                        <option value="#808080">Gray</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Intensity</label>
                    <input type="range" id="intensity2" min="0" max="255" value="255" oninput="document.getElementById('intensity-val2').textContent=this.value">
                    <span id="intensity-val2" style="color: #888; font-size: 0.85em;">255</span>
                </div>
                <div class="form-group">
                    <label>Font Style</label>
                    <select id="font2">
                        <option value="arial">Arial (Sans-Serif)</option>
                        <option value="verdana">Verdana (Sans-Serif)</option>
                        <option value="digital12">Digital 12pt</option>
                        <option value="mono9">Mono 9pt</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Text Alignment</label>
                    <div class="align-group">
                        <button class="align-btn" onclick="setAlign(2, 'L', this)">Left</button>
                        <button class="align-btn active" onclick="setAlign(2, 'C', this)">Center</button>
                        <button class="align-btn" onclick="setAlign(2, 'R', this)">Right</button>
                    </div>
                </div>
                <div class="button-group">
                    <button class="btn-primary" onclick="sendText(2)">Display</button>
                    <button class="btn-primary" onclick="previewText(2)">Preview</button>
                    <button class="btn-danger" onclick="clearSegment(2)">Clear</button>
                </div>
            </div>
            
            <div class="card compact" id="segment-card-3">
                <h2>Segment 4</h2>
                <div class="form-group">
                    <label>Text Message</label>
                    <input type="text" id="text3" placeholder="Enter your message...">
                </div>
                <div class="form-group">
                    <label>Text Color</label>
                    <select id="color3">
                        <option value="#FFFFFF">White</option>
                        <option value="#FF0000">Red</option>
                        <option value="#00FF00">Lime</option>
                        <option value="#0000FF">Blue</option>
                        <option value="#FFFF00" selected>Yellow</option>
                        <option value="#FF00FF">Magenta</option>
                        <option value="#00FFFF">Cyan</option>
                        <option value="#FFA500">Orange</option>
                        <option value="#800080">Purple</option>
                        <option value="#008000">Green</option>
                        <option value="#FFC0CB">Pink</option>
                        <option value="#FFD700">Gold</option>
                        <option value="#C0C0C0">Silver</option>
                        <option value="#808080">Gray</option>
                        <option value="#000000">Black</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Background Color</label>
                    <select id="bgcolor3">
                        <option value="#000000" selected>Black</option>
                        <option value="#FFFFFF">White</option>
                        <option value="#FF0000">Red</option>
                        <option value="#00FF00">Lime</option>
                        <option value="#0000FF">Blue</option>
                        <option value="#FFFF00">Yellow</option>
                        <option value="#FF00FF">Magenta</option>
                        <option value="#00FFFF">Cyan</option>
                        <option value="#FFA500">Orange</option>
                        <option value="#800080">Purple</option>
                        <option value="#008000">Green</option>
                        <option value="#FFC0CB">Pink</option>
                        <option value="#FFD700">Gold</option>
                        <option value="#C0C0C0">Silver</option>
                        <option value="#808080">Gray</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Intensity</label>
                    <input type="range" id="intensity3" min="0" max="255" value="255" oninput="document.getElementById('intensity-val3').textContent=this.value">
                    <span id="intensity-val3" style="color: #888; font-size: 0.85em;">255</span>
                </div>
                <div class="form-group">
                    <label>Font Style</label>
                    <select id="font3">
                        <option value="arial">Arial (Sans-Serif)</option>
                        <option value="verdana">Verdana (Sans-Serif)</option>
                        <option value="digital12">Digital 12pt</option>
                        <option value="mono9">Mono 9pt</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Text Alignment</label>
                    <div class="align-group">
                        <button class="align-btn" onclick="setAlign(3, 'L', this)">Left</button>
                        <button class="align-btn active" onclick="setAlign(3, 'C', this)">Center</button>
                        <button class="align-btn" onclick="setAlign(3, 'R', this)">Right</button>
                    </div>
                </div>
                <div class="button-group">
                    <button class="btn-primary" onclick="sendText(3)">Display</button>
                    <button class="btn-primary" onclick="previewText(3)">Preview</button>
                    <button class="btn-danger" onclick="clearSegment(3)">Clear</button>
                </div>
            </div>
            </div>
            
            <div class="card" style="grid-column: 1 / -1;">
                <h2>Display Settings</h2>
                <div class="form-group">
                    <label>Brightness Control</label>
                    <input type="range" id="brightness" min="0" max="255" value="128" oninput="updateBrightness(this.value)">
                    <div class="brightness-display">
                        <span style="color: #888;">Dim</span>
                        <span class="brightness-value" id="brightness-value">128</span>
                        <span style="color: #888;">Bright</span>
                    </div>
                </div>
                <button class="btn-clear-all" onclick="clearAll()">Clear All Segments</button>
            </div>
        </div>
    </div>
    
    <script>
        const canvas = document.getElementById('preview');
        const ctx = canvas.getContext('2d');
        
        // Store alignment for each segment
        const segmentAlign = ['C', 'C', 'C', 'C'];
        
        // Store segment bounds (x, y, width, height)
        const segmentBounds = [
            { x: 0, y: 0, width: 32, height: 32 },  // Segment 1 (left half default)
            { x: 32, y: 0, width: 32, height: 32 }, // Segment 2 (right half default)
            { x: 0, y: 16, width: 32, height: 16 }, // Segment 3 (bottom-left default)
            { x: 32, y: 16, width: 32, height: 16 } // Segment 4 (bottom-right default)
        ];
        
        // Initialize black canvas
        ctx.fillStyle = '#000000';
        ctx.fillRect(0, 0, 64, 32);
        
        // Initialize with split-vertical layout (default)
        window.addEventListener('load', function() {
            updateSegmentStates([0, 1]); // Only segments 1 and 2 active by default
        });
        
        function updateStatus(message, type = 'ready') {
            const statusEl = document.getElementById('status');
            statusEl.textContent = message;
            statusEl.className = 'status ' + type;
        }
        
        function setAlign(segment, align, btn) {
            segmentAlign[segment] = align;
            
            // Update active state for buttons in this segment's group
            const parent = btn.parentElement;
            parent.querySelectorAll('.align-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            
            updateStatus('Alignment set to ' + (align === 'L' ? 'Left' : align === 'C' ? 'Center' : 'Right'), 'ready');
        }
        
        function updateSegmentStates(activeSegments) {
            // activeSegments is an array of segment indices (0-3) that should be active
            for (let i = 0; i < 4; i++) {
                const card = document.getElementById('segment-card-' + i);
                if (card) {
                    if (activeSegments.includes(i)) {
                        card.classList.remove('inactive');
                    } else {
                        card.classList.add('inactive');
                    }
                }
            }
        }
        
        function applyLayout(layoutType) {
            updateStatus('Applying ' + layoutType + ' layout...', 'sending');
            
            let commands = [];
            
            // Save current segment 1 settings before layout change
            const seg0Text = document.getElementById('text0').value;
            const seg0Color = document.getElementById('color0').value;
            const seg0BgColor = document.getElementById('bgcolor0').value;
            const seg0Intensity = document.getElementById('intensity0').value;
            const seg0Font = document.getElementById('font0').value;
            const seg0Align = segmentAlign[0];
            
            if (layoutType === 'split-vertical') {
                // 1 | 2: Left half (0->32px) | Right half (32->64px), full height
                // Only segments 1 and 2 are active
                updateSegmentStates([0, 1]);
                segmentBounds[0] = { x: 0, y: 0, width: 32, height: 32 };
                segmentBounds[1] = { x: 32, y: 0, width: 32, height: 32 };
                segmentBounds[2] = { x: 0, y: 0, width: 0, height: 0 };
                segmentBounds[3] = { x: 0, y: 0, width: 0, height: 0 };
                commands = [
                    'CONFIG|segment|0|x|0',
                    'CONFIG|segment|0|y|0',
                    'CONFIG|segment|0|width|32',
                    'CONFIG|segment|0|height|32',
                    'CONFIG|segment|1|x|32',
                    'CONFIG|segment|1|y|0',
                    'CONFIG|segment|1|width|32',
                    'CONFIG|segment|1|height|32',
                    'CLEAR|2',
                    'CLEAR|3'
                ];
            } else if (layoutType === 'split-horizontal') {
                // 1 / 2: Top half (0->16px) / Bottom half (16->32px), full width
                // Only segments 1 and 2 are active
                updateSegmentStates([0, 1]);
                segmentBounds[0] = { x: 0, y: 0, width: 64, height: 16 };
                segmentBounds[1] = { x: 0, y: 16, width: 64, height: 16 };
                segmentBounds[2] = { x: 0, y: 0, width: 0, height: 0 };
                segmentBounds[3] = { x: 0, y: 0, width: 0, height: 0 };
                commands = [
                    'CONFIG|segment|0|x|0',
                    'CONFIG|segment|0|y|0',
                    'CONFIG|segment|0|width|64',
                    'CONFIG|segment|0|height|16',
                    'CONFIG|segment|1|x|0',
                    'CONFIG|segment|1|y|16',
                    'CONFIG|segment|1|width|64',
                    'CONFIG|segment|1|height|16',
                    'CLEAR|2',
                    'CLEAR|3'
                ];
            } else if (layoutType === 'quad') {
                // 2x2 Grid: 1(top-left) 2(top-right) 3(bottom-left) 4(bottom-right)
                // All 4 segments are active
                updateSegmentStates([0, 1, 2, 3]);
                segmentBounds[0] = { x: 0, y: 0, width: 32, height: 16 };
                segmentBounds[1] = { x: 32, y: 0, width: 32, height: 16 };
                segmentBounds[2] = { x: 0, y: 16, width: 32, height: 16 };
                segmentBounds[3] = { x: 32, y: 16, width: 32, height: 16 };
                commands = [
                    'CONFIG|segment|0|x|0',
                    'CONFIG|segment|0|y|0',
                    'CONFIG|segment|0|width|32',
                    'CONFIG|segment|0|height|16',
                    'CONFIG|segment|1|x|32',
                    'CONFIG|segment|1|y|0',
                    'CONFIG|segment|1|width|32',
                    'CONFIG|segment|1|height|16',
                    'CONFIG|segment|2|x|0',
                    'CONFIG|segment|2|y|16',
                    'CONFIG|segment|2|width|32',
                    'CONFIG|segment|2|height|16',
                    'CONFIG|segment|3|x|32',
                    'CONFIG|segment|3|y|16',
                    'CONFIG|segment|3|width|32',
                    'CONFIG|segment|3|height|16'
                ];
            } else if (layoutType === 'fullscreen') {
                // Fullscreen: Only segment 1 is active, displays on full screen
                updateSegmentStates([0]);
                segmentBounds[0] = { x: 0, y: 0, width: 64, height: 32 };
                segmentBounds[1] = { x: 0, y: 0, width: 0, height: 0 };
                segmentBounds[2] = { x: 0, y: 0, width: 0, height: 0 };
                segmentBounds[3] = { x: 0, y: 0, width: 0, height: 0 };
                commands = [
                    'CONFIG|segment|0|x|0',
                    'CONFIG|segment|0|y|0',
                    'CONFIG|segment|0|width|64',
                    'CONFIG|segment|0|height|32',
                    'CLEAR|1',
                    'CLEAR|2',
                    'CLEAR|3'
                ];
            }
            
            // Send all CONFIG commands
            for (let i = 0; i < commands.length; i++) {
                sendUDP(commands[i]);
            }
            
            // Re-send all active segments' content to apply them with new layout dimensions
            const segmentsToResend = layoutType === 'fullscreen' ? [0] : 
                                     (layoutType === 'split-vertical' || layoutType === 'split-horizontal') ? [0, 1] : 
                                     [0, 1, 2, 3];
            
            setTimeout(function() {
                for (let seg of segmentsToResend) {
                    const text = document.getElementById('text' + seg).value;
                    if (text) {
                        sendText(seg);
                    }
                }
            }, 200);
            
            // Clear and redraw canvas with new layout
            ctx.fillStyle = '#000000';
            ctx.fillRect(0, 0, 64, 32);
            
            // Preview all active segments with new layout
            setTimeout(function() {
                for (let seg of segmentsToResend) {
                    const text = document.getElementById('text' + seg).value;
                    if (text) {
                        previewText(seg);
                    }
                }
            }, 100);
            
            setTimeout(() => updateStatus('Layout applied: ' + layoutType, 'ready'), 500);
        }
        
        function previewText(segment) {
            const text = document.getElementById('text' + segment).value;
            const colorInput = document.getElementById('color' + segment);
            const color = colorInput ? colorInput.value : '#FFFFFF';
            const bgColorInput = document.getElementById('bgcolor' + segment);
            const bgColor = bgColorInput ? bgColorInput.value : '#000000';
            const intensityInput = document.getElementById('intensity' + segment);
            const intensity = intensityInput ? parseInt(intensityInput.value) / 255 : 1.0;
            const align = segmentAlign[segment];
            
            // Get segment bounds
            const bounds = segmentBounds[segment];
            
            // Skip if segment has no area (disabled in current layout)
            if (bounds.width === 0 || bounds.height === 0) {
                updateStatus('Segment ' + (segment + 1) + ' not active in current layout', 'ready');
                return;
            }
            
            // Fill segment area with background color
            ctx.fillStyle = bgColor;
            ctx.globalAlpha = intensity;
            ctx.fillRect(bounds.x, bounds.y, bounds.width, bounds.height);
            ctx.globalAlpha = 1.0;
            
            // Set text properties
            ctx.fillStyle = color;
            
            // Get font selection
            const fontSelect = document.getElementById('font' + segment);
            const fontValue = fontSelect ? fontSelect.value : 'arial';
            let fontFamily;
            if (fontValue === 'arial') {
                fontFamily = 'Arial, sans-serif';
            } else if (fontValue === 'verdana') {
                fontFamily = 'Verdana, sans-serif';
            } else {
                fontFamily = 'monospace';
            }
            
            // Calculate optimal font size to fit in segment
            const availWidth = bounds.width - 4; // 2px padding each side
            const availHeight = bounds.height - 4;
            
            // Try different font sizes from largest to smallest
            const fontSizes = [24, 20, 18, 16, 14, 12, 10, 9, 8, 6];
            let fontSize = 6;
            
            for (let size of fontSizes) {
                ctx.font = size + 'px ' + fontFamily;
                const metrics = ctx.measureText(text);
                const textWidth = metrics.width;
                const textHeight = size * 1.2; // Approximate height with line spacing
                
                if (textWidth <= availWidth && textHeight <= availHeight) {
                    fontSize = size;
                    break;
                }
            }
            
            // Set final font
            ctx.font = fontSize + 'px ' + fontFamily;
            
            // Measure text for proper centering
            const metrics = ctx.measureText(text);
            const textWidth = metrics.width;
            const textHeight = fontSize; // Approximate height
            
            // Set alignment and calculate X position
            let x;
            if (align === 'L') {
                ctx.textAlign = 'left';
                x = bounds.x + 2;
            } else if (align === 'R') {
                ctx.textAlign = 'right';
                x = bounds.x + bounds.width - 2;
            } else {
                ctx.textAlign = 'center';
                x = bounds.x + bounds.width / 2;
            }
            
            // Calculate Y position for vertical centering
            // Calculate Y position for vertical centering
            ctx.textBaseline = 'middle';
            const y = bounds.y + bounds.height / 2;
            
            // Draw text (simple single line for now)
            ctx.fillText(text, x, y);
            
            updateStatus('Preview updated for Segment ' + (segment + 1), 'ready');
        }
        
        function sendUDP(command) {
            updateStatus('Sending command...', 'sending');
            fetch('/api/test', {
                method: 'POST',
                headers: {'Content-Type': 'text/plain'},
                body: command
            })
            .then(response => response.text())
            .then(data => {
                updateStatus('Command sent successfully', 'ready');
            })
            .catch(error => {
                updateStatus('Error: ' + error, 'error');
            });
        }
        
        function sendText(segment) {
            const text = document.getElementById('text' + segment).value;
            const colorInput = document.getElementById('color' + segment);
            const color = colorInput ? colorInput.value.substring(1).toUpperCase() : 'FFFFFF';
            const bgColorInput = document.getElementById('bgcolor' + segment);
            const bgColor = bgColorInput ? bgColorInput.value.substring(1).toUpperCase() : '000000';
            const intensityInput = document.getElementById('intensity' + segment);
            const intensity = intensityInput ? intensityInput.value : '255';
            const fontSelect = document.getElementById('font' + segment);
            const font = fontSelect ? fontSelect.value : 'roboto12';
            const align = segmentAlign[segment];
            
            const command = 'TEXT|' + segment + '|' + text + '|' + color + '|' + font + '|auto|' + align + '|none|' + bgColor + '|' + intensity;
            sendUDP(command);
            
            // Update preview
            previewText(segment);
        }
        
        function clearSegment(segment) {
            sendUDP('CLEAR|' + segment);
            
            // Clear preview
            ctx.fillStyle = '#000000';
            ctx.fillRect(0, 0, 64, 32);
            updateStatus('Segment ' + segment + ' cleared', 'ready');
        }
        
        function clearAll() {
            sendUDP('CLEAR_ALL');
            
            // Clear preview
            ctx.fillStyle = '#000000';
            ctx.fillRect(0, 0, 64, 32);
            updateStatus('All segments cleared', 'ready');
        }
        
        function updateBrightness(value) {
            document.getElementById('brightness-value').textContent = value;
            sendUDP('BRIGHTNESS|' + value);
        }
    </script>
</body>
</html>
    )rawliteral";
    
    // Replace placeholders with actual values
    html.replace("{{IP_ADDRESS}}", ipAddress);
    html.replace("{{UDP_PORT}}", udpPort);
    html.replace("{{MATRIX_SIZE}}", matrixSize);
    
    request->send(200, "text/html", html);
}

void handleConfig(AsyncWebServerRequest *request) {
    StaticJsonDocument<512> doc;
    
    doc["udp_port"] = UDP_PORT;
    doc["brightness"] = currentBrightness;
    doc["matrix_width"] = LED_MATRIX_WIDTH;
    doc["matrix_height"] = LED_MATRIX_HEIGHT;
    
    String json;
    serializeJson(doc, json);
    
    request->send(200, "application/json", json);
}

void handleSegments(AsyncWebServerRequest *request) {
    StaticJsonDocument<1024> doc;
    JsonArray segments = doc.createNestedArray("segments");
    
    for (int i = 0; i < MAX_SEGMENTS; i++) {
        Segment* seg = segmentManager.getSegment(i);
        if (seg) {
            JsonObject segObj = segments.createNestedObject();
            segObj["id"] = seg->id;
            segObj["text"] = seg->text;
            segObj["active"] = seg->isActive;
        }
    }
    
    String json;
    serializeJson(doc, json);
    
    request->send(200, "application/json", json);
}

void handleTest(AsyncWebServerRequest *request) {
    if (request->hasParam("plain", true)) {
        String command = request->getParam("plain", true)->value();
        Serial.print("Web test command: ");
        Serial.println(command);
        
        // Actually process the command through UDP handler
        char buffer[UDP_BUFFER_SIZE];
        command.toCharArray(buffer, UDP_BUFFER_SIZE);
        
        // Parse and execute command
        if (command.startsWith("TEXT|")) {
            udpHandler->parseTextCommand(buffer);
            request->send(200, "text/plain", "Text command sent: " + command);
        } else if (command.startsWith("CLEAR|")) {
            udpHandler->parseClearCommand(buffer);
            request->send(200, "text/plain", "Clear command sent: " + command);
        } else if (command == "CLEAR_ALL") {
            segmentManager.clearAll();
            request->send(200, "text/plain", "Cleared all segments");
        } else if (command.startsWith("BRIGHTNESS|")) {
            udpHandler->parseBrightnessCommand(buffer);
            request->send(200, "text/plain", "Brightness command sent: " + command);
        } else {
            request->send(400, "text/plain", "Unknown command: " + command);
        }
    } else {
        request->send(400, "text/plain", "No command provided");
    }
}
