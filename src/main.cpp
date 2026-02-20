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
 * - PORTED: WiFi (Olimex ESP32 Gateway) â†’ Ethernet (WT32-ETH01)
 * - CHANGED: ETH.begin() with ETH_CLOCK_GPIO0_OUT (ESP32 drives 50 MHz to LAN8720)
 * - CHANGED: UDP protocol from pipe-delimited to JSON
 * - CHANGED: Pin assignments for WT32-ETH01 (no ETH GPIO conflicts)
 * - ADDED: Hostname wt32-led-matrix
 * - ADDED: IP address in /api/config response
 * - IMPROVED: text_renderer.h â€” correct x1/y1 offset handling, border support
 * - IMPROVED: segment_manager.h â€” clearSegment() deactivates and marks dirty
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

// Watchdog timeout (30 seconds â€” must be longer than Ethernet init wait)
#define WDT_TIMEOUT 30

// Global objects
MatrixPanel_I2S_DMA *dma_display = nullptr;
SegmentManager segmentManager;
TextRenderer *textRenderer = nullptr;
UDPHandler *udpHandler = nullptr;
AsyncWebServer server(WEB_SERVER_PORT);

// Ethernet connection flag
static bool eth_connected = false;

// True while the IP-address splash is being shown on the matrix.
// Cleared the first time a UDP command arrives.
static bool ipSplashActive = false;

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
void showIPOnDisplay(const String& ip);
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
    Serial.println("âœ“ Watchdog enabled (30s timeout)");
    
    // Initialize LittleFS (true = format partition if mount fails)
    if (!LittleFS.begin(true)) {
        Serial.println("WARNING: LittleFS mount failed â€” config save/load disabled");
        Serial.println("  Check: board_build.partitions = no_ota.csv in platformio.ini");
    } else {
        Serial.println("âœ“ LittleFS mounted successfully");
    }
    
    // Load configuration
    loadConfiguration();
    
    // Setup Ethernet FIRST â€” get network up before anything else
    // (UDP is started from the ETH_GOT_IP event)
    setupEthernet();
    
    // Setup web server
    setupWebServer();
    
    // Setup LED Matrix AFTER network is up
    // (matrix DMA init is deferred so a missing/unconnected panel can't block boot)
    setupMatrix();
    
    Serial.println("\n==================================");
    Serial.println("System Ready!");
    Serial.println("==================================");
    Serial.print("IP Address: ");
    Serial.println(ETH.localIP());
    Serial.print("UDP Port: ");
    Serial.println(UDP_PORT);
    Serial.println("Web Interface: http://" + ETH.localIP().toString());
    Serial.println("==================================\n");

    // Show IP address on matrix â€” cleared on first incoming UDP command.
    // If Ethernet hasn't connected yet the splash will be shown from the
    // ETH_GOT_IP event or the fallback-IP assignment in setupEthernet().
    String ip = ETH.localIP().toString();
    if (ip != "0.0.0.0") {
        showIPOnDisplay(ip);
    }
    // (If still "0.0.0.0" here, WiFiEvent / setupEthernet will call showIPOnDisplay)
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

    // Clear IP splash on first received command
    if (ipSplashActive && udpHandler && udpHandler->hasReceivedCommand()) {
        ipSplashActive = false;
        segmentManager.clearAll();          // wipe the IP text
        Serial.println("[SPLASH] First command received â€” IP splash cleared");
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
    // The web UI and UDP handler still work â€” segments are updated in RAM
    // and the web preview reflects all changes sent from Q-SYS.
    Serial.println("âš  NO_DISPLAY mode: HUB75 matrix init skipped (virtual preview only)");
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
    
    // Initialize display â€” if no panel is connected this returns false cleanly
    Serial.println("  Allocating DMA buffers...");
    if (!dma_display->begin()) {
        Serial.println("WARNING: Matrix init failed (no panel connected?)");
        Serial.println("  Firmware continues â€” web UI and UDP still work");
        delete dma_display;
        dma_display = nullptr;
        textRenderer = nullptr;
        return;
    }
    
    Serial.println("âœ“ LED Matrix initialized");
    
    // Set brightness
    dma_display->setBrightness8(currentBrightness);
    
    // Clear display
    dma_display->clearScreen();
    
    // Create text renderer
    textRenderer = new TextRenderer(dma_display, &segmentManager);
    
    Serial.print("âœ“ Matrix size: ");
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
            Serial.println("âœ“ Ethernet connected");
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
                Serial.println("âœ“ mDNS started: wt32-led-matrix.local");
            } else {
                Serial.println("WARNING: mDNS failed to start");
            }
            // Show IP on the matrix (DHCP address)
            showIPOnDisplay(ETH.localIP().toString());
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

    // Register event handler BEFORE ETH.begin() so we never miss ETH_START.
    WiFi.onEvent(WiFiEvent);

    // Give GPIO0 REF_CLK output time to stabilise before the LAN8720 samples it.
    // The LAN8720 latches its strap pins (PHYAD, CLK_IN/OUT mode) on rising nRST.
    // Without this delay, the clock may not be clean yet → PHY stays in reset.
    delay(250);

    // WT32-ETH01 explicit parameters — never rely on board-package defaults:
    //   phy_addr  : 1                   (PHYAD0 strapped HIGH on WT32-ETH01)
    //   power pin : -1                  (no GPIO-controlled power rail)
    //   MDC  pin  : 23
    //   MDIO pin  : 18
    //   PHY type  : ETH_PHY_LAN8720
    //   Clock mode: ETH_CLOCK_GPIO0_OUT (ESP32 APLL → GPIO0 → LAN8720 REF_CLK)
    //
    // Signature: begin(phy_addr, power, mdc, mdio, type, clk_mode)
    if (!ETH.begin(1, -1, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO0_OUT)) {
        Serial.println("ERROR: Ethernet initialization failed!");
        return;
    }
    
    Serial.println("Waiting for Ethernet connection...");
    
    // Wait up to 15 seconds for connection; feed watchdog each iteration
    int attempts = 0;
    while (!eth_connected && attempts < 30) {
        esp_task_wdt_reset();
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (!eth_connected) {
        Serial.println("WARNING: No DHCP lease after 15 s â€” applying fallback static IP");
        Serial.println("  Fallback: " FALLBACK_IP "/24  GW: " FALLBACK_GW);

        IPAddress fbIP, fbGW, fbSN;
        fbIP.fromString(FALLBACK_IP);
        fbGW.fromString(FALLBACK_GW);
        fbSN.fromString(FALLBACK_SUBNET);
        ETH.config(fbIP, fbGW, fbSN);

        // Give the stack a moment to apply the address
        delay(200);

        eth_connected = true;
        Serial.print("âœ“ Fallback IP active: ");
        Serial.println(ETH.localIP());

        // Show fallback IP on matrix
        showIPOnDisplay(ETH.localIP().toString());
    }
}

