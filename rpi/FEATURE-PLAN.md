# Python Version Features - Implementation Plan

## Working in: `/Users/user/.openclaw/workspace/QSYS-LED-Matrix/rpi/`

---

## 1. Rotation Support (0°, 90°, 180°, 270°)

### Current State
- Python version has `ORIENTATION` (landscape/portrait) but no rotation
- C++ version had rotation via pixel mapper (required restart)
- Python can use PIL Image.rotate() for dynamic rotation without restart

### Implementation Strategy

**Add to `config.py`:**
```python
ROTATION = 0  # 0, 90, 180, 270 degrees
```

**Add to `udp_handler.py`:**
```python
_rotation = ROTATION  # 0, 90, 180, 270
_rotation_lock = threading.Lock()

def get_rotation() -> int:
    with _rotation_lock:
        return _rotation

def _set_rotation(value: int):
    global _rotation
    with _rotation_lock:
        if value in [0, 90, 180, 270]:
            _rotation = value
            _save_config()

# UDP command handler:
elif cmd == "rotation":
    angle = doc.value("value", 0)
    if angle in [0, 90, 180, 270]:
        _set_rotation(angle)
        if self._rotation_callback:
            self._rotation_callback(angle)
```

**Add to `text_renderer.py`:**
```python
# In render() method, after creating PIL Image:
rotation = udp_handler.get_rotation()
if rotation != 0:
    # Rotate the image
    img = img.rotate(-rotation, expand=True)  # Negative for clockwise
    
    # Handle canvas size changes
    if rotation in [90, 270]:
        # Swap width/height
        canvas_w, canvas_h = canvas_h, canvas_w
```

**UDP Command:**
```json
{"cmd":"rotation", "value":90}
```

**Q-SYS Plugin Integration:**
- Add rotation dropdown: 0°, 90°, 180°, 270°
- No restart needed (Python handles it dynamically)

---

## 2. Curtain Visual Feedback

### Requirements
- Boolean state: open/closed
- Visual feedback: curtain graphics on left/right edges
- Should not interfere with main content

### Design Options

#### Option A: Border Overlay (Recommended)
- Draw 2-4 pixel wide curtain pattern on left/right edges
- Overlays on top of existing content
- Simple vertical lines or texture pattern

#### Option B: Dedicated Curtain Segment
- Reserve segments 0-1 for curtain graphics
- Main content in segments 2-7
- More flexible but uses segment slots

#### Option C: Background Layer
- Curtain as persistent background layer
- Content rendered on top
- Most realistic but complex

### Implementation (Option A - Border Overlay)

**Add to `config.py`:**
```python
# Curtain visualization
CURTAIN_ENABLED = True
CURTAIN_WIDTH = 3  # pixels per side
CURTAIN_COLOR_OPEN = "00FF00"    # Green
CURTAIN_COLOR_CLOSED = "FF0000"  # Red
CURTAIN_PATTERN = "||||"  # Vertical bars
```

**Add to `segment_manager.py`:**
```python
class SegmentManager:
    def __init__(self, ...):
        self._curtain_state = False  # False=closed, True=open
        self._curtain_lock = threading.Lock()
    
    def set_curtain_state(self, is_open: bool):
        """Set curtain state and redraw."""
        with self._curtain_lock:
            self._curtain_state = is_open
        self.mark_dirty()
    
    def get_curtain_state(self) -> bool:
        with self._curtain_lock:
            return self._curtain_state
    
    def _draw_curtain_overlay(self, canvas):
        """Draw curtain graphics on canvas edges."""
        if not CURTAIN_ENABLED:
            return
        
        is_open = self._curtain_state
        color = CURTAIN_COLOR_OPEN if is_open else CURTAIN_COLOR_CLOSED
        rgb = (int(color[0:2], 16), int(color[2:4], 16), int(color[4:6], 16))
        
        width = CURTAIN_WIDTH
        height = canvas.height
        
        # Left curtain
        if is_open:
            # Curtain pulled to the side (thin)
            for y in range(0, height, 2):
                canvas.SetPixel(0, y, rgb[0], rgb[1], rgb[2])
        else:
            # Curtain closed (wider)
            for x in range(width):
                for y in range(height):
                    if (x + y) % 2 == 0:  # Checkerboard pattern
                        canvas.SetPixel(x, y, rgb[0], rgb[1], rgb[2])
        
        # Right curtain (mirror)
        right_edge = canvas.width - 1
        if is_open:
            for y in range(0, height, 2):
                canvas.SetPixel(right_edge, y, rgb[0], rgb[1], rgb[2])
        else:
            for x in range(width):
                for y in range(height):
                    if (x + y) % 2 == 0:
                        canvas.SetPixel(right_edge - x, y, rgb[0], rgb[1], rgb[2])
    
    def render(self, ...):
        # Existing render code...
        # At the end, draw curtain overlay:
        self._draw_curtain_overlay(canvas)
```

**Add to `udp_handler.py`:**
```python
def handle_message(self, data: bytes, addr):
    # ... existing code ...
    
    elif cmd == "curtain":
        state = doc.value("state", "closed")  # "open" or "closed"
        is_open = (state.lower() == "open")
        self._sm.set_curtain_state(is_open)
        logger.info(f"[UDP] Curtain → {state}")
```

**UDP Commands:**
```json
{"cmd":"curtain", "state":"open"}
{"cmd":"curtain", "state":"closed"}
```

**Q-SYS Plugin Integration:**
```lua
-- Add curtain control
Controls.curtain_state.EventHandler = function(ctl)
    local state = ctl.Boolean and "open" or "closed"
    SendCommand(buildJson({
        cmd="curtain", state=state, group=activeGroup
    }))
end
```

### Visual Design Examples

**Closed Curtain (wider, red):**
```
###....content....###
###....content....###
###....content....###
```

**Open Curtain (thin, green):**
```
|......content......|
|......content......|
|......content......|
```

---

## 3. Integration with Q-SYS Plugin

### New Controls to Add

**Rotation:**
- Control: `rotation` (ComboBox)
- Choices: 0°, 90°, 180°, 270°
- Position: Below orientation controls

**Curtain:**
- Control: `curtain_open` (Toggle button)
- Legend: "Curtain Open"
- Color: Green (open) / Red (closed)
- Position: Near frame controls

---

## Summary

**Rotation:**
- ✅ Dynamic (no restart needed)
- ✅ PIL Image.rotate() for flexibility
- ✅ Supports all 4 angles
- ✅ Q-SYS dropdown integration

**Curtain:**
- ✅ Simple boolean state
- ✅ Visual overlay (doesn't use segment slots)
- ✅ Customizable colors/width
- ✅ Pattern options (bars, checkerboard, gradient)
- ✅ Q-SYS toggle button integration

**Files to modify:**
1. `config.py` - Add ROTATION and CURTAIN settings
2. `udp_handler.py` - Add rotation and curtain commands
3. `segment_manager.py` - Add curtain overlay rendering
4. `text_renderer.py` - Add rotation transform
5. `main.py` - Add rotation callback

**Testing commands:**
```bash
# Rotation
echo '{"cmd":"rotation","value":90}' | nc -u -w1 10.1.1.99 21324

# Curtain
echo '{"cmd":"curtain","state":"open"}' | nc -u -w1 10.1.1.99 21324
echo '{"cmd":"curtain","state":"closed"}' | nc -u -w1 10.1.1.99 21324
```

Ready to implement once your fresh Raspbian is up! 🎯
