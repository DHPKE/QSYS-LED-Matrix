# QSYS-LED-Matrix — Project Status
> Last updated: 2026-03-01  |  Commit: fbcb3bb

---

## Current Hardware Configuration

### Raspberry Pi CM4
| Item | Value |
|---|---|
| Board | Raspberry Pi Compute Module 4 |
| Panel | 64 × 32 HUB75 LED Matrix |
| IP (Static) | **10.1.1.25 / 24** |
| eth0 | DOWN (10.20.30.40 fallback) |
| eth1 | UP (10.1.1.25 static) |
| UDP Port | 21324 |
| Web UI | http://10.1.1.25:8080/ |
| Runtime User | daemon (rgbmatrix drops privileges) |
| Python | 3.13.5 |
| Dependencies | numpy 2.2.4, pillow, rgbmatrix |

---

## Python Implementation (Current) — `rpi/`

### Recent Updates (2026-03-01)
- ✅ Full 4-way rotation (0°/90°/180°/270°)
- ✅ Test mode with 5-color scrolling bars
- ✅ IP splash at forced 0° rotation
- ✅ Static IP configuration via NetworkManager
- ✅ WebUI hostname change
- ✅ WebUI reboot button
- ✅ Helper scripts for privileged operations
- ✅ Enhanced IP detection (only UP interfaces)

### Key Files

| File | Role |
|---|---|
| `main.py` | Service entry point, UDP listener, IP splash, main loop |
| `config.py` | Constants, layout presets (landscape + portrait) |
| `segment_manager.py` | 8-segment RAM model, dirty tracking, effects |
| `text_renderer.py` | PIL-based rendering, rotation support, transparent backgrounds |
| `udp_handler.py` | JSON UDP parser, layout application, callbacks |
| `web_server.py` | HTTP server (port 8080), network config, hostname, reboot |
| `configure-network.sh` | NetworkManager static IP / DHCP helper |
| `set-hostname.sh` | Hostname change helper |
| `reboot-device.sh` | System reboot helper |
| `install.sh` | Automated installation script |

### Features

**Rotation:**
- 0° / 90° / 180° / 270° rotation
- Persists to `/var/lib/led-matrix/config.json`
- Requires service restart to apply
- IP splash forced to 0° for easy reading

**Layouts:**
- Landscape presets (64×32) for 0° and 180°
- Portrait presets (32×64) for 90° and 270°
- Auto-selects correct preset based on rotation

**Test Mode:**
- 5 color bars (Red, Green, Blue, Yellow, White)
- Scrolls left-to-right at 60fps
- Text overlay with transparent background
- 4-state cycle: hostname top → blank → IP bottom → blank
- Forced to 0° rotation
- Toggle via WebUI or `/tmp/led-matrix-testmode`

**WebUI (port 8080):**
- Network configuration (DHCP / static IP)
- Hostname change
- Reboot button
- Test mode toggle
- Minimal UI (matrix control via Q-SYS UDP)

**Helper Scripts:**
- Run with sudo via `/etc/sudoers.d/led-matrix`
- Daemon user can execute (rgbmatrix drops privileges)
- See `rpi/HELPER-SCRIPTS.md` for details

### Service Configuration

**File:** `/etc/systemd/system/led-matrix.service`
```ini
[Service]
ExecStart=/usr/bin/python3 /opt/led-matrix/main.py
WorkingDirectory=/opt/led-matrix
Restart=always
RestartSec=2
User=root  # rgbmatrix drops to daemon
```

**Config Storage:** `/var/lib/led-matrix/config.json`
```json
{"orientation": "landscape", "rotation": 0, "group_id": 0}
```

### IP Detection

Priority order:
1. eth* interfaces (eth0, eth1, ...)
2. wlan* interfaces (wlan0, ...)
3. Other interfaces

Only shows IPs from interfaces that are:
- **UP** (has LOWER_UP or state UP)
- **NOT DOWN** (no state DOWN flag)
- **NOT NO-CARRIER** (has link)

Example:
- eth0: `state DOWN, NO-CARRIER` → ignored (10.20.30.40)
- eth1: `state UP, LOWER_UP` → used (10.1.1.25) ✅

---

## Legacy ESP32 Firmware — `src/` (Deprecated)

> **Note:** The project has migrated to Python on Raspberry Pi CM4.
> ESP32 code is preserved for reference but no longer actively developed.

---

## Firmware — `src/`

