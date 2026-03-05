# Layout 15 & 16 - Voice-Over Layouts

## Version 7.0.10 (VO Layouts)

Added two new layouts optimized for voice-over video production and live streaming overlays.

---

## Layout 15: VO-left

**Use Case:** Large speaker identification area on left, small status/logo on bottom-right

### Landscape Mode (64×32)
- **Segment 1:** 53×32 pixels (5/6 width, aligned left)
  - Position: (0, 0)
  - Size: 53×32
  - Use: Speaker name, title, large text
  
- **Segment 3:** 32×16 pixels (quarter bottom-right)
  - Position: (32, 16)
  - Size: 32×16
  - Use: "LIVE" indicator, timestamp, logo

### Portrait Mode (32×64)
- **Segment 1:** 32×53 pixels (5/6 height, aligned top)
- **Segment 3:** 16×32 pixels (quarter bottom-right)

### Example Commands
```json
{"cmd":"layout","preset":15}
{"cmd":"text","seg":1,"text":"John Smith","color":"FFFFFF","bgcolor":"0000FF","align":"C"}
{"cmd":"text","seg":3,"text":"LIVE","color":"FFFFFF","bgcolor":"FF0000","align":"C"}
```

---

## Layout 16: VO-right

**Use Case:** Right sidebar for info/subtitles, small status/logo on bottom-right

### Landscape Mode (64×32)
- **Segment 2:** 21×32 pixels (1/3 width, aligned top-right)
  - Position: (43, 0)
  - Size: 21×32
  - Use: Info sidebar, scrolling text, subtitles
  
- **Segment 3:** 32×16 pixels (quarter bottom-right)
  - Position: (32, 16)
  - Size: 32×16
  - Use: "REC" indicator, timestamp, logo

### Portrait Mode (32×64)
- **Segment 2:** 32×21 pixels (1/3 height, aligned bottom)
- **Segment 3:** 16×32 pixels (quarter bottom-right)

### Example Commands
```json
{"cmd":"layout","preset":16}
{"cmd":"text","seg":2,"text":"Recording","color":"FFFFFF","bgcolor":"00FF00","align":"C"}
{"cmd":"text","seg":3,"text":"REC","color":"FFFFFF","bgcolor":"FF0000","align":"C"}
```

---

## Visual Layout

### Layout 15 (Landscape 64×32)
```
┌─────────────────────────────────────────────────────┬───────────┐
│                                                     │           │
│             Segment 1 (53×32)                       │           │
│          Speaker Name / Title                       │           │
│                5/6 width left                       │           │
│                                                     │           │
│                                                     │           │
│                                                     ├───────────┤
│                                                     │  Seg 3    │
│                                                     │  LIVE     │
└─────────────────────────────────────────────────────┴───────────┘
                53 pixels                                 32×16
```

### Layout 16 (Landscape 64×32)
```
┌─────────────────────────────────────────────────┬───────────────┐
│                                                 │   Segment 2   │
│                                                 │   (21×32)     │
│                                                 │   Info/Text   │
│                                                 │   1/3 width   │
│                                                 │               │
│                                                 │               │
│                                                 ├───────────────┤
│                                                 │    Seg 3      │
│                                                 │    REC        │
└─────────────────────────────────────────────────┴───────────────┘
                    43 pixels                         21px   32×16
```

---

## Use Cases

### Layout 15 (VO-left)
- Podcast/interview graphics (large speaker name)
- Conference presentations (speaker ID + LIVE status)
- Tutorial videos (instructor name + recording indicator)
- Live streams (host info + viewer count)

### Layout 16 (VO-right)
- News tickers (right sidebar for breaking news)
- Sports scores (right column for stats)
- Social media handles (right bar for @mentions)
- Translation/subtitles (right column for text)

---

## Node-RED Example

```javascript
// Layout 15: Speaker intro
msg.layout = 15;
msg.segment = 1;
msg.text = "Dr. Jane Doe";
msg.color = "FFFFFF";
msg.bgcolor = "0066CC";
return msg;

// Add LIVE indicator
var msg2 = {};
msg2.segment = 3;
msg2.text = "LIVE";
msg2.color = "FFFFFF";
msg2.bgcolor = "FF0000";
return [msg, msg2];
```

---

## Q-SYS Plugin

The Q-SYS plugin can use these layouts by sending preset 15 or 16:

```lua
-- Layout 15
json_cmd = '{"cmd":"layout","preset":15}'
-- Layout 16  
json_cmd = '{"cmd":"layout","preset":16}'
```

---

## Testing

Use the included test script:

```bash
./test-vo-layouts.sh
```

Or test manually:
```bash
# Test Layout 15
echo '{"cmd":"layout","preset":15}' | nc -u 10.1.1.15 21324
echo '{"cmd":"text","seg":1,"text":"SPEAKER","color":"FFFFFF","bgcolor":"0000FF"}' | nc -u 10.1.1.15 21324
echo '{"cmd":"text","seg":3,"text":"LIVE","color":"FFFFFF","bgcolor":"FF0000"}' | nc -u 10.1.1.15 21324

# Test Layout 16
echo '{"cmd":"layout","preset":16}' | nc -u 10.1.1.15 21324
echo '{"cmd":"text","seg":2,"text":"INFO","color":"FFFFFF","bgcolor":"00FF00"}' | nc -u 10.1.1.15 21324
echo '{"cmd":"text","seg":3,"text":"REC","color":"FFFFFF","bgcolor":"FF0000"}' | nc -u 10.1.1.15 21324
```

---

## Deployment

**Version:** 7.0.10  
**Deployed to:** 10.1.1.15 ✅  
**Git commit:** c27d5eb  
**Branch:** feature/curtain-frame-indicator

---

## Dimensions Reference

### Landscape (64×32)
| Layout | Seg | Position | Size | Description |
|--------|-----|----------|------|-------------|
| 15 | 1 | (0, 0) | 53×32 | 5/6 width left |
| 15 | 3 | (32, 16) | 32×16 | Quarter BR |
| 16 | 2 | (43, 0) | 21×32 | 1/3 width right |
| 16 | 3 | (32, 16) | 32×16 | Quarter BR |

### Portrait (32×64)
| Layout | Seg | Position | Size | Description |
|--------|-----|----------|------|-------------|
| 15 | 1 | (0, 0) | 32×53 | 5/6 height top |
| 15 | 3 | (16, 32) | 16×32 | Quarter BR |
| 16 | 2 | (0, 43) | 32×21 | 1/3 height bottom |
| 16 | 3 | (16, 32) | 16×32 | Quarter BR |