void setupUDP() {
    Serial.println("Starting UDP listener...");
    
    udpHandler = new UDPHandler(&segmentManager);
    
    if (udpHandler->begin()) {
        Serial.print("âœ“ UDP listening on port ");
        Serial.println(UDP_PORT);
    } else {
        Serial.println("ERROR: UDP initialization failed!");
    }
}

void setupWebServer() {
    Serial.println("Starting web server...");
    
    // API endpoints
    server.on("/api/config", HTTP_GET, handleConfig);
    server.on("/api/segments", HTTP_GET, handleSegments);
    server.on("/api/test", HTTP_POST, handleTest);
    
    // Root handler â€” serves embedded HTML (no LittleFS needed)
    server.on("/", HTTP_GET, handleRoot);
    
    // 404 handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
    
    server.begin();
    Serial.println("âœ“ Web server started on port 80");
}

// Show the device IP address as a fullscreen splash on segment 0.
// Remains visible until the first UDP command is received.
void showIPOnDisplay(const String& ip) {
    Serial.println("[SPLASH] Showing IP: " + ip);
    ipSplashActive = true;

    // Reset to fullscreen layout (preset 1 = seg0 covers whole panel)
    if (udpHandler) {
        udpHandler->applyLayoutPreset(1);
    } else {
        // UDP handler not ready yet â€” configure seg0 directly
        Segment* s = segmentManager.getSegment(0);
        if (s) {
            s->x = 0; s->y = 0;
            s->width = LED_MATRIX_WIDTH; s->height = LED_MATRIX_HEIGHT;
            s->isActive = true;
        }
    }

    Segment* seg = segmentManager.getSegment(0);
    if (!seg) return;

    strncpy(seg->text, ip.c_str(), MAX_TEXT_LENGTH - 1);
    seg->text[MAX_TEXT_LENGTH - 1] = '\0';
    seg->color    = 0xFFFF;   // white text
    seg->bgColor  = 0x0000;   // black background
    strncpy(seg->fontName, "arial", 15);
    seg->autoSize = true;
    seg->align    = ALIGN_CENTER;
    seg->effect   = EFFECT_NONE;
    seg->isActive = true;
    seg->isDirty  = true;
}

void loadConfiguration() {
    
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
    
    Serial.println("âœ“ Configuration loaded");
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
    
    Serial.println("âœ“ Configuration saved");
}


