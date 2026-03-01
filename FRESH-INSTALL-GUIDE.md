# Fresh Raspbian Installation Guide

## 1. Prepare Fresh Raspbian OS

### Download & Flash
1. Get **Raspberry Pi OS Lite (64-bit)** - Bookworm recommended
2. Flash to SD card using Raspberry Pi Imager
3. **Before booting:**
   - Enable SSH (create empty `ssh` file in boot partition)
   - Configure WiFi (optional): create `wpa_supplicant.conf` in boot partition

### First Boot Setup
```bash
# Default credentials
# User: pi
# Pass: raspberry

# Update hostname (optional)
sudo raspi-config
# 1. System Options → S4 Hostname → "ledmatrix"

# Update system
sudo apt update && sudo apt upgrade -y

# Set timezone
sudo timedatectl set-timezone Europe/Vienna

# Expand filesystem (if not auto-expanded)
sudo raspi-config
# 6. Advanced Options → A1 Expand Filesystem
```

## 2. Clean OS Optimization

### Disable Unnecessary Services
```bash
# Disable Bluetooth (if not needed)
sudo systemctl disable bluetooth
sudo systemctl stop bluetooth

# Disable WiFi power management (prevents dropouts)
sudo iw wlan0 set power_save off
echo "iw wlan0 set power_save off" | sudo tee -a /etc/rc.local

# Disable unused services
sudo systemctl disable avahi-daemon
sudo systemctl disable triggerhappy
```

### Performance Tuning
Add to `/boot/firmware/config.txt`:
```ini
# GPU memory (LED matrix doesn't need much)
gpu_mem=16

# Overclock Pi Zero 2 W (optional, test stability)
arm_freq=1200
over_voltage=2

# Disable HDMI (saves power)
hdmi_blanking=2
```

### Install Basic Tools
```bash
sudo apt install -y vim htop git sshpass curl
```

## 3. Install LED Matrix Software

### Clone Repository
```bash
cd ~
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/rpi
```

### Run Installer
```bash
# Run WITHOUT sudo (script will prompt when needed)
bash install.sh
```

**Installation takes ~5-10 minutes:**
- ✅ Blacklists audio driver (conflicts with LED PWM)
- ✅ Installs dependencies (Python, Pillow, fonts)
- ✅ Clones and builds rpi-rgb-led-matrix library
- ✅ Installs Python bindings
- ✅ Copies application files to `/opt/led-matrix`
- ✅ Configures network fallback (10.20.30.40/24)
- ✅ Creates systemd service
- ✅ Starts service automatically

### Verify Installation
```bash
# Check service status
sudo systemctl status led-matrix

# View logs
sudo journalctl -u led-matrix -f

# Test UDP command
echo '{"cmd":"text","seg":0,"group":0,"text":"HELLO","color":"00FF00"}' | nc -u -w1 localhost 21324

# Access web UI
http://<pi-ip>:8080/
```

## 4. Recommended Post-Install

### Set Static IP (Optional)
```bash
# Via web UI: http://<pi-ip>:8080/network

# Or manually edit /etc/dhcpcd.conf:
sudo nano /etc/dhcpcd.conf

# Add:
interface eth0
static ip_address=10.1.1.99/24
static routers=10.1.1.1
static domain_name_servers=8.8.8.8

sudo systemctl restart dhcpcd
```

### Security Hardening
```bash
# Change default password
passwd

# Disable password SSH (use keys only)
sudo nano /etc/ssh/sshd_config
# Set: PasswordAuthentication no
sudo systemctl restart ssh

# Enable firewall
sudo apt install -y ufw
sudo ufw allow 22/tcp   # SSH
sudo ufw allow 8080/tcp # Web UI
sudo ufw allow 21324/udp # LED control
sudo ufw enable
```

### Backup Configuration
```bash
# Backup config
sudo cp /var/lib/led-matrix/config.json ~/config-backup.json

# Backup service file
sudo cp /etc/systemd/system/led-matrix.service ~/led-matrix.service.backup
```

## 5. Feature Extensions to Add

### Current Features
- ✅ 8-group segment management
- ✅ Text rendering with effects (scroll, fade, etc.)
- ✅ UDP control (Q-SYS integration)
- ✅ Web UI for configuration
- ✅ Layout presets
- ✅ Network configuration
- ✅ Auto-restart on rotation changes
- ✅ Brightness control (non-blocking in Python)

### Proposed New Features

#### A. Real-time Clock Display
- Auto-update clock segment every second
- Time/date formatting options
- Timezone support

#### B. Weather Display
- Fetch from OpenWeather API
- Show temp + icon
- Update every 10 minutes

#### C. Calendar/Event Integration
- Pull from Google Calendar API
- Show upcoming events
- Countdown timers

#### D. System Monitoring
- CPU temperature display
- Memory usage
- Uptime counter

#### E. Animation Library
- Pre-built animations (fire, rain, matrix, etc.)
- Trigger via UDP command
- Background effects

#### F. MQTT Support
- Subscribe/publish to topics
- Home automation integration
- IoT device control

#### G. Audio Visualization
- Mic input FFT visualization
- Spectrum analyzer bars
- Beat detection

#### H. QR Code Display
- Generate QR codes on-the-fly
- Display for WiFi credentials
- URL sharing

#### I. RSS Feed Ticker
- Scroll news headlines
- Multiple feed support
- Auto-refresh

#### J. Game Mode
- Snake game (UDP control)
- Pong
- Tetris

---

## Current Status

**Working:**
- Python-based LED matrix controller
- UDP commands (text, layout, clear, config)
- Web UI (port 8080)
- Q-SYS plugin v6.3.0 (frame auto-sync, reboot)

**Known Issues:**
- C++ version had brightness freeze (rpi-rgb-led-matrix library bug)
- Solution: Python version uses non-blocking `matrix.brightness` property

**Next Steps:**
1. Fresh Raspbian install
2. Run `install.sh`
3. Choose features to implement from the list above
