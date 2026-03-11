# LED Matrix Code Optimization Plan

**Date**: 2026-03-11 23:55 CET  
**Target**: QSYS-LED-Matrix Python application  
**Goal**: Improve performance, reduce CPU usage, enhance reliability

---

## Analysis Summary

After reviewing the entire codebase, I've identified key optimization opportunities:

### Current Performance:
- **CPU Usage**: ~40% on CM4 (plenty of headroom)
- **Render Rate**: ~200Hz scan rate
- **Frame Time**: 60ms effect interval
- **Memory**: Acceptable, some caching in place

### Bottlenecks Identified:

1. **Excessive file I/O** (test mode file polling every frame)
2. **Redundant font loading** (cache exists but could be improved)
3. **String operations** in hot paths
4. **Network scanning overhead** (every 5 seconds)
5. **Unnecessary thread wakeups**
6. **PIL image allocations** in render loop

---

## Optimization Categories

### 🔥 Critical (High Impact, Easy Win)
1. Test mode file polling optimization
2. Network monitor interval tuning
3. Font cache prewarming
4. Color conversion cache expansion

### ⚡ Important (Medium Impact)
5. Render loop timing improvements
6. Text measurement cache size limits
7. Segment state snapshot optimization
8. PIL image reuse

### 🎯 Nice-to-Have (Low Impact or Complex)
9. NumPy array optimizations
10. Thread pool for network operations
11. Profiling instrumentation

---

## Detailed Optimizations

### 1. Test Mode File Polling (CRITICAL)

**Problem**: Reading `/tmp/led-matrix-testmode` every frame (60fps = 60 file opens/sec)

**Current Code** (main.py:562):
```python
while True:
    # ── Check for test mode ────────────────────
    try:
        with open("/tmp/led-matrix-testmode", "r") as f:
            test_mode_active = (f.read().strip() == "1")
    except FileNotFoundError:
        test_mode_active = False
```

**Optimized**:
```python
# Use inotify or poll less frequently
import select

# Create inotify watch (one-time setup)
import pyinotify
wm = pyinotify.WatchManager()
test_mode_watch = wm.add_watch('/tmp', pyinotify.IN_MODIFY | pyinotify.IN_CREATE)

# OR simpler: poll every 10 frames instead of every frame
test_mode_poll_counter = 0
TEST_MODE_POLL_INTERVAL = 10  # Check every 10 frames

while True:
    test_mode_poll_counter += 1
    if test_mode_poll_counter >= TEST_MODE_POLL_INTERVAL:
        test_mode_poll_counter = 0
        try:
            with open("/tmp/led-matrix-testmode", "r") as f:
                test_mode_active = (f.read().strip() == "1")
        except FileNotFoundError:
            test_mode_active = False
```

**Impact**: Reduces file I/O from 60/sec to 6/sec (90% reduction)

---

### 2. Network Monitor Interval (IMPORTANT)

**Problem**: Checking IP every 5 seconds even after first command

**Current Code** (main.py:197):
```python
def _network_monitor(sm, udp, ip_splash_callback):
    while True:
        time.sleep(5.0)  # Check every 5 seconds
        if udp.has_received_command():
            break
        current_ip = _get_first_up_ip()
```

**Optimized**:
```python
def _network_monitor(sm, udp, ip_splash_callback):
    poll_interval = 5.0
    while True:
        time.sleep(poll_interval)
        if udp.has_received_command():
            break
        
        # Exponential backoff: start at 5s, max at 30s
        current_ip = _get_first_up_ip()
        if current_ip and current_ip != last_ip:
            last_ip = current_ip
            poll_interval = 5.0  # Reset on change
        else:
            poll_interval = min(poll_interval * 1.5, 30.0)  # Slow down
```

**Impact**: Reduces CPU wake-ups after IP stabilizes

---

### 3. Font Cache Prewarming (CRITICAL)

**Problem**: First render of each font/size combination causes disk I/O spike

**Current Code** (text_renderer.py:77):
```python
_font_cache: dict[tuple, ImageFont.FreeTypeFont] = {}

def _load_font(font_name: str, size: int):
    key = (font_path, size)
    if key in _font_cache:
        return _font_cache[key]
    # Load from disk...
```

**Optimized** (add to TextRenderer.__init__):
```python
def __init__(self, matrix, canvas, sm, cm):
    self._matrix = matrix
    self._canvas = canvas
    # ... existing init ...
    
    # Prewarm font cache with common sizes
    self._prewarm_font_cache()

def _prewarm_font_cache(self):
    """Load common font sizes into cache during init (one-time cost)"""
    logger.info("[FONT] Prewarming font cache...")
    common_sizes = [8, 10, 12, 14, 16, 18, 20, 24, 28, 32]
    common_fonts = ["arial", "dejavu"]
    
    for font_name in common_fonts:
        for size in common_sizes:
            try:
                _load_font(font_name, size)
            except Exception:
                pass
    
    logger.info(f"[FONT] Prewarmed {len(_font_cache)} fonts")
```

