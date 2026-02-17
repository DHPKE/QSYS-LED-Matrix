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
#include <WiFi.h>
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

// WiFi credentials (can be configured via web interface)
const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;

// Configuration variables
uint8_t currentBrightness = DEFAULT_BRIGHTNESS;
unsigned long lastEffectUpdate = 0;
const unsigned long EFFECT_UPDATE_INTERVAL = 50; // 50ms for smooth effects

// Function declarations
void setupMatrix();
void setupWiFi();
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
    
    // Setup WiFi
    setupWiFi();
    
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

void setupWiFi() {
    Serial.print("Connecting to WiFi");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("✓ WiFi connected");
        Serial.print("  IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("  MAC Address: ");
        Serial.println(WiFi.macAddress());
    } else {
        Serial.println();
        Serial.println("WARNING: WiFi connection failed!");
        Serial.println("  Starting in AP mode...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("OlimexLED-Matrix", "12345678");
        Serial.print("  AP IP Address: ");
        Serial.println(WiFi.softAPIP());
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
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Olimex LED Matrix Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #1a1a1a;
            color: #ffffff;
        }
        h1 {
            color: #00ff00;
            text-align: center;
        }
        .section {
            background-color: #2a2a2a;
            padding: 20px;
            margin: 10px 0;
            border-radius: 5px;
        }
        input, select, button {
            padding: 10px;
            margin: 5px 0;
            width: 100%;
            box-sizing: border-box;
            background-color: #3a3a3a;
            color: #ffffff;
            border: 1px solid #555;
            border-radius: 3px;
        }
        button {
            background-color: #007bff;
            color: white;
            cursor: pointer;
            font-weight: bold;
        }
        button:hover {
            background-color: #0056b3;
        }
        .clear-btn {
            background-color: #dc3545;
        }
        .clear-btn:hover {
            background-color: #c82333;
        }
        label {
            display: block;
            margin-top: 10px;
            color: #cccccc;
        }
        .info {
            background-color: #1a3a5a;
            padding: 10px;
            border-radius: 3px;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <h1>🖥️ LED Matrix Controller</h1>
    
    <div class="info">
        <strong>UDP Port:</strong> )" + String(UDP_PORT) + "<br>
        <strong>Matrix Size:</strong> )" + String(LED_MATRIX_WIDTH) + "x" + String(LED_MATRIX_HEIGHT) + "<br>
        <strong>Status:</strong> <span id="status">Ready</span>
    </div>
    
    <div class="section">
        <h2>Segment 0 (Full Screen)</h2>
        <label>Text:</label>
        <input type="text" id="text0" placeholder="Enter text...">
        <label>Color (hex):</label>
        <input type="color" id="color0" value="#ffffff">
        <label>Font:</label>
        <select id="font0">
            <option value="roboto12">Roboto 12pt</option>
            <option value="roboto16">Roboto 16pt</option>
            <option value="roboto24">Roboto 24pt</option>
            <option value="digital12">Digital 12pt</option>
            <option value="mono9">Mono 9pt</option>
        </select>
        <button onclick="sendText(0)">Display Text</button>
        <button class="clear-btn" onclick="clearSegment(0)">Clear</button>
    </div>
    
    <div class="section">
        <h2>Segment 1 (Top Half)</h2>
        <label>Text:</label>
        <input type="text" id="text1" placeholder="Enter text...">
        <label>Color (hex):</label>
        <input type="color" id="color1" value="#00ff00">
        <button onclick="sendText(1)">Display Text</button>
        <button class="clear-btn" onclick="clearSegment(1)">Clear</button>
    </div>
    
    <div class="section">
        <h2>Quick Actions</h2>
        <button onclick="clearAll()">Clear All Segments</button>
        <label>Brightness (0-255):</label>
        <input type="range" id="brightness" min="0" max="255" value="128" 
               oninput="setBrightness(this.value)">
        <span id="brightness-value">128</span>
    </div>
    
    <script>
        function sendUDP(command) {
            fetch('/api/test', {
                method: 'POST',
                headers: {'Content-Type': 'text/plain'},
                body: command
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('status').textContent = 'Command sent: ' + command;
            })
            .catch(error => {
                document.getElementById('status').textContent = 'Error: ' + error;
            });
        }
        
        function sendText(segment) {
            const text = document.getElementById('text' + segment).value;
            const colorInput = document.getElementById('color' + segment);
            const color = colorInput ? colorInput.value.substring(1) : 'FFFFFF';
            const fontSelect = document.getElementById('font' + segment);
            const font = fontSelect ? fontSelect.value : 'roboto12';
            
            const command = `TEXT|${segment}|${text}|${color}|${font}|auto|C|none`;
            sendUDP(command);
        }
        
        function clearSegment(segment) {
            sendUDP(`CLEAR|${segment}`);
        }
        
        function clearAll() {
            sendUDP('CLEAR_ALL');
        }
        
        function setBrightness(value) {
            document.getElementById('brightness-value').textContent = value;
            sendUDP(`BRIGHTNESS|${value}`);
        }
    </script>
</body>
</html>
    )";
    
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
