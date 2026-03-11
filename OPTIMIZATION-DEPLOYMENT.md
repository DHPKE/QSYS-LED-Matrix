# Code Optimization - Deployment Guide

**Date**: 2026-03-11 23:59 CET  
**Target**: Raspberry Pi CM4 running LED Matrix controller  
**Goal**: Reduce CPU usage by 15-25%, improve responsiveness

---

## Pre-Deployment Checklist

- [ ] Pi is accessible (ssh node@10.1.1.21)
- [ ] Current code is backed up
- [ ] Baseline performance measured
- [ ] Service can be restarted without issues

---

## Baseline Measurement

### On the Pi:

```bash
# Measure current CPU usage (60 seconds)
top -b -n 60 -d 1 | grep python3 | tee ~/cpu-baseline.log

# Calculate average
awk '{sum+=$9; count++} END {print "Average CPU:", sum/count "%"}' ~/cpu-baseline.log

# Check current render performance
sudo journalctl -u led-matrix --since "1 hour ago" | grep -E "RENDER|PERF" | tail -20
```

**Expected baseline**: ~40% CPU, occasional render lag on font changes

---

## Phase 1: Quick Wins (15 minutes)

### Files to Update:
1. `rpi/main.py`
2. `rpi/text_renderer.py`

### Steps:

```bash
# On your Mac:
cd /Users/user/.openclaw/workspace/QSYS-LED-Matrix

# Create backup
git stash  # Save any uncommitted changes
git checkout -b feature/performance-optimization

# Apply Phase 1 optimizations (see OPTIMIZATION-PATCHES.md)
# Edit rpi/main.py:
#   - Add test_mode_poll_counter (line ~545)
#   - Add watchdog_check_counter (line ~545)
#   - Modify test mode polling (line ~562)
#   - Modify network monitor (line ~197)
#   - Modify watchdog check (line ~733)
#   - Improve render timing (line ~759)

# Edit rpi/text_renderer.py:
#   - Prepopulate color cache (line ~105)

# Commit changes
git add rpi/main.py rpi/text_renderer.py
git commit -m "perf: Phase 1 optimizations (test mode, network, watchdog)"
```

### Deploy to Pi:

```bash
# From Mac:
cd /Users/user/.openclaw/workspace/QSYS-LED-Matrix

# Copy optimized files
sshpass -p 'node' scp rpi/main.py node@10.1.1.21:/tmp/main.py.optimized
sshpass -p 'node' scp rpi/text_renderer.py node@10.1.1.21:/tmp/text_renderer.py.optimized

# On Pi (via SSH):
ssh node@10.1.1.21

# Backup current files
sudo cp /opt/led-matrix/main.py /opt/led-matrix/main.py.backup-20260311
sudo cp /opt/led-matrix/text_renderer.py /opt/led-matrix/text_renderer.py.backup-20260311

# Deploy optimized versions
sudo cp /tmp/main.py.optimized /opt/led-matrix/main.py
sudo cp /tmp/text_renderer.py.optimized /opt/led-matrix/text_renderer.py

# Restart service
sudo systemctl restart led-matrix

# Verify it started
sudo systemctl status led-matrix

# Watch logs
sudo journalctl -u led-matrix -f
```

### Verify Phase 1:

```bash
# Should see these new log messages:
[FONT] Prewarmed X fonts in X.Xms  # (if font prewarming added)
[NET-MON] No IP change, backing off to X.Xs  # (network backoff)
[RENDER] Slow render: X.Xms  # (if render > 50ms)

# Measure new CPU usage
top -b -n 60 -d 1 | grep python3 | tee ~/cpu-phase1.log
awk '{sum+=$9; count++} END {print "Average CPU:", sum/count "%"}' ~/cpu-phase1.log
```

**Expected improvement**: 5-10% CPU reduction

---

## Phase 2: Font & Memory (30 minutes)

### Additional Changes:

```bash
# On Mac, continue editing:

# Edit rpi/text_renderer.py:
#   - Add _prewarm_font_cache() method
#   - Add @lru_cache for _measure_text_cached()
#   - Update _fit_text() to use LRU cache

# Commit
git add rpi/text_renderer.py
git commit -m "perf: Phase 2 - font prewarming and LRU cache"
```

### Deploy:

```bash
# Copy and deploy (same process as Phase 1)
sshpass -p 'node' scp rpi/text_renderer.py node@10.1.1.21:/tmp/text_renderer.py.phase2

# On Pi:
sudo cp /tmp/text_renderer.py.phase2 /opt/led-matrix/text_renderer.py
sudo systemctl restart led-matrix
```

### Verify Phase 2:

```bash
# Should see:
[FONT] Prewarmed 20 fonts in 50.0ms  # Font cache ready

# Test first render (should be smooth, no lag)
# Send command via UDP or Q-SYS plugin

# Measure CPU
top -b -n 60 -d 1 | grep python3 | tee ~/cpu-phase2.log
awk '{sum+=$9; count++} END {print "Average CPU:", sum/count "%"}' ~/cpu-phase2.log
```

