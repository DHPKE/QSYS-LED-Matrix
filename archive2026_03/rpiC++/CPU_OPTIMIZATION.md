# CPU Optimization Guide

## Understanding CPU Usage

The RGB LED Matrix library uses **~40-60% of one CPU core** by design. This is **normal and expected** behavior!

### Why So Much CPU?

The library constantly "bit-bangs" GPIO pins to refresh the LED panel:
- No hardware controller - pure software refresh
- Needs to update ~2048 LEDs at 150-300Hz
- Uses busy-waiting for precise timing
- Runs a dedicated refresh thread

**This is documented behavior** from the hzeller/rpi-rgb-led-matrix library.

---

## Optimization Strategies

### 1. CPU Isolation (Recommended) ✅

**Dedicate one CPU core** to the matrix refresh thread:

```bash
sudo nano /boot/cmdline.txt
```

Add `isolcpus=3` to the end of the line (keeps core 3 for LED matrix only).

**Reboot required:** `sudo reboot`

**Benefits:**
- Other processes won't be scheduled on core 3
- No interference from system tasks
- Smoother LED refresh
- Other cores remain fully available

### 2. Reduce PWM Bits (Quality vs Performance)

Current: `PWM_BITS = 11` (2048 brightness levels, high quality)

Lower options in `config.h`:
```cpp
#define PWM_BITS 9  // 512 levels, ~15% less CPU
#define PWM_BITS 7  // 128 levels, ~30% less CPU
```

**Trade-off:** Lower PWM bits = less smooth color gradients

### 3. Limit Refresh Rate (Already Optimized)

Current: `REFRESH_LIMIT = 200` Hz

```cpp
#define REFRESH_LIMIT 150  // Slightly less CPU, still smooth
```

### 4. Idle Optimization (Already Implemented) ✅

Our code already:
- Only renders when segments are dirty
- Sleeps 10ms when idle (vs 1ms when active)
- Updates effects at 10fps (not 60fps)
- Skips rendering unchanged frames

---

## Current Optimization Status

✅ **Already Optimized:**
- Dirty flag tracking (skip unchanged frames)
- 10fps effect updates (reduced from 20fps)
- Idle sleep extended to 10ms
- Render rate limited to 30fps max

⚠️ **Can't Optimize Further:**
- RGB matrix refresh thread (library behavior)
- GPIO bit-banging is inherently CPU-intensive
- This is the trade-off for software-driven displays

---

## Performance Breakdown

From `ps -eLo` output:

```
PID     TID   COMMAND     %CPU
3740    3740  led-matrix   0.2%   ← Main thread (idle when no effects)
3740    3743  led-matrix  51.4%   ← RGB matrix refresh (library, can't optimize)
3740    3744  led-matrix   0.0%   ← UDP listener
3740    3745  led-matrix   0.0%   ← Web server
```

**The 51% is the library's refresh thread** - not our code!

---

## Recommended Action

**Add CPU isolation** to `/boot/cmdline.txt`:

```bash
# Current line (example):
console=serial0,115200 console=tty1 root=PARTUUID=xxx-xx rootfstype=ext4 fsck.repair=yes rootwait

# Add isolcpus=3 at the end:
console=serial0,115200 console=tty1 root=PARTUUID=xxx-xx rootfstype=ext4 fsck.repair=yes rootwait isolcpus=3
```

Then reboot. This dedicates one core to the LED matrix and keeps the other cores free for system tasks.

---

## Alternative: Accept the CPU Usage

On a Pi 4/5 with 4 cores:
- **1 core** @ 50% = 12.5% total system CPU
- **3 cores** remain fully available
- This is perfectly acceptable for a dedicated LED controller

On a Pi Zero 2W (4 cores but slower):
- Use CPU isolation
- Consider reducing PWM bits to 9

---

## Bottom Line

**50% CPU usage is normal and expected.** The library needs it to refresh the display. Your options:

1. **Isolate the core** (best solution) - `isolcpus=3` in `/boot/cmdline.txt`
2. **Reduce PWM bits** (slightly lower quality) - edit `config.h`
3. **Accept it** (perfectly fine on multi-core Pi)

The controller itself (our code) uses <1% CPU. The library's refresh thread uses the rest.
