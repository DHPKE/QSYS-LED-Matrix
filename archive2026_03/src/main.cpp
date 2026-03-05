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
// Minimal status-only web UI. Dynamic values (IP, port, size) are injected
// via chunked response with %%PLACEHOLDER%% token replacement.
static const char WEBPAGE_HTML[] PROGMEM = R"HTMLEOF(<!DOCTYPE html>
<html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>LED Matrix Status</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#0f0f1e,#1a1a2e);color:#e0e0e0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.wrap{max-width:600px;width:100%}
.hdr{text-align:center;margin-bottom:32px;padding:32px 24px;background:linear-gradient(135deg,#1e3c72,#2a5298);border-radius:12px;box-shadow:0 8px 32px rgba(0,0,0,.3)}
.hdr h1{font-size:2.2em;color:#fff;margin-bottom:8px}
.hdr p{color:#a0c4ff;font-size:1.1em}
.card{background:rgba(30,30,46,.9);border-radius:10px;padding:24px;border:1px solid rgba(255,255,255,.1);box-shadow:0 4px 16px rgba(0,0,0,.2)}
.info{display:flex;flex-direction:column;gap:20px}
.row{display:flex;justify-content:space-between;align-items:center;padding:12px 0;border-bottom:1px solid rgba(255,255,255,.1)}
.row:last-child{border-bottom:none}
.label{color:#888;font-weight:600;font-size:.85em;text-transform:uppercase;letter-spacing:.5px}
.value{color:#e0e0e0;font-size:1.1em;font-family:monospace;background:rgba(0,0,0,.3);padding:6px 12px;border-radius:6px;border:1px solid rgba(255,255,255,.1)}
.st{padding:8px 16px;border-radius:20px;font-size:.9em;font-weight:600;display:inline-block}
.st.ok{background:#1a472a;color:#4ade80}
.st.err{background:#7f1d1d;color:#f87171}
.footer{text-align:center;margin-top:24px;color:#666;font-size:.85em}
</style></head><body>
<div class="wrap">
<div class="hdr">
<h1>LED Matrix Controller</h1>
<p>%%SIZE%% RGB Display</p>
</div>
<div class="card">
<div class="info">
<div class="row">
<span class="label">IP Address</span>
<span class="value" id="ip">%%IP%%</span>
</div>
<div class="row">
<span class="label">UDP Port</span>
<span class="value">%%PORT%%</span>
</div>
<div class="row">
<span class="label">Status</span>
<span id="status" class="st ok">Connected</span>
</div>
</div>
</div>
<div class="footer">
Use Q-SYS Plugin to control display
</div>
</div>
<script>
let pFail=0;
function checkStatus(){
const ac=new AbortController();
const tid=setTimeout(()=>ac.abort(),3000);
fetch('/api/config',{signal:ac.signal})
.then(r=>{clearTimeout(tid);return r.json();})
.then(()=>{
pFail=0;
const st=document.getElementById('status');
st.textContent='Connected';
st.className='st ok';
setTimeout(checkStatus,5000);
})
.catch(()=>{
clearTimeout(tid);
pFail++;
if(pFail>=3){
const st=document.getElementById('status');
st.textContent='Disconnected';
st.className='st err';
}
setTimeout(checkStatus,pFail<3?5000:10000);
});
}
window.addEventListener('load',()=>setTimeout(checkStatus,2000));
</script></body></html>)HTMLEOF";


// handleRoot â€” streams the PROGMEM page via a chunked response.
// Peak RAM use â‰ˆ 1 TCP MSS (1460 bytes) stack buffer inside ESPAsyncWebServer.
// Zero heap allocation for the HTML body.
void handleRoot(AsyncWebServerRequest *request) {

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
                    if      (strcmp(name, "IP"      ) == 0) repl = st->ip;
                    else if (strcmp(name, "PORT"    ) == 0) repl = st->port;
                    else if (strcmp(name, "SIZE"    ) == 0) repl = st->size;

                    if (!repl) repl = ""; // Unknown placeholder → empty string
                    size_t rlen = strlen(repl);

                    if (out + rlen > maxLen) break; // defer to next chunk

                    memcpy(buf + out, repl, rlen);
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
            // font: map fontName to the numeric ID used by the WebUI selector
            // 1=Arial(Bold)  2=Verdana  3=Impact  (default to 1)
            int fontId = 1;
            if (strstr(seg->fontName, "verdana") || strstr(seg->fontName, "roboto8"))
                fontId = 2;
            else if (strstr(seg->fontName, "impact"))
                fontId = 3;
            segObj["font"] = fontId;

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