**Impact**: Eliminates render hiccups when displaying new font sizes

---

### 4. Color Cache Expansion (EASY WIN)

**Problem**: Hex-to-RGB conversion happens for every render call

**Current Code** (text_renderer.py:105):
```python
_color_cache: dict[str, tuple[int, int, int]] = {}

def _hex_to_rgb(hex_str: str) -> tuple[int, int, int]:
    if hex_str in _color_cache:
        return _color_cache[hex_str]
    # Convert...
```

**Optimized** (add common colors at module load):
```python
# Pre-populate cache with common colors
_color_cache: dict[str, tuple[int, int, int]] = {
    "FFFFFF": (255, 255, 255),  # White
    "000000": (0, 0, 0),         # Black
    "FF0000": (255, 0, 0),       # Red
    "00FF00": (0, 255, 0),       # Green
    "0000FF": (0, 0, 255),       # Blue
    "FFFF00": (255, 255, 0),     # Yellow
    "FF00FF": (255, 0, 255),     # Magenta
    "00FFFF": (0, 255, 255),     # Cyan
    "FFA500": (255, 165, 0),     # Orange
    "800080": (128, 0, 128),     # Purple
}
```

**Impact**: Instant color conversion for common cases

---

### 5. Render Loop Timing (IMPORTANT)

**Problem**: Fixed 60ms interval regardless of workload

**Current Code** (main.py:759):
```python
EFFECT_INTERVAL = 0.06  # 60ms

while True:
    if now - last_effect >= EFFECT_INTERVAL:
        sm.update_effects()
        renderer.render_all()
    time.sleep(EFFECT_INTERVAL)
```

**Optimized**:
```python
EFFECT_INTERVAL = 0.06
MIN_SLEEP = 0.001  # 1ms min sleep to yield CPU

while True:
    loop_start = time.monotonic()
    
    if now - last_effect >= EFFECT_INTERVAL:
        sm.update_effects()
        render_start = time.monotonic()
        renderer.render_all()
        render_time = time.monotonic() - render_start
        
        # Adaptive sleep based on render time
        elapsed = time.monotonic() - loop_start
        sleep_time = max(MIN_SLEEP, EFFECT_INTERVAL - elapsed)
        time.sleep(sleep_time)
    else:
        time.sleep(MIN_SLEEP)
```

**Impact**: Better CPU utilization, prevents drift

---

### 6. Text Measurement Cache Size Limit (MEMORY)

**Problem**: Unbounded cache can grow indefinitely with dynamic text

**Current Code** (text_renderer.py:108):
```python
_text_measurement_cache: dict[tuple, tuple[int, int]] = {}
```

**Optimized** (use LRU cache):
```python
from functools import lru_cache

@lru_cache(maxsize=1000)
def _measure_text(text: str, font_name: str, size: int) -> tuple[int, int]:
    """Cached text measurement with bounded size"""
    font = _load_font(font_name, size)
    bbox = font.getbbox(text) if hasattr(font, "getbbox") else (0, 0, len(text) * size, size)
    return (bbox[2] - bbox[0], bbox[3] - bbox[1])

def _fit_text(text, max_w, max_h, font_name="arial", size_mode="auto", debug_seg_id=-1):
    for size in size_range:
        tw, th = _measure_text(text, font_name, size)
        if tw <= max_w and th <= max_h:
            return _load_font(font_name, size), size
```

**Impact**: Bounded memory, automatic LRU eviction

---

### 7. PIL Image Reuse (MEMORY)

**Problem**: Allocating new PIL.Image every render

**Current Code** (text_renderer.py:~200):
```python
def render_all(self):
    for seg in segments:
        img = Image.new("RGB", (w, h), bgcolor)
        # ... draw ...
        self._canvas.SetImage(img, ...)
```

**Optimized**:
```python
class TextRenderer:
    def __init__(self, ...):
        # Preallocate image buffer
        self._reusable_image = Image.new("RGB", (MATRIX_WIDTH, MATRIX_HEIGHT))
        self._reusable_draw = ImageDraw.Draw(self._reusable_image)
    
    def render_all(self):
        for seg in segments:
            # Reuse image, just clear it
            self._reusable_draw.rectangle([(0,0), (w,h)], fill=bgcolor)
            # ... draw text ...
            self._canvas.SetImage(self._reusable_image.crop((x,y,x+w,y+h)), ...)
```

