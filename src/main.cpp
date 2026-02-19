/*
 * WT32-ETH01 LED Matrix Controller
 * Ethernet firmware for 64x32 HUB75 LED Matrix
 * Receives UDP JSON commands from Q-SYS plugin to display dynamic text
 *
 * Hardware:
 * - WT32-ETH01 (ESP32 + LAN8720, ETH_CLOCK_GPIO0_OUT)
 * - 64x32 HUB75 LED Matrix Panel
 *
 * Author: Generated for DHPKE/QSYS-LED-Matrix
 * Version: 2.0.0
 *
 * CHANGELOG v2.0.0:
 * - PORTED: WiFi (Olimex ESP32 Gateway) → Ethernet (WT32-ETH01)
 * - CHANGED: ETH.begin() with ETH_CLOCK_GPIO0_OUT (ESP32 drives 50 MHz to LAN8720)
 * - CHANGED: UDP protocol from pipe-delimited to JSON
 * - CHANGED: Pin assignments for WT32-ETH01 (no ETH GPIO conflicts)
 * - ADDED: Hostname wt32-led-matrix
 * - ADDED: IP address in /api/config response
 * - IMPROVED: text_renderer.h — correct x1/y1 offset handling, border support
 * - IMPROVED: segment_manager.h — clearSegment() deactivates and marks dirty
 */