// â”€â”€ WebUI HTML stored in flash (PROGMEM) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// The entire page is served from flash. Dynamic values (IP, port, matrix size)
// are injected via a chunked-response callback that resolves %%PLACEHOLDER%%
// tokens without ever copying the 8+ KB body into heap RAM.
// Colour <option> lists are built once at startup into small static Strings.
static const char WEBPAGE_HTML[] PROGMEM = R"HTMLEOF(<!DOCTYPE html>
<html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>LED Matrix</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#0f0f1e,#1a1a2e);color:#e0e0e0;min-height:100vh;padding:20px}
.wrap{max-width:1200px;margin:0 auto}
.hdr{text-align:center;margin-bottom:24px;padding:24px 16px;background:linear-gradient(135deg,#1e3c72,#2a5298);border-radius:12px}
.hdr h1{font-size:2em;color:#fff;margin-bottom:6px}
.hdr p{color:#a0c4ff;font-size:1em}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-bottom:16px}
.segs{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px}
@media(max-width:768px){.grid,.segs{grid-template-columns:1fr}}
.card{background:rgba(30,30,46,.9);border-radius:10px;padding:16px;border:1px solid rgba(255,255,255,.1)}
.card.cp{padding:12px;transition:opacity .2s,filter .2s;position:relative}
.card.cp.off{opacity:.4;pointer-events:none;filter:grayscale(.7)}
.card.cp.off::after{content:'INACTIVE';position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);background:rgba(0,0,0,.85);color:#666;padding:6px 16px;border-radius:5px;font-weight:700;font-size:.85em;letter-spacing:2px;z-index:10}
.card h2{font-size:1.05em;margin-bottom:12px;color:#4a9eff;border-bottom:2px solid #4a9eff;padding-bottom:6px}
.ii{display:flex;flex-direction:column;gap:4px}
.il{color:#888;font-weight:600;font-size:.8em;text-transform:uppercase;letter-spacing:.5px}
.iv input{background:rgba(0,0,0,.3);border:1px solid rgba(255,255,255,.1);border-radius:4px;color:#e0e0e0;padding:5px 8px;font-size:.85em;font-family:monospace;min-width:120px}
.st{padding:6px 14px;border-radius:18px;font-size:.85em;font-weight:600;display:inline-block}
.st.ok{background:#1a472a;color:#4ade80}.st.snd{background:#1e3a8a;color:#60a5fa}.st.err{background:#7f1d1d;color:#f87171}
.prev{grid-column:1/-1;text-align:center}
#preview{border:2px solid #333;background:#000;border-radius:6px;image-rendering:pixelated;image-rendering:crisp-edges;width:100%;max-width:576px;height:auto}
.fg{margin-bottom:10px}
label{display:block;margin-bottom:4px;color:#a0a0a0;font-weight:600;font-size:.8em;text-transform:uppercase;letter-spacing:.5px}
input[type=text],input[type=number],select{width:100%;padding:8px 10px;background:rgba(0,0,0,.4);border:1px solid rgba(255,255,255,.1);border-radius:6px;color:#e0e0e0;font-size:.9em}
input[type=text]:focus,input[type=number]:focus,select:focus{outline:none;border-color:#4a9eff}
input[type=range]{width:100%;height:6px;background:rgba(255,255,255,.1);border-radius:3px;outline:none;-webkit-appearance:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;background:#4a9eff;border-radius:50%;cursor:pointer}
input[type=range]::-moz-range-thumb{width:18px;height:18px;background:#4a9eff;border-radius:50%;cursor:pointer;border:none}
.bv{display:flex;justify-content:space-between;align-items:center;margin-top:4px}
.bval{background:rgba(74,158,255,.2);color:#4a9eff;padding:4px 12px;border-radius:16px;font-weight:700}
.bg{display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;margin-top:10px}
button{padding:8px 12px;border:none;border-radius:6px;font-size:.85em;font-weight:600;cursor:pointer;text-transform:uppercase;letter-spacing:.5px}
.bp{background:#2563eb;color:#fff}.bp:hover{background:#1d4ed8}
.bd{background:#ef4444;color:#fff}.bd:hover{background:#dc2626}
.bca{grid-column:1/-1;background:#dc2626;color:#fff;width:100%;margin-top:12px}.bca:hover{background:#b91c1c}
.ag{display:flex;gap:6px;margin-top:4px}
.ab{flex:1;padding:5px;background:rgba(255,255,255,.1);border:2px solid rgba(255,255,255,.2);border-radius:5px;color:#888;cursor:pointer;font-size:.8em}
.ab.on{background:#2563eb;border-color:#2563eb;color:#fff}.ab:hover{border-color:#2563eb}
.lbs{display:grid;grid-template-columns:repeat(6,1fr);gap:8px;margin-bottom:8px}
.lb{padding:0;height:72px;background:rgba(255,255,255,.05);border:2px solid rgba(255,255,255,.1);border-radius:7px;cursor:pointer;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:6px}
.lb:hover{background:rgba(37,99,235,.2);border-color:#2563eb}.lb.on{background:rgba(37,99,235,.35);border-color:#2563eb}
.lv{display:grid;width:44px;height:28px;gap:2px;background:#000;border-radius:3px;padding:2px}
.lv.fu{padding:0}
.ls{background:#2563eb;border-radius:2px;display:flex;align-items:center;justify-content:center;font-size:8px;font-weight:700;color:#fff}
.ll{font-size:.7em;color:#888}
.nf{display:flex;gap:16px;flex-wrap:wrap;align-items:flex-start}
.nt{display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:12px}
</style></head><body>
<div class="wrap">
<div class="hdr"><h1>LED Matrix Controller</h1><p>%%SIZE%% RGB Display</p></div>
<div class="grid">
<div class="card" style="grid-column:1/-1">
<div class="nt">
<div>
<h2 style="margin:0 0 12px">Network</h2>
<div class="nf">
<div class="ii"><span class="il">IP Address</span><div class="iv"><input type="text" id="ip-address" value="%%IP%%" placeholder="192.168.1.100"></div></div>
<div class="ii"><span class="il">UDP Port</span><div class="iv"><input type="number" id="udp-port" value="%%PORT%%" placeholder="21324" min="1" max="65535" style="width:90px"></div></div>
<div class="ii"><span class="il">Display</span><span style="padding:5px 8px;background:rgba(0,0,0,.3);border-radius:4px;border:1px solid rgba(255,255,255,.1);font-size:.85em">%%SIZE%%</span></div>
</div>
</div>
<span id="status" class="st ok">Ready</span>
</div></div>
<div class="card prev"><h2>Live Preview</h2><canvas id="preview" width="576" height="288"></canvas></div>
<div class="card" style="grid-column:1/-1">
<h2>Layouts</h2>
<div class="lbs">
<button class="lb" data-p="1" onclick="applyLayout(1)"><div class="lv fu"><div class="ls" style="width:100%;height:100%;border-radius:3px">1</div></div><span class="ll">Full</span></button>
<button class="lb" data-p="2" onclick="applyLayout(2)"><div class="lv" style="grid-template-rows:1fr 1fr"><div class="ls">1</div><div class="ls">2</div></div><span class="ll">1/2</span></button>
<button class="lb" data-p="3" onclick="applyLayout(3)"><div class="lv" style="grid-template-columns:1fr 1fr"><div class="ls">1</div><div class="ls">2</div></div><span class="ll">1|2</span></button>
<button class="lb" data-p="4" onclick="applyLayout(4)"><div class="lv" style="grid-template-columns:1fr 1fr;grid-template-rows:1fr 1fr"><div class="ls">1</div><div class="ls">2</div><div class="ls">3</div><div class="ls">4</div></div><span class="ll">2x2</span></button>
<button class="lb" data-p="5" onclick="applyLayout(5)"><div class="lv" style="grid-template-columns:1fr 1fr 1fr"><div class="ls">1</div><div class="ls">2</div><div class="ls">3</div></div><span class="ll">1|2|3</span></button>
<button class="lb" data-p="6" onclick="applyLayout(6)"><div class="lv" style="grid-template-columns:1fr 1fr;grid-template-rows:1fr 1fr"><div class="ls" style="grid-row:span 2">1</div><div class="ls">2</div><div class="ls">3</div></div><span class="ll">1/2|3</span></button>
</div></div>
</div>
<div class="segs">
<div class="card cp" id="sc0"><h2>Segment 1</h2><div class="fg"><label>Text</label><input type="text" id="text0" placeholder="Message..."></div><div class="fg"><label>Color</label><select id="color0">%%COPTS_W%%</select></div><div class="fg"><label>Background</label><select id="bgcolor0">%%COPTS_B%%</select></div><div class="fg"><label>Intensity</label><input type="range" id="int0" min="0" max="255" value="255" oninput="document.getElementById('iv0').textContent=this.value"><span id="iv0" style="color:#888;font-size:.8em">255</span></div><div class="fg"><label>Font</label><select id="font0">%%FOPTS%%</select></div><div class="fg"><label>Align</label><div class="ag"><button class="ab" onclick="sa(0,'L',this)">L</button><button class="ab on" onclick="sa(0,'C',this)">C</button><button class="ab" onclick="sa(0,'R',this)">R</button></div></div><div class="bg"><button class="bp" onclick="sendText(0)">Send</button><button class="bp" onclick="previewText(0)">Preview</button><button class="bd" onclick="clearSeg(0)">Clear</button></div></div>
<div class="card cp off" id="sc1"><h2>Segment 2</h2><div class="fg"><label>Text</label><input type="text" id="text1" placeholder="Message..."></div><div class="fg"><label>Color</label><select id="color1">%%COPTS_G%%</select></div><div class="fg"><label>Background</label><select id="bgcolor1">%%COPTS_B%%</select></div><div class="fg"><label>Intensity</label><input type="range" id="int1" min="0" max="255" value="255" oninput="document.getElementById('iv1').textContent=this.value"><span id="iv1" style="color:#888;font-size:.8em">255</span></div><div class="fg"><label>Font</label><select id="font1">%%FOPTS%%</select></div><div class="fg"><label>Align</label><div class="ag"><button class="ab" onclick="sa(1,'L',this)">L</button><button class="ab on" onclick="sa(1,'C',this)">C</button><button class="ab" onclick="sa(1,'R',this)">R</button></div></div><div class="bg"><button class="bp" onclick="sendText(1)">Send</button><button class="bp" onclick="previewText(1)">Preview</button><button class="bd" onclick="clearSeg(1)">Clear</button></div></div>
<div class="card cp off" id="sc2"><h2>Segment 3</h2><div class="fg"><label>Text</label><input type="text" id="text2" placeholder="Message..."></div><div class="fg"><label>Color</label><select id="color2">%%COPTS_R%%</select></div><div class="fg"><label>Background</label><select id="bgcolor2">%%COPTS_B%%</select></div><div class="fg"><label>Intensity</label><input type="range" id="int2" min="0" max="255" value="255" oninput="document.getElementById('iv2').textContent=this.value"><span id="iv2" style="color:#888;font-size:.8em">255</span></div><div class="fg"><label>Font</label><select id="font2">%%FOPTS%%</select></div><div class="fg"><label>Align</label><div class="ag"><button class="ab" onclick="sa(2,'L',this)">L</button><button class="ab on" onclick="sa(2,'C',this)">C</button><button class="ab" onclick="sa(2,'R',this)">R</button></div></div><div class="bg"><button class="bp" onclick="sendText(2)">Send</button><button class="bp" onclick="previewText(2)">Preview</button><button class="bd" onclick="clearSeg(2)">Clear</button></div></div>
<div class="card cp off" id="sc3"><h2>Segment 4</h2><div class="fg"><label>Text</label><input type="text" id="text3" placeholder="Message..."></div><div class="fg"><label>Color</label><select id="color3">%%COPTS_Y%%</select></div><div class="fg"><label>Background</label><select id="bgcolor3">%%COPTS_B%%</select></div><div class="fg"><label>Intensity</label><input type="range" id="int3" min="0" max="255" value="255" oninput="document.getElementById('iv3').textContent=this.value"><span id="iv3" style="color:#888;font-size:.8em">255</span></div><div class="fg"><label>Font</label><select id="font3">%%FOPTS%%</select></div><div class="fg"><label>Align</label><div class="ag"><button class="ab" onclick="sa(3,'L',this)">L</button><button class="ab on" onclick="sa(3,'C',this)">C</button><button class="ab" onclick="sa(3,'R',this)">R</button></div></div><div class="bg"><button class="bp" onclick="sendText(3)">Send</button><button class="bp" onclick="previewText(3)">Preview</button><button class="bd" onclick="clearSeg(3)">Clear</button></div></div>
</div>
<div class="card" style="margin-bottom:16px">
<h2>Display Settings</h2>
<div class="fg"><label>Brightness</label><input type="range" id="brightness" min="0" max="255" value="128" oninput="updBri(this.value)"><div class="bv"><span style="color:#888">Dim</span><span class="bval" id="bv">128</span><span style="color:#888">Bright</span></div></div>
<button class="bca" onclick="clearAll()">Clear All Segments</button>
</div>
</div>
<script>
'use strict';
const canvas=document.getElementById('preview'),ctx=canvas.getContext('2d');
const sa_=[...Array(4)].map(()=>'C');
const sb=[{x:0,y:0,w:64,h:32},{x:0,y:0,w:0,h:0},{x:0,y:0,w:0,h:0},{x:0,y:0,w:0,h:0}];
let lockUntil=0,pFail=0,pTimer=null;
const MAX_FAIL=5;
const offsc=document.createElement('canvas');offsc.width=64;offsc.height=32;
const off=offsc.getContext('2d');
const LS=9,LCOLS=64,LROWS=32,LDOT=8,LR=2;
function drawBg(){ctx.fillStyle='#111';ctx.fillRect(0,0,576,288);ctx.fillStyle='#1a1a1a';for(let r=0;r<LROWS;r++)for(let c=0;c<LCOLS;c++){ctx.beginPath();ctx.roundRect(c*LS,r*LS,LDOT,LDOT,LR);ctx.fill();}}
function blit(){const d=off.getImageData(0,0,LCOLS,LROWS).data;for(let r=0;r<LROWS;r++)for(let c=0;c<LCOLS;c++){const i=(r*LCOLS+c)*4,R=d[i],G=d[i+1],B=d[i+2];if(R<9&&G<9&&B<9)continue;ctx.fillStyle='rgb('+R+','+G+','+B+')';ctx.beginPath();ctx.roundRect(c*LS,r*LS,LDOT,LDOT,LR);ctx.fill();}}
function redraw(){drawBg();blit();ctx.strokeStyle='rgba(255,255,255,.12)';ctx.lineWidth=1;for(let i=0;i<4;i++){const b=sb[i];if(b&&b.w>0)ctx.strokeRect(b.x*LS-.5,b.y*LS-.5,b.w*LS,b.h*LS);}}
function drawSeg(i,txt,fg,bg,al){const b=sb[i];if(!b||!b.w)return;off.fillStyle=bg||'#000';off.fillRect(b.x,b.y,b.w,b.h);if(txt){const fv=parseInt((document.getElementById('font'+i)||{value:'1'}).value);const ff=[,'bold Arial','Verdana','Impact'][fv]||'bold Arial';const aw=b.w-2,ah=b.h-2;let fs=6;for(const sz of[24,20,18,16,14,12,10,9,8,6]){off.font=sz+'px '+ff;const m=off.measureText(txt);if(m.width<=aw&&(m.actualBoundingBoxAscent||sz)+(m.actualBoundingBoxDescent||sz*.2)<=ah){fs=sz;break;}}off.font=fs+'px '+ff;off.fillStyle=fg||'#fff';off.textBaseline='middle';const tw=off.measureText(txt).width;const tx=al==='L'?b.x+1:al==='R'?b.x+b.w-tw-1:b.x+(b.w-tw)/2;off.fillText(txt,tx,b.y+b.h/2);}redraw();}
function updSt(active){for(let i=0;i<4;i++){const c=document.getElementById('sc'+i);if(c)c.classList.toggle('off',!active.includes(i));}}
function updLb(p){document.querySelectorAll('.lb').forEach(b=>b.classList.toggle('on',+b.dataset.p===p));}
function setSt(msg,t){const e=document.getElementById('status');e.textContent=msg;e.className='st '+(t||'ok');}
function sa(i,a,btn){sa_[i]=a;btn.parentElement.querySelectorAll('.ab').forEach(b=>b.classList.remove('on'));btn.classList.add('on');}
function sched(d){clearTimeout(pTimer);pTimer=setTimeout(poll,d);}
function poll(){const ac=new AbortController();const tid=setTimeout(()=>ac.abort(),3000);fetch('/api/segments',{signal:ac.signal}).then(r=>{clearTimeout(tid);return r.json();}).then(data=>{if(!data||!data.segments)return;pFail=0;if(Date.now()<lockUntil){sched(2000);return;}const act=data.segments.filter(s=>s.active&&s.w>0&&s.h>0).map(s=>s.id);updSt(act);data.segments.forEach(s=>{sb[s.id]={x:s.x,y:s.y,w:s.w,h:s.h};});off.fillStyle='#000';off.fillRect(0,0,LCOLS,LROWS);data.segments.forEach(s=>{if(s.active&&s.w>0)drawSeg(s.id,s.text||'',s.color||'#fff',s.bgcolor||'#000',s.align||'C');});redraw();sched(2000);}).catch(()=>{clearTimeout(tid);pFail++;const bk=Math.min(2000*Math.pow(2,pFail),30000);if(pFail===1)setSt('Lost connection...','err');if(pFail>=MAX_FAIL)setSt('Device unreachable','err');sched(bk);});}
function sj(obj,r){r=r===undefined?2:r;return fetch('/api/test',{method:'POST',headers:{'Content-Type':'text/plain'},body:JSON.stringify(obj)}).then(res=>{if(!res.ok)throw new Error('HTTP '+res.status);return res.text();}).catch(e=>{if(r>0)return new Promise(ok=>setTimeout(ok,300)).then(()=>sj(obj,r-1));setSt('Error: '+e.message,'err');throw e;});}
const LAYOUTS={1:[[0,0,64,32]],2:[[0,0,64,16],[0,16,64,16]],3:[[0,0,32,32],[32,0,32,32]],4:[[0,0,32,16],[32,0,32,16],[0,16,32,16],[32,16,32,16]],5:[[0,0,21,32],[21,0,21,32],[42,0,22,32]],6:[[0,0,32,32],[32,0,32,16],[32,16,32,16]]};
const LA={1:[0],2:[0,1],3:[0,1],4:[0,1,2,3],5:[0,1,2],6:[0,1,2]};
function applyLayout(p){setSt('Applying...','snd');lockUntil=Date.now()+8000;updLb(p);const g=LAYOUTS[p]||[];for(let i=0;i<4;i++)sb[i]=g[i]?{x:g[i][0],y:g[i][1],w:g[i][2],h:g[i][3]}:{x:0,y:0,w:0,h:0};updSt(LA[p]||[]);off.fillStyle='#000';off.fillRect(0,0,LCOLS,LROWS);for(let i=0;i<4;i++)if(sb[i].w>0){const t=document.getElementById('text'+i);drawSeg(i,t?t.value:'',document.getElementById('color'+i).value,document.getElementById('bgcolor'+i).value,sa_[i]);}redraw();sj({cmd:'layout',preset:p}).then(()=>{setSt('Layout '+p+' active');sched(500);});}
function sendText(i){const t=document.getElementById('text'+i).value,fg=document.getElementById('color'+i).value.replace('#',''),bg=document.getElementById('bgcolor'+i).value.replace('#',''),iv=parseInt(document.getElementById('int'+i).value)||255,f=document.getElementById('font'+i).value,a=sa_[i];setSt('Sending...','snd');sj({cmd:'text',seg:i,text:t,color:fg,bgcolor:bg,font:f,size:'auto',align:a,effect:'none',intensity:iv}).then(()=>{drawSeg(i,t,'#'+fg,'#'+bg,a);setSt('Seg '+(i+1)+' updated');});}
function clearSeg(i){sj({cmd:'clear',seg:i}).then(()=>{const b=sb[i];off.fillStyle='#000';off.fillRect(b.x,b.y,b.w,b.h);redraw();setSt('Seg '+(i+1)+' cleared');});}
function clearAll(){sj({cmd:'clear_all'}).then(()=>{off.fillStyle='#000';off.fillRect(0,0,LCOLS,LROWS);redraw();setSt('All cleared');});}
function updBri(v){document.getElementById('bv').textContent=v;sj({cmd:'brightness',value:+v});}
function previewText(i){drawSeg(i,document.getElementById('text'+i).value,document.getElementById('color'+i).value,document.getElementById('bgcolor'+i).value,sa_[i]);setSt('Preview seg '+(i+1));}
off.fillStyle='#000';off.fillRect(0,0,LCOLS,LROWS);drawBg();
window.addEventListener('load',()=>{updSt([0]);updLb(1);sched(500);});
</script></body></html>)HTMLEOF";

// Colour <option> lists and font options â€” built once at startup, reused on
// every request.  Stored as static Strings (permanent small heap allocation,
// ~2 KB total) so we never rebuild them per-request.
static String _coW, _coG, _coR, _coY, _coB, _fopts;
static bool   _uiSnippetsReady = false;

static void buildUISnippets() {
    if (_uiSnippetsReady) return;
    struct ColEntry { const char* name; const char* hex; };
    static const ColEntry cols[] = {
        {"White",  "#FFFFFF"}, {"Red",    "#FF0000"}, {"Lime",   "#00FF00"},
        {"Blue",   "#0000FF"}, {"Yellow", "#FFFF00"}, {"Magenta","#FF00FF"},
        {"Cyan",   "#00FFFF"}, {"Orange", "#FFA500"}, {"Purple", "#800080"},
        {"Green",  "#008000"}, {"Pink",   "#FFC0CB"}, {"Gold",   "#FFD700"},
        {"Silver", "#C0C0C0"}, {"Gray",   "#808080"}, {"Black",  "#000000"}
    };
    const int N = 15;
    auto makeOpts = [&](int defIdx) -> String {
        String s; s.reserve(500);
        for (int i = 0; i < N; i++) {
            s += F("<option value=\""); s += cols[i].hex; s += '"';
            if (i == defIdx) s += F(" selected");
            s += '>'; s += cols[i].name; s += F("</option>");
        }
        return s;
    };
    _coW = makeOpts(0);   // White  (seg 1 default text colour)
    _coG = makeOpts(2);   // Lime   (seg 2)
    _coR = makeOpts(1);   // Red    (seg 3)
    _coY = makeOpts(4);   // Yellow (seg 4)
    _coB = makeOpts(14);  // Black  (all backgrounds)
    _fopts = F("<option value=\"1\" selected>Arial (Bold)</option>"
               "<option value=\"2\">Verdana</option>"
               "<option value=\"3\">Impact</option>");
    _uiSnippetsReady = true;
}

// handleRoot â€” streams the PROGMEM page via a chunked response.
// Peak RAM use â‰ˆ 1 TCP MSS (1460 bytes) stack buffer inside ESPAsyncWebServer.
// Zero heap allocation for the HTML body.
void handleRoot(AsyncWebServerRequest *request) {
    buildUISnippets();

    // Small per-request state â€” lives only for the duration of the stream.
    struct CState {
        const char* src;       // current read position inside WEBPAGE_HTML (flash)
        size_t      rem;       // bytes remaining
        char        ip[20];
        char        port[8];
        char        size[10];
    };
    auto* st = new CState();
    st->src = WEBPAGE_HTML;
    st->rem = strlen_P(WEBPAGE_HTML);
    snprintf(st->ip,   sizeof(st->ip),   "%s", ETH.localIP().toString().c_str());
    snprintf(st->port, sizeof(st->port),  "%u", (unsigned)UDP_PORT);
    snprintf(st->size, sizeof(st->size),  "%dx%d", LED_MATRIX_WIDTH, LED_MATRIX_HEIGHT);

    AsyncWebServerResponse* resp = request->beginChunkedResponse(
        "text/html",
        [st](uint8_t* buf, size_t maxLen, size_t /*idx*/) -> size_t {

            size_t out = 0;

            while (out < maxLen && st->rem > 0) {
                char c = (char)pgm_read_byte(st->src);

                // â”€â”€ Placeholder detection: %%NAME%% â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
                if (c == '%' && st->rem >= 4 &&
                    (char)pgm_read_byte(st->src + 1) == '%') {

                    // Scan for closing %%
                    const char* p   = st->src + 2;
                    size_t      rem2 = st->rem - 2;
                    while (rem2 > 1 &&
                           !((char)pgm_read_byte(p)   == '%' &&
                             (char)pgm_read_byte(p+1) == '%')) {
                        ++p; --rem2;
                    }

                    if (rem2 < 2) {
                        // No closing %% â€” emit the '%' literally and move on
                        if (out + 1 > maxLen) break;
                        buf[out++] = '%';
                        ++st->src; --st->rem;
                        continue;
                    }

                    // Extract name into a small stack buffer
                    char name[12] = {};
                    size_t nlen = (size_t)(p - (st->src + 2));
                    if (nlen < sizeof(name)) {
                        for (size_t k = 0; k < nlen; k++)
                            name[k] = (char)pgm_read_byte(st->src + 2 + k);
                    }

                    // Resolve placeholder â†’ replacement string
                    const char* repl = nullptr;
                    const String* replS = nullptr;
                    if      (strcmp(name, "IP"      ) == 0) repl  = st->ip;
                    else if (strcmp(name, "PORT"    ) == 0) repl  = st->port;
                    else if (strcmp(name, "SIZE"    ) == 0) repl  = st->size;
                    else if (strcmp(name, "COPTS_W" ) == 0) replS = &_coW;
                    else if (strcmp(name, "COPTS_G" ) == 0) replS = &_coG;
                    else if (strcmp(name, "COPTS_R" ) == 0) replS = &_coR;
                    else if (strcmp(name, "COPTS_Y" ) == 0) replS = &_coY;
                    else if (strcmp(name, "COPTS_B" ) == 0) replS = &_coB;
                    else if (strcmp(name, "FOPTS"   ) == 0) replS = &_fopts;

                    const char* rp  = repl ? repl : (replS ? replS->c_str() : "");
                    size_t      rlen = strlen(rp);

                    if (out + rlen > maxLen) break; // defer to next chunk

                    memcpy(buf + out, rp, rlen);
                    out += rlen;

                    // Advance past %%NAME%%
                    st->src = p + 2;
                    st->rem = rem2 - 2;
                    continue;
                }

                buf[out++] = (uint8_t)c;
                ++st->src; --st->rem;
            }

            if (st->rem == 0 && out == 0) {
                delete st;
                return 0; // signal end-of-stream to ESPAsyncWebServer
            }
            return out;
        }
    );

    resp->addHeader("Cache-Control", "no-cache");
    request->send(resp);
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
            snprintf(colorHex, sizeof(colorHex), "#%02X%02X%02X",
                     ((c >> 8) & 0xF8), ((c >> 3) & 0xFC), ((c << 3) & 0xF8));
            snprintf(bgHex, sizeof(bgHex), "#%02X%02X%02X",
                     ((bg >> 8) & 0xF8), ((bg >> 3) & 0xFC), ((bg << 3) & 0xF8));
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

        char buffer[UDP_BUFFER_SIZE];
        command.toCharArray(buffer, UDP_BUFFER_SIZE);
        udpHandler->dispatchCommand(buffer);
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "No command provided");
    }
}
