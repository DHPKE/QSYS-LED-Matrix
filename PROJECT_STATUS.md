# QSYS-LED-Matrix — Project Status
> Last updated: 2026-02-20  |  Commit: fe36667

---

## Hardware

| Item | Value |
|---|---|
| Board | WT32-ETH01 (ESP32 + LAN8720) |
| Panel | 64 × 32 HUB75 LED Matrix |
| IP (DHCP) | 10.1.1.22 (or whatever DHCP hands out) |
| IP (fallback, no DHCP) | **10.10.10.99 / 24** |
| UDP Port | 21324 |
| Web UI | http://\<IP\>/ |
| mDNS | wt32-led-matrix.local |
| Upload port | COM3 @ 921600 |

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
