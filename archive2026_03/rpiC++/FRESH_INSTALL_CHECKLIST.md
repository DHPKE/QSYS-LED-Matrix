# Fresh System Installation Checklist

## Before You Start

### ✅ Required Files (Run pre-check)
```bash
./pre-install-check.sh
```

Should show all files present:
- Core sources (main.cpp, segment_manager.*, udp_handler.*, text_renderer.*, web_server.*)
- Headers (config.h, *.h)
- Build system (Makefile)
- Services (led-matrix.service, led-matrix-network.service)
- Scripts (install.sh, apply-network-config.sh)

### ✅ Hardware Requirements
- [ ] Raspberry Pi (Zero 2W, 3, 4, or 5)
- [ ] HUB75 RGB LED panel (default: 64×32)
- [ ] Power supply (5V, 4A+ recommended)
- [ ] Ethernet cable (for installation)

### ✅ Software Requirements
- [ ] Raspbian Lite 64-bit installed
- [ ] Internet connection working
- [ ] SSH access enabled (or keyboard/monitor)

---

## Installation Steps

### 1. Transfer Files to Pi
```bash
# From your computer:
scp -r rpiC++/ pi@<pi-ip>:/home/pi/
```

### 2. Run Pre-Check
```bash
ssh pi@<pi-ip>
cd rpiC++
./pre-install-check.sh
```

### 3. Run Installer
```bash
sudo ./install.sh
```

**Or use QUICK_START:**
```bash
./QUICK_START.sh
```

### 4. Answer Prompts
- **Rebuild?** (if binary exists) → Usually `y`
- **CPU isolation?** → `y` for Pi 4/5, `n` for Pi Zero
- **Reboot now?** (if audio was disabled) → `y`
- **Start service?** → `y`
- **Send test?** → `y`

### 5. Validate Installation
```bash
sudo ./validate-install.sh
```

Should show all checks passing.

---

## Installation Time

| Pi Model | Estimated Time |
|----------|----------------|
| Pi Zero 2W | 15-20 minutes |
| Pi 3 | 10-15 minutes |
| Pi 4 | 5-10 minutes |
| Pi 5 | 3-5 minutes |

Most time is spent building the RGB matrix library.

---

## Post-Installation

### ✅ Verify Service Running
```bash
sudo systemctl status led-matrix
```

Should show: `Active: active (running)`

### ✅ Check IP Display
The LED panel should show the Pi's IP address.

### ✅ Test UDP Protocol
```bash
IP=$(hostname -I | awk '{print $1}')
echo '{"cmd":"text","seg":0,"text":"HELLO","color":"00FF00"}' | nc -u -w1 $IP 21324
```

LED panel should show "HELLO" in green.

### ✅ Access Web UI
Open in browser: `http://<pi-ip>:8080`

Should show network config interface.

### ✅ Run Full Test Suite
```bash
./test-commands.sh $(hostname -I | awk '{print $1}')
```

Should run 11 tests successfully.

---

## Known Issues Fixed

All these are **already fixed** in the current version:

✅ Missing `<cstdint>` include → Added to config.h  
✅ Incorrect `#endif` in .cpp files → Removed  
✅ Static linkage of globals → Made non-static  
✅ Boost dependencies → Removed  
✅ RGB matrix install target → Manual copy implemented  
✅ No fonts installed → Added fonts-dejavu-core  
✅ High CPU usage → Documented + idle optimization  

---

## If Installation Fails

### Check logs:
```bash
# Installation output
tail -100 /var/log/led-matrix-install.log

# Service logs
sudo journalctl -u led-matrix -n 50
```

### Common issues:

**Build fails:**
- Missing dependency → Re-run `sudo apt update && sudo apt upgrade`
- Out of disk space → Check with `df -h`
- Network timeout → Check internet connection

**Service won't start:**
- Check logs: `sudo journalctl -u led-matrix -n 50`
- Run manually: `sudo /usr/local/bin/led-matrix`
- Check permissions: `ls -la /usr/local/bin/led-matrix`

**No network:**
- Check cable
- Check `/boot/config.txt` for `dtparam=audio=off`
- Try fallback IP in `config.h`

---

## Rollback

If something goes wrong:

```bash
sudo ./uninstall.sh
```

Then fix the issue and re-run `install.sh`.

---

## Success Criteria

All these should be true:

✅ `sudo systemctl status led-matrix` shows active  
✅ LED panel displays IP address  
✅ UDP test command changes display  
✅ Web UI accessible at http://<ip>:8080  
✅ Service survives reboot  
✅ Network config persists  

If all pass → **Installation successful!** 🎉

---

## Next Steps

1. **Configure network** (if needed) → http://<ip>:8080
2. **Connect QSYS** → Send commands to `<ip>:21324`
3. **Monitor logs** → `sudo journalctl -u led-matrix -f`
4. **Optimize CPU** (optional) → See `CPU_OPTIMIZATION.md`

See `DEPLOYMENT_SUCCESS.md` for full deployment report.