**Expected improvement**: Additional 5-10% CPU reduction, no render lag

---

## Phase 3: Advanced (1 hour, optional)

### PIL Image Reuse:

**⚠️ Warning**: This is more invasive. Test thoroughly!

```bash
# Edit rpi/text_renderer.py:
#   - Add self._reusable_image in __init__
#   - Modify _render_segment_from_snapshot() to reuse image

# Commit
git add rpi/text_renderer.py
git commit -m "perf: Phase 3 - PIL image buffer reuse"
```

### Deploy & Test:

```bash
# Deploy (same process)
# Test extensively:
#   - All layouts (1-14)
#   - Test mode
#   - Multiple segments
#   - Curtain mode
#   - Frame rendering
```

**Expected improvement**: Additional 5-10% CPU, 70% less memory allocations

---

## Rollback Procedure

### If something breaks:

```bash
# On Pi:
sudo cp /opt/led-matrix/main.py.backup-20260311 /opt/led-matrix/main.py
sudo cp /opt/led-matrix/text_renderer.py.backup-20260311 /opt/led-matrix/text_renderer.py
sudo systemctl restart led-matrix

# Verify
sudo systemctl status led-matrix
```

### On Mac:

```bash
cd /Users/user/.openclaw/workspace/QSYS-LED-Matrix
git checkout main  # Or previous stable branch
```

---

## Performance Comparison

### Before vs After:

| Metric | Baseline | Phase 1 | Phase 2 | Phase 3 | Total Improvement |
|--------|----------|---------|---------|---------|-------------------|
| CPU Usage | ~40% | ~35% | ~30% | ~25-28% | 12-15% reduction |
| File I/O | 60/sec | 6/sec | 6/sec | 6/sec | 90% reduction |
| Network Wake-ups | 12/min | 2-4/min | 2-4/min | 2-4/min | 66-83% reduction |
| First Render Lag | Noticeable | Noticeable | None | None | Eliminated |
| Memory Allocs | High | High | Medium | Low | 70% reduction |

---

## Long-Term Monitoring

### Add to crontab (on Pi):

```bash
# Monitor CPU usage daily
0 * * * * top -b -n 60 -d 1 | grep python3 >> /var/log/led-matrix-cpu.log

# Rotate logs weekly
0 0 * * 0 mv /var/log/led-matrix-cpu.log /var/log/led-matrix-cpu.log.old
```

### Check for issues:

```bash
# Memory leaks (should stay constant):
ps aux | grep python3 | grep led-matrix

# CPU over time (should be stable):
tail -100 /var/log/led-matrix-cpu.log | awk '{sum+=$9; count++} END {print sum/count}'
```

---

## Success Criteria

- [ ] CPU usage reduced by 10-15%
- [ ] No render lag on font changes
- [ ] Test mode still works
- [ ] Network monitor stops after first command
- [ ] Watchdog blanking still works
- [ ] No memory leaks after 24 hours
- [ ] All layouts render correctly

---

## Troubleshooting

### High CPU after optimization:

```bash
# Check for infinite loops
sudo journalctl -u led-matrix --since "10 minutes ago" | grep -E "ERROR|WARNING"

# Revert to baseline
sudo cp /opt/led-matrix/main.py.backup-20260311 /opt/led-matrix/main.py
sudo systemctl restart led-matrix
```

### Rendering artifacts:

```bash
# Likely PIL image reuse issue (Phase 3)
# Revert text_renderer.py to Phase 2 version
sudo cp /opt/led-matrix/text_renderer.py.backup-20260311 /opt/led-matrix/text_renderer.py
sudo systemctl restart led-matrix
```

### Service won't start:

```bash
# Check syntax errors
python3 -m py_compile /opt/led-matrix/main.py
python3 -m py_compile /opt/led-matrix/text_renderer.py

# Check logs
sudo journalctl -u led-matrix -n 50
```

---

## Documentation Updates

After successful deployment:

- [ ] Update PROJECT-STATUS.md with new performance metrics
- [ ] Update README.md with optimization notes
- [ ] Tag release v7.2.0 (if stable)
- [ ] Update deployment documentation

---

## Next Steps

1. **Deploy Phase 1** (safest, biggest impact)
2. **Measure improvement**
3. **Deploy Phase 2** (if Phase 1 successful)
4. **Measure improvement**
5. **Consider Phase 3** (if more performance needed)
6. **Document final results**

---

**Status**: Ready for deployment  
**Risk Level**: Low (Phase 1), Medium (Phase 2), Medium-High (Phase 3)  
**Estimated Time**: 15-60 minutes depending on phase  
**Reversible**: Yes (backups created automatically)