### Build
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e wt32-eth01 --target upload
```
`platformio.ini` key settings:
```ini
board                    = wt32-eth01
build_flags              = -DBOARD_HAS_PSRAM=0 -DNO_DISPLAY
board_build.partitions   = no_ota.csv      ; 2 MB app + 1.9 MB LittleFS
upload_port              = COM3
upload_speed             = 921600
```
> `NO_DISPLAY` keeps HUB75 DMA init out of the build — remove it once the
> panel is physically connected.

### Boot sequence
1. WDT armed (30 s)
2. LittleFS mount → load `brightness` from `/config.json`
3. `setupEthernet()` — waits 15 s for DHCP
   - **DHCP received** → `ETH_GOT_IP` event → start UDP, mDNS,
     display IP on matrix (white, fullscreen, autoSize)
   - **No DHCP after 15 s** → `ETH.config(10.10.10.99, …)` static fallback,
     display fallback IP on matrix
4. Web server starts on port 80
5. Matrix DMA init (skipped when `NO_DISPLAY`)
6. IP splash stays until first UDP command arrives → `segmentManager.clearAll()`

### Key files

| File | Role |
|---|---|
| `config.h` | Pin map, constants, fallback IP macros |
| `segment_manager.h` | 4-segment RAM model, effect timers, clearAll |
| `fonts.h` | Font lookup: Arial=FreeSansBold, Verdana=FreeSans, Impact=FreeMonoBold, TomThumb fallback |
| `text_renderer.h` | `fitFont()` auto-size (26→22→16→12→TomThumb), sharp rendering, baseline centering |
| `udp_handler.h` | JSON UDP parser, integer protocol, `applyLayoutPreset()`, IP-splash clear flag |
| `main.cpp` | ETH event handler, web server, `showIPOnDisplay()`, loop |

---

## Protocol

### Text command
```json
{"cmd":"text","seg":0,"text":"HELLO","color":1,"bgcolor":14,"font":1,"effect":0,"align":"C"}
```

### Layout command (single packet)
```json
{"cmd":"layout","preset":3}
```

### Other commands
```json
{"cmd":"clear","seg":0}
{"cmd":"clear_all"}
{"cmd":"brightness","value":200}
```

### Integer enums

**Layout presets (1–6)**
| # | Name | Geometry |
|---|---|---|
| 1 | Fullscreen | seg0: 64×32 |
| 2 | Split Horizontal | seg0: 64×16 top, seg1: 64×16 bottom |
| 3 | Split Vertical | seg0: 32×32 left, seg1: 32×32 right |
| 4 | Quad | seg0-3: 32×16 each (2×2 grid) |
| 5 | Thirds | seg0-2: ~21×32 columns |
| 6 | Triple | seg0: 32×32 left, seg1: 32×16 top-right, seg2: 32×16 bottom-right |
| 11-14 | Single-seg fullscreen | seg(N-11) covers full panel |

**Colors (1–14)**
`1=White 2=Red 3=Lime 4=Blue 5=Yellow 6=Magenta 7=Cyan 8=Orange 9=Purple 10=Gold 11=Grey 12=Black`

**Fonts (1–3)**
`1=Arial(FreeSansBold)  2=Verdana(FreeSans)  3=Impact(FreeMonoBold)`

**Effects (0–3)**
`0=None  1=Scroll  2=Blink  3=Fade`

**Align:** `"L"` `"C"` `"R"`

> All fields also accept legacy hex strings (`"FFFFFF"`) and name strings
> (`"arial"`, `"scroll"`) for backward compatibility.

---

## Q-SYS Plugin — `qsys-plugin/WT32_LEDMatrix_v4.qplug`

| Field | Detail |
|---|---|
| Plugin ID | `dhpke.wt32.led.matrix.4.0.0` |
| Default IP | 10.1.1.22 |
| Default port | 21324 |
| Layout control | ComboBox "1 – Fullscreen" … "6 – Triple" |
| Color controls | ComboBox "1 – White" … "14 – Black" (per segment, text + BG) |
| Font control | ComboBox "1 – Arial (Bold)" / "2 – Verdana" / "3 – Impact" |
| Effect control | ComboBox "0 – None" … "3 – Fade" |
| Align | ComboBox "L" / "C" / "R" |
| Auto-send | 400 ms debounce on all text/color/font/effect/align changes |
| Brightness | Knob 0-255, 500 ms debounce |
| Invert button | Swaps color ↔ bgcolor, then auto-sends |
| `choiceToInt()` | Parses "N – Name" OR plain "N" string — Q-SYS scripts can write integers |

---

## Web UI (embedded in `main.cpp`)

- Canvas preview (64×32 px, scaled ×4) — shows bgcolor even with empty text
- 6 layout preset buttons with `data-preset` integer matching
- Font selects: value 1/2/3 (integer strings)
- `applyLayout(preset)` sends `{"cmd":"layout","preset":N}` — updates local
  `segmentBounds` from `LAYOUTS` table for immediate canvas repaint
- Poll every 2 s; poll frozen 8 s after layout change (`layoutLockUntil`)
- `drawSegmentOnCanvas()` always called for active segments — bgcolor shown
  even when text field is empty

---

## Known Limitations / Next Steps

- [ ] Remove `NO_DISPLAY` flag once HUB75 panel is physically connected
- [ ] Test all 6 layout presets on real panel
- [ ] Verify font auto-size on small segments (32×16 quad cells)
- [ ] FADE effect not yet implemented in `segment_manager.h` (renders as none)
- [ ] Web UI preview uses monospace canvas font — not pixel-accurate vs GFX fonts
- [ ] Consider removing web canvas preview (reduce HTML size) as discussed

---

## Commit History (recent)

```
fe36667  feat: integer protocol, 6 layout presets, sharp fonts, fallback IP, IP splash
```