#include <Arduino.h>
#include <ETH.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
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
    Serial.println("WT32-ETH01 LED Matrix Controller");
    Serial.println("Version: 2.0.0");
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
    
    // Setup Ethernet (UDP will be started from the ETH_GOT_IP event)
    setupEthernet();
    
    // Setup web server
    setupWebServer();
    
    Serial.println("\n==================================");
    Serial.println("System Ready!");
    Serial.println("==================================");
    Serial.print("IP Address: ");
    Serial.println(ETH.localIP());
    Serial.print("UDP Port: ");
    Serial.println(UDP_PORT);
    Serial.println("Web Interface: http://" + ETH.localIP().toString());
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
    
    // If Ethernet is up but UDP handler wasn't started yet, start it now
    if (eth_connected && !udpHandler) {
        setupUDP();
    }

    // Process UDP packets
    if (udpHandler) {
        udpHandler->process();
    }
    
    // Update brightness if changed
    uint8_t newBrightness = udpHandler ? udpHandler->getBrightness() : DEFAULT_BRIGHTNESS;
    if (newBrightness != currentBrightness) {
        currentBrightness = newBrightness;
        if (dma_display) dma_display->setBrightness8(currentBrightness);
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
#ifdef NO_DISPLAY
    // NO_DISPLAY mode: skip HUB75 init entirely.
    // The web UI and UDP handler still work — segments are updated in RAM
    // and the web preview reflects all changes sent from Q-SYS.
    Serial.println("⚠ NO_DISPLAY mode: HUB75 matrix init skipped (virtual preview only)");
    dma_display = nullptr;
    textRenderer = nullptr;
#else
    Serial.println("Initializing LED Matrix...");
    
    // HUB75 configuration
    HUB75_I2S_CFG mxconfig(
        LED_MATRIX_WIDTH,   // Module width
        LED_MATRIX_HEIGHT,  // Module height
        MATRIX_CHAIN        // Chain length
    );
    
    // Pin configuration for WT32-ETH01 (see src/config.h for pin assignment rationale)
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
        delete dma_display;
        dma_display = nullptr;
        textRenderer = nullptr;
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
#endif
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("ETH Started");
            ETH.setHostname("wt32-led-matrix");
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
            // Start UDP listener now that the network interface is up
            setupUDP();
            // Start mDNS responder so device is reachable as wt32-led-matrix.local
            if (MDNS.begin("wt32-led-matrix")) {
                MDNS.addService("http", "tcp", 80);
                Serial.println("✓ mDNS started: wt32-led-matrix.local");
            } else {
                Serial.println("WARNING: mDNS failed to start");
            }
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
    
    // WT32-ETH01 Ethernet configuration
    // PHY Type    : LAN8720A
    // PHY Address : 1
    // Power Pin   : -1 (no dedicated enable pin; PHY is always powered)
    // MDC Pin     : 23
    // MDIO Pin    : 18
    // Clock Mode  : ETH_CLOCK_GPIO0_OUT
    //               The ESP32 outputs a 50 MHz ref clock on GPIO0 to drive
    //               the LAN8720A. The WT32-ETH01 has no standalone oscillator.
    //               GPIO0_IN would listen for an external clock — wrong here.
    
    if (!ETH.begin(1, -1, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO0_OUT)) {
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
        
        // Store segment bounds (x, y, width, height) — updated by layout commands and /api/segments polling
        const segmentBounds = [
            { x: 0, y: 0, width: 32, height: 32 },
            { x: 32, y: 0, width: 32, height: 32 },
            { x: 0, y: 16, width: 32, height: 16 },
            { x: 32, y: 16, width: 32, height: 16 }
        ];
        
        // Initialize black canvas
        ctx.fillStyle = '#000000';
        ctx.fillRect(0, 0, 64, 32);
        
        // Poll /api/segments every second to reflect Q-SYS changes
        window.addEventListener('load', function() {
            updateSegmentStates([0, 1]);
            pollSegments();
            setInterval(pollSegments, 1000);
        });
        
        function pollSegments() {
            fetch('/api/segments')
                .then(r => r.json())
                .then(data => {
                    if (!data.segments) return;
                    // Determine which segments are active
                    const active = data.segments
                        .filter(s => s.active && s.w > 0 && s.h > 0)
                        .map(s => s.id);
                    updateSegmentStates(active);
                    
                    // Update bounds from server state
                    data.segments.forEach(s => {
                        segmentBounds[s.id] = { x: s.x, y: s.y, width: s.w, height: s.h };
                        // Update text fields with current server values
                        const textEl = document.getElementById('text' + s.id);
                        if (textEl && document.activeElement !== textEl) {
                            textEl.value = s.text || '';
                        }
                    });
                    
                    // Redraw all active segments on the canvas
                    ctx.fillStyle = '#000000';
                    ctx.fillRect(0, 0, 64, 32);
                    data.segments.forEach(s => {
                        if (s.active && s.w > 0 && s.h > 0 && s.text) {
                            drawSegmentOnCanvas(s.id, s.text,
                                s.color   || '#FFFFFF',
                                s.bgcolor || '#000000',
                                s.align   || 'C');
                        }
                    });
                })
                .catch(() => {}); // Silently ignore network errors
        }
        
        function updateStatus(message, type = 'ready') {
            const statusEl = document.getElementById('status');
            statusEl.textContent = message;
            statusEl.className = 'status ' + type;
        }
        
        function setAlign(segment, align, btn) {
            segmentAlign[segment] = align;
            const parent = btn.parentElement;
            parent.querySelectorAll('.align-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            updateStatus('Alignment set to ' + (align === 'L' ? 'Left' : align === 'C' ? 'Center' : 'Right'), 'ready');
        }
        
        function updateSegmentStates(activeSegments) {
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
        
        // ── JSON command senders ──────────────────────────────────────────────
        
        function sendJSON(obj) {
            updateStatus('Sending...', 'sending');
            fetch('/api/test', {
                method: 'POST',
                headers: { 'Content-Type': 'text/plain' },
                body: JSON.stringify(obj)
            })
            .then(r => r.text())
            .then(() => updateStatus('OK', 'ready'))
            .catch(e => updateStatus('Error: ' + e, 'error'));
        }
        
        function sendText(segment) {
            const text      = document.getElementById('text'      + segment).value;
            const color     = document.getElementById('color'     + segment).value.replace('#','');
            const bgcolor   = document.getElementById('bgcolor'   + segment).value.replace('#','');
            const intensity = parseInt(document.getElementById('intensity' + segment).value);
            const font      = document.getElementById('font'      + segment).value;
            const align     = segmentAlign[segment];
            
            sendJSON({
                cmd: 'text', seg: segment,
                text: text, color: color, bgcolor: bgcolor,
                font: font, size: 'auto', align: align,
                effect: 'none', intensity: intensity
            });
            drawSegmentOnCanvas(segment, text,
                '#' + color, '#' + bgcolor, align);
        }
        
        function clearSegment(segment) {
            sendJSON({ cmd: 'clear', seg: segment });
            ctx.fillStyle = '#000000';
            const b = segmentBounds[segment];
            ctx.fillRect(b.x, b.y, b.width, b.height);
            updateStatus('Segment ' + (segment + 1) + ' cleared', 'ready');
        }
        
        function clearAll() {
            sendJSON({ cmd: 'clear_all' });
            ctx.fillStyle = '#000000';
            ctx.fillRect(0, 0, 64, 32);
            updateStatus('All segments cleared', 'ready');
        }
        
        function updateBrightness(value) {
            document.getElementById('brightness-value').textContent = value;
            sendJSON({ cmd: 'brightness', value: parseInt(value) });
        }
        
        function applyLayout(layoutType) {
            updateStatus('Applying ' + layoutType + ' layout...', 'sending');
            
            let configCmds = [];
            let clearSegs  = [];
            
            if (layoutType === 'split-vertical') {
                updateSegmentStates([0, 1]);
                segmentBounds[0] = { x: 0,  y: 0, width: 32, height: 32 };
                segmentBounds[1] = { x: 32, y: 0, width: 32, height: 32 };
                segmentBounds[2] = segmentBounds[3] = { x: 0, y: 0, width: 0, height: 0 };
                configCmds = [
                    { cmd:'config', seg:0, x:0,  y:0, w:32, h:32 },
                    { cmd:'config', seg:1, x:32, y:0, w:32, h:32 }
                ];
                clearSegs = [2, 3];
            } else if (layoutType === 'split-horizontal') {
                updateSegmentStates([0, 1]);
                segmentBounds[0] = { x: 0, y: 0,  width: 64, height: 16 };
                segmentBounds[1] = { x: 0, y: 16, width: 64, height: 16 };
                segmentBounds[2] = segmentBounds[3] = { x: 0, y: 0, width: 0, height: 0 };
                configCmds = [
                    { cmd:'config', seg:0, x:0, y:0,  w:64, h:16 },
                    { cmd:'config', seg:1, x:0, y:16, w:64, h:16 }
                ];
                clearSegs = [2, 3];
            } else if (layoutType === 'quad') {
                updateSegmentStates([0, 1, 2, 3]);
                segmentBounds[0] = { x: 0,  y: 0,  width: 32, height: 16 };
                segmentBounds[1] = { x: 32, y: 0,  width: 32, height: 16 };
                segmentBounds[2] = { x: 0,  y: 16, width: 32, height: 16 };
                segmentBounds[3] = { x: 32, y: 16, width: 32, height: 16 };
                configCmds = [
                    { cmd:'config', seg:0, x:0,  y:0,  w:32, h:16 },
                    { cmd:'config', seg:1, x:32, y:0,  w:32, h:16 },
                    { cmd:'config', seg:2, x:0,  y:16, w:32, h:16 },
                    { cmd:'config', seg:3, x:32, y:16, w:32, h:16 }
                ];
            } else if (layoutType === 'fullscreen') {
                updateSegmentStates([0]);
                segmentBounds[0] = { x: 0, y: 0, width: 64, height: 32 };
                segmentBounds[1] = segmentBounds[2] = segmentBounds[3] = { x: 0, y: 0, width: 0, height: 0 };
                configCmds = [
                    { cmd:'config', seg:0, x:0, y:0, w:64, h:32 }
                ];
                clearSegs = [1, 2, 3];
            }
            
            // Send config commands sequentially then clear unused segments
            configCmds.forEach(c => sendJSON(c));
            clearSegs.forEach(s => sendJSON({ cmd: 'clear', seg: s }));
            
            // Redraw canvas with new bounds
            ctx.fillStyle = '#000000';
            ctx.fillRect(0, 0, 64, 32);
            for (let seg = 0; seg < 4; seg++) {
                const text = document.getElementById('text' + seg).value;
                if (text && segmentBounds[seg].width > 0) {
                    const color   = document.getElementById('color'   + seg).value;
                    const bgcolor = document.getElementById('bgcolor' + seg).value;
                    drawSegmentOnCanvas(seg, text, color, bgcolor, segmentAlign[seg]);
                }
            }
            
            setTimeout(() => updateStatus('Layout applied: ' + layoutType, 'ready'), 300);
        }
        
        // ── Canvas preview renderer ───────────────────────────────────────────
        
        function drawSegmentOnCanvas(segment, text, color, bgcolor, align) {
            const bounds = segmentBounds[segment];
            if (!bounds || bounds.width === 0 || bounds.height === 0) return;
            
            // Background fill
            ctx.fillStyle = bgcolor;
            ctx.fillRect(bounds.x, bounds.y, bounds.width, bounds.height);
            
            if (!text) return;
            
            const availW = bounds.width  - 4;
            const availH = bounds.height - 2;
            
            // Find the largest font that fits
            const sizes = [24, 20, 18, 16, 14, 12, 10, 9, 8, 6];
            let fontSize = 6;
            for (let size of sizes) {
                ctx.font = size + 'px Arial, sans-serif';
                if (ctx.measureText(text).width <= availW && size * 1.2 <= availH) {
                    fontSize = size;
                    break;
                }
            }
            ctx.font = fontSize + 'px Arial, sans-serif';
            ctx.fillStyle = color;
            ctx.textBaseline = 'middle';
            
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
            ctx.fillText(text, x, bounds.y + bounds.height / 2);
        }
        
        function previewText(segment) {
            const text    = document.getElementById('text'    + segment).value;
            const color   = document.getElementById('color'   + segment).value;
            const bgcolor = document.getElementById('bgcolor' + segment).value;
            const align   = segmentAlign[segment];
            drawSegmentOnCanvas(segment, text, color, bgcolor, align);
            updateStatus('Preview updated for Segment ' + (segment + 1), 'ready');
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
    
    doc["ip_address"] = ETH.localIP().toString();
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
            segObj["id"]     = seg->id;
            segObj["text"]   = seg->text;
            segObj["active"] = seg->isActive;
            segObj["x"]      = seg->x;
            segObj["y"]      = seg->y;
            segObj["w"]      = seg->width;
            segObj["h"]      = seg->height;
            segObj["align"]  = (seg->align == ALIGN_LEFT) ? "L" :
                               (seg->align == ALIGN_RIGHT) ? "R" : "C";
            segObj["effect"] = (seg->effect == EFFECT_SCROLL)  ? "scroll" :
                               (seg->effect == EFFECT_BLINK)   ? "blink"  :
                               (seg->effect == EFFECT_FADE)    ? "fade"   :
                               (seg->effect == EFFECT_RAINBOW) ? "rainbow": "none";

            // Convert RGB565 colors back to hex strings for the web UI
            uint16_t c  = seg->color;
            uint16_t bg = seg->bgColor;
            char colorHex[8], bgHex[8];
            // RGB565 → RGB888 approximate
            snprintf(colorHex, sizeof(colorHex), "#%02X%02X%02X",
                     ((c >> 8) & 0xF8),
                     ((c >> 3) & 0xFC),
                     ((c << 3) & 0xF8));
            snprintf(bgHex, sizeof(bgHex), "#%02X%02X%02X",
                     ((bg >> 8) & 0xF8),
                     ((bg >> 3) & 0xFC),
                     ((bg << 3) & 0xF8));
            segObj["color"]   = colorHex;
            segObj["bgcolor"] = bgHex;
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

        // Pass JSON directly to dispatchCommand
        char buffer[UDP_BUFFER_SIZE];
        command.toCharArray(buffer, UDP_BUFFER_SIZE);
        udpHandler->dispatchCommand(buffer);
        request->send(200, "text/plain", "Command dispatched: " + command);
    } else {
        request->send(400, "text/plain", "No command provided");
    }
}