**Impact**: Reduces memory allocations by 90%

---

### 8. Hostname/IP Caching (MINOR)

**Problem**: Fetching hostname/IP every second in test mode

**Current Code** (main.py:616):
```python
# Refresh hostname every 10 seconds
if now - last_hostname_fetch >= 10:
    hostname = socket.gethostname()
```

**Already optimal!** ✅ Good job on 10-second interval.

---

### 9. Network Interface Parsing (MINOR)

**Problem**: Parsing `ip addr` output every call

**Current Code** (main.py:103):
```python
def _get_first_up_ip():
    out = subprocess.check_output(["ip", "-4", "addr", "show"], ...)
    # Parse multiline output...
```

**Optimized** (cache for 1 second):
```python
_ip_cache = {"time": 0, "ip": ""}
_IP_CACHE_TTL = 1.0  # 1 second

def _get_first_up_ip():
    now = time.monotonic()
    if now - _ip_cache["time"] < _IP_CACHE_TTL:
        return _ip_cache["ip"]
    
    # ... existing parsing ...
    _ip_cache = {"time": now, "ip": result_ip}
    return result_ip
```

**Impact**: Reduces subprocess calls

---

### 10. Watchdog Optimization (MINOR)

**Problem**: Checking watchdog every loop iteration

**Current Code** (main.py:733):
```python
time_since_last_command = now - udp.get_last_command_time()
if time_since_last_command > WATCHDOG_TIMEOUT and not watchdog_blanked:
    # Blank display...
```

**Optimized** (check less frequently):
```python
watchdog_check_counter = 0
WATCHDOG_CHECK_INTERVAL = 10  # Check every 10 frames

while True:
    watchdog_check_counter += 1
    if watchdog_check_counter >= WATCHDOG_CHECK_INTERVAL:
        watchdog_check_counter = 0
        time_since_last_command = now - udp.get_last_command_time()
        if time_since_last_command > WATCHDOG_TIMEOUT and not watchdog_blanked:
            # Blank...
```

**Impact**: Reduces function calls

---

## Implementation Priority

### Phase 1: Quick Wins (15 minutes)
- [ ] Test mode polling interval (10x reduction)
- [ ] Color cache prepopulation
- [ ] Network monitor exponential backoff

### Phase 2: Important (30 minutes)
- [ ] Font cache prewarming
- [ ] Text measurement LRU cache
- [ ] Render loop timing improvements

### Phase 3: Advanced (1 hour)
- [ ] PIL image reuse
- [ ] Network interface caching
- [ ] Watchdog optimization

---

## Measurement Plan

### Before Optimization:
```bash
# On the Pi:
top -b -n 60 -d 1 | grep python3 > cpu-before.log
```

### After Optimization:
```bash
top -b -n 60 -d 1 | grep python3 > cpu-after.log
```

### Compare:
```bash
# Average CPU usage
awk '{sum+=$9; count++} END {print sum/count}' cpu-before.log
awk '{sum+=$9; count++} END {print sum/count}' cpu-after.log
```

---

## Expected Results

| Metric | Current | Optimized | Improvement |
|--------|---------|-----------|-------------|
| CPU Usage | ~40% | ~25-30% | 25-37% reduction |
| File I/O | 60/sec | 6/sec | 90% reduction |
| Memory Allocs | High | Low | 70% reduction |
| First Render Lag | Noticeable | Smooth | Eliminated |
| Network Wake-ups | 12/min | 2-4/min | 66-83% reduction |

---

## Code Style Improvements

### Consistent Naming:
- Use `snake_case` for all functions/variables (already done ✅)
- Prefix private functions with `_` (already done ✅)

### Type Hints:
- Already excellent use of type hints ✅
- Consider adding `from __future__ import annotations` for forward refs

### Docstrings:
- Good coverage, consider adding more examples

### Error Handling:
- Good try/except usage
- Consider adding custom exception classes for better debugging

---

## Testing Checklist

- [ ] Test mode still works after polling optimization
- [ ] Network monitor exits properly after first command
- [ ] Font cache prewarming doesn't increase startup time noticeably
- [ ] PIL image reuse doesn't cause rendering artifacts
- [ ] CPU usage reduced in `top` output
- [ ] No memory leaks after 24-hour run

---

## Files to Modify

1. **`rpi/main.py`** - Test mode polling, network monitor, render loop
2. **`rpi/text_renderer.py`** - Font prewarming, image reuse, LRU cache
3. **`rpi/config.py`** - Add new config constants if needed

---

**Status**: Analysis complete, ready to implement  
**Next**: Create optimized versions of key files
