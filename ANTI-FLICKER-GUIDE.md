# Anti-Flicker Solutions for LED Matrix

**Problem**: Visible flicker or rolling bands when filming the LED matrix with a camera/phone.

**Cause**: Camera shutter speed doesn't sync with LED matrix PWM refresh rate.

---

## Quick Fixes (Try These First)

### **Option 1: Camera Settings** (Easiest)
Adjust your camera/phone settings:

1. **Lower the shutter speed**:
   - Use **1/100s or slower** (1/50s, 1/30s, 1/25s)
   - iPhone: Use ProCamera or Filmic Pro app
   - Android: Use Manual Camera or Cinema P3
   - DSLR: Set shutter to 1/100 or slower

2. **Match to refresh rate**:
   - Your matrix refresh: ~200Hz (every 5ms)
   - Camera shutter: 1/100s = 10ms (captures 2 full cycles)
   - 1/50s = 20ms (captures 4 full cycles) — BEST
   - 1/30s = 33ms (captures 6+ cycles) — smoothest

3. **Enable motion blur**:
   - Lower shutter speed adds motion blur, hiding flicker

---

## Matrix Configuration Tweaks

### **Option 2: Increase PWM Bits** (Better Colors, Slower Refresh)

Your current: `MATRIX_PWM_BITS = 8` (256 color levels)

**Try PWM_BITS = 11** for smoother appearance:
```python
# config.py
MATRIX_PWM_BITS = 11  # 2048 levels per channel (smoother gradient)
```

**Trade-off**: Slightly lower refresh rate, but smoother on camera.

---

### **Option 3: Increase GPIO Slowdown** (Higher Refresh Rate)

Your current: `MATRIX_GPIO_SLOWDOWN = 3` (~200Hz)

**Try SLOWDOWN = 2** for faster refresh:
```python
# config.py
MATRIX_GPIO_SLOWDOWN = 2  # ~250-300Hz refresh
```

**Trade-off**: May cause slight instability on Pi Zero 2 W. Test carefully.

---

### **Option 4: Enable PWM Dithering** (Smoother Gradients)

Your current: `MATRIX_PWM_DITHER_BITS = 0` (off)

**Try enabling dithering**:
```python
# config.py
MATRIX_PWM_DITHER_BITS = 1  # Temporal dithering for smoother appearance
```

**Effect**: Reduces visible banding, makes PWM transitions smoother.

---

### **Option 5: Remove Refresh Limit** (Maximum Speed)

Your current: `MATRIX_REFRESH_LIMIT = 200` (capped at 200Hz)

**Try uncapped refresh**:
```python
# config.py
MATRIX_REFRESH_LIMIT = 0  # No limit, run as fast as possible
```

**Trade-off**: Higher CPU usage, may cause instability.

---

## Recommended Configuration for Filming

```python
# config.py - Anti-flicker optimized

MATRIX_GPIO_SLOWDOWN = 2     # Faster refresh (~300Hz)
MATRIX_PWM_BITS = 11         # Smoother colors (2048 levels)
MATRIX_PWM_DITHER_BITS = 1   # Temporal dithering
MATRIX_REFRESH_LIMIT = 0     # Uncapped refresh
MATRIX_BRIGHTNESS = 60       # Slightly brighter (compensates for dithering)
```

---

## Camera Settings Guide

### **iPhone:**
- Use **Filmic Pro** or **ProCamera** app
- Set shutter: **1/50s or 1/100s**
- Set frame rate: **24fps or 30fps** (not 60fps)
- Enable: Motion blur

### **Android:**
- Use **Cinema P3** or **Manual Camera**
- Shutter speed: **1/50s**
- ISO: Auto or 400-800
- Frame rate: 24fps or 30fps

### **DSLR/Mirrorless:**
- Shutter: **1/50s or 1/100s**
- Frame rate: 24p or 30p
- Enable: Long exposure noise reduction OFF

### **GoPro/Action Cam:**
- Mode: 24fps or 30fps (not 60fps or 120fps)
- Shutter: Auto (will match frame rate)
- Protune: ON, Shutter 1/50

---

## Testing Order

1. **Start with camera settings** (easiest, no code changes)
   - Try 1/50s shutter speed first
   - Use 24fps or 30fps recording

2. **If still flickering**, try config tweaks:
   - Set `MATRIX_GPIO_SLOWDOWN = 2`
   - Set `MATRIX_PWM_BITS = 11`
   - Set `MATRIX_PWM_DITHER_BITS = 1`

3. **If still flickering**, try extreme settings:
   - Set `MATRIX_REFRESH_LIMIT = 0`
   - Set `MATRIX_GPIO_SLOWDOWN = 1` (RPi 4 only, may be unstable on Zero 2 W)

4. **If STILL flickering**, hardware limitation:
   - Some cameras have fixed high shutter speeds
   - Consider external lighting to force slower shutter
   - Use ND filter to reduce light (forces slower shutter)

---

## Hardware-Specific Notes

### **Pi Zero 2 W:**
- `GPIO_SLOWDOWN = 3` is most stable
- `GPIO_SLOWDOWN = 2` is safe but test for glitches
- `GPIO_SLOWDOWN = 1` may cause flickering/instability
- Don't use `GPIO_SLOWDOWN = 0` (will glitch)

### **Pi 4 / CM4:**
- Can handle `GPIO_SLOWDOWN = 1` or `2` easily
- Use `2` for best balance

---

## Deploy Changes

After editing `config.py`:

```bash
# On Pi:
sudo systemctl restart led-matrix

# Check logs:
sudo journalctl -u led-matrix -n 50
```

Or from Mac (when Pi is online):

```bash
cd /Users/user/.openclaw/workspace/QSYS-LED-Matrix
bash deploy-optimizations.sh 10.1.1.21 node node
```

---

## Expected Results

| Setting | Refresh Rate | Flicker Reduction | Stability |
|---------|--------------|-------------------|-----------|
| SLOWDOWN=3, BITS=8 | ~200Hz | Moderate | Excellent |
| SLOWDOWN=2, BITS=11 | ~300Hz | Good | Good |
| SLOWDOWN=2, BITS=11, DITHER=1 | ~300Hz | Excellent | Good |
| SLOWDOWN=1, BITS=11, LIMIT=0 | ~500Hz+ | Best | Fair (Pi 4 only) |

---

## Why This Happens

**LED Matrix Refresh**: 
- LEDs turn on/off rapidly (PWM) to create colors
- Refresh rate: 200-500Hz (every 2-5ms)

**Camera Shutter**:
- Captures one frame every 8-42ms (24-120fps)
- If shutter is faster than LED refresh cycle, captures partial cycles
- Result: Rolling bands or flicker

**Solution**: Make sure camera captures **multiple full refresh cycles** per frame.

---

**Recommendation**: Start with **1/50s shutter speed on your camera** before changing matrix config.
