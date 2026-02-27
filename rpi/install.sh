#!/bin/bash
# install.sh — Install the LED Matrix controller on a Raspberry Pi Zero 2 W
#              running Raspberry Pi OS (Bookworm / Bullseye)
#
# Run WITHOUT sudo:  bash install.sh
# (Script will prompt for sudo when needed)
#
# What this does:
#   0. Pre-flight checks (hardware detection, OS check)
#   1. Blacklist snd_bcm2835 (conflicts with rpi-rgb-led-matrix PWM)
#   2. Install system packages (Python 3, Pillow, fonts, build tools)
#   3. Clone and build rpi-rgb-led-matrix library
#   4. Install Python bindings
#   5. Copy app files to /opt/led-matrix
#   6. Configure network fallback IP (dhcpcd.conf)
#   7. Install network configuration helper script
#   8. Configure sudoers for daemon user
#   9. Install and enable the systemd service

set -e

# Capture script directory FIRST — before any cd commands change $PWD.
# ${BASH_SOURCE[0]} is the path to this script file regardless of CWD.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================="
echo "RPi Zero 2 W LED Matrix - Installation"
echo "========================================="
echo ""

# ── Check NOT running as root ──────────────────────────────────────────────
if [ "$EUID" -eq 0 ]; then
  echo "ERROR: Do not run this script as root (don't use sudo)"
  echo "The script will prompt for sudo when needed"
  exit 1
fi

# ── 0. Pre-flight ──────────────────────────────────────────────────────────
echo "[0/9] Pre-flight checks..."

# Detect hardware model
if [ -f /proc/device-tree/model ]; then
    MODEL=$(cat /proc/device-tree/model 2>/dev/null | tr -d '\0' || echo "unknown")
    echo "  Detected: $MODEL"
else
    echo "  ⚠  Cannot detect hardware model"
fi

# Warn if not Raspberry Pi OS
if [ ! -f /etc/rpi-issue ] && ! grep -qi "raspberry" /etc/os-release 2>/dev/null; then
  echo "  ⚠  This does not appear to be a Raspberry Pi OS system."
  echo "     The script is written for RPi OS Bookworm/Bullseye."
  read -p "  Continue anyway? [y/N] " confirm
  [[ "$confirm" =~ ^[Yy]$ ]] || exit 1
fi

echo "  ✓ Pre-flight passed"
echo ""

# ── 1. Blacklist snd_bcm2835 ───────────────────────────────────────────────
# rpi-rgb-led-matrix uses the BCM PWM hardware for OE- timing.
# The Pi on-board sound driver (snd_bcm2835) claims the same peripheral and
# causes the library to hard-exit unless disable_hardware_pulsing is set.
# Blacklisting the module allows hardware-timed (better quality) OE-.
echo "[1/9] Blacklisting snd_bcm2835 (on-board sound conflicts with LED matrix PWM)..."
cat <<'EOF' | sudo tee /etc/modprobe.d/blacklist-rgb-matrix.conf > /dev/null
# Blacklisted by led-matrix install.sh — conflicts with rpi-rgb-led-matrix PWM
blacklist snd_bcm2835
EOF
# Unload immediately if already loaded (best-effort, may fail if in use)
sudo modprobe -r snd_bcm2835 2>/dev/null || true
# Comment out dtparam=audio=on in /boot/firmware/config.txt (Bookworm path)
for cfg in /boot/firmware/config.txt /boot/config.txt; do
  if [ -f "$cfg" ]; then
    sudo sed -i 's/^\(dtparam=audio=on\)/#\1  # disabled by led-matrix install/' "$cfg"
    echo "  ✓ Commented out dtparam=audio=on in $cfg"
    break
  fi
done
# Regenerate initramfs so the blacklist takes effect after reboot
if command -v update-initramfs &>/dev/null; then
  sudo update-initramfs -u -k all 2>/dev/null || true
fi
echo "  ✓ snd_bcm2835 blacklisted (reboot to fully apply)"
echo ""

# ── 2. System packages ─────────────────────────────────────────────────────
echo "[2/9] Installing system packages..."
sudo apt-get update -qq
sudo apt-get install -y \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-pillow \
    fonts-dejavu-core \
    git \
    build-essential \
    cython3 \
    libgraphicsmagick++-dev \
    libwebp-dev
echo "  ✓ Dependencies installed"
echo ""

# ── 3. Clone / update rpi-rgb-led-matrix ──────────────────────────────────
echo "[3/9] Installing rpi-rgb-led-matrix library..."

if [ -d ~/rpi-rgb-led-matrix ]; then
    echo "  ℹ  Library already exists at ~/rpi-rgb-led-matrix"
    
    # Check if it's a valid git repo
    if [ -d ~/rpi-rgb-led-matrix/.git ]; then
        read -p "  Update existing library? [y/N]: " UPDATE_LIB
        if [[ "$UPDATE_LIB" =~ ^[Yy]$ ]]; then
            cd ~/rpi-rgb-led-matrix
            git pull
            echo "  ✓ Library updated"
        fi
    else
        echo "  ⚠  Directory exists but is not a git repository"
        read -p "  Remove and reclone? [y/N]: " RECLONE
        if [[ "$RECLONE" =~ ^[Yy]$ ]]; then
            cd ~
            sudo rm -rf rpi-rgb-led-matrix
            git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
            echo "  ✓ Library cloned"
        else
            echo "  Keeping existing directory"
        fi
    fi
else
    echo "  Cloning rpi-rgb-led-matrix library..."
    cd ~
    git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
    echo "  ✓ Library cloned"
fi
echo ""

# ── 4. Compile library + Python bindings ──────────────────────────────────
echo "[4/9] Compiling library (takes 2-3 minutes on Pi Zero 2 W)..."
cd ~/rpi-rgb-led-matrix

# Ensure we're in the right place
if [ ! -f "lib/Makefile" ]; then
    echo "  ERROR: Library source files not found"
    exit 1
fi

# Verify Python binding sources exist BEFORE building
echo "  Checking Python binding sources..."
if [ ! -f "bindings/python/rgbmatrix/core.cpp" ]; then
    echo "  ERROR: Python binding source files not found in cloned repo"
    echo "  Files in bindings/python/:"
    ls -la bindings/python/ || true
    echo "  Files in bindings/python/rgbmatrix/:"
    ls -la bindings/python/rgbmatrix/ || true
    echo ""
    echo "  Removing and recloning..."
    cd ~
    sudo rm -rf rpi-rgb-led-matrix
    git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
    cd ~/rpi-rgb-led-matrix
    
    # Verify again after reclone
    if [ ! -f "bindings/python/rgbmatrix/core.cpp" ]; then
        echo "  ERROR: Python bindings still missing after reclone"
        echo "  There may be a network issue or repository problem"
        exit 1
    fi
fi
echo "  ✓ Python binding sources verified"

make clean
# Use 'regular' hardware mapping (matches MATRIX_HARDWARE_MAPPING in config.py)
make -j"$(nproc)"
echo "  ✓ Library compiled"

echo "  Installing Python bindings..."
cd ~/rpi-rgb-led-matrix/bindings/python

# Debug: show current directory and file structure
echo "  Current directory: $(pwd)"
echo "  Checking for core.cpp..."
if [ -f "rgbmatrix/core.cpp" ]; then
    echo "  ✓ core.cpp exists"
    ls -la rgbmatrix/core.cpp
else
    echo "  ✗ core.cpp NOT FOUND"
    echo "  Files in rgbmatrix/:"
    ls -la rgbmatrix/ || echo "    Directory doesn't exist!"
    exit 1
fi

# Clean any previous build artifacts (use sudo in case they're root-owned)
sudo python3 setup.py clean --all 2>/dev/null || true
sudo rm -rf build dist *.egg-info 2>/dev/null || true

# Try direct setup.py install instead of make (bypasses Makefile issues)
echo "  Building Python module..."
sudo python3 setup.py build
echo "  Installing Python module..."
sudo python3 setup.py install
echo "  ✓ Python bindings installed"
echo ""

# ── 5. Copy app files ──────────────────────────────────────────────────────
echo "[5/9] Copying application files to /opt/led-matrix..."
sudo mkdir -p /opt/led-matrix
sudo cp "$SCRIPT_DIR"/*.py /opt/led-matrix/
echo "  ✓ Files copied"
echo ""

# ── 6. Configure network fallback IP ──────────────────────────────────────
echo "[6/9] Configuring network fallback IP (dhcpcd.conf)..."

# Backup dhcpcd.conf if not already backed up
if [ -f /etc/dhcpcd.conf ] && [ ! -f /etc/dhcpcd.conf.backup ]; then
    sudo cp /etc/dhcpcd.conf /etc/dhcpcd.conf.backup
    echo "  ✓ Created backup: /etc/dhcpcd.conf.backup"
fi

# Check if fallback profile already exists
if sudo grep -q "profile static_fallback" /etc/dhcpcd.conf 2>/dev/null; then
    echo "  ℹ  Fallback IP profile already configured in dhcpcd.conf"
else
    echo "  Adding fallback IP configuration..."
    cat <<'DHCPCD_EOF' | sudo tee -a /etc/dhcpcd.conf > /dev/null

# LED Matrix fallback static IP (added by install.sh)
# Applied automatically if DHCP fails on eth0
profile static_fallback
static ip_address=10.20.30.40/24
static routers=10.20.30.1
static domain_name_servers=8.8.8.8

interface eth0
fallback static_fallback
DHCPCD_EOF
    echo "  ✓ Fallback IP configured: 10.20.30.40/24"
fi
echo ""

# ── 7. Install network configuration helper ──────────────────────────────
echo "[7/9] Installing network configuration helper..."

# Copy network-config.sh helper
if [ -f "$SCRIPT_DIR/network-config.sh" ]; then
    sudo cp "$SCRIPT_DIR/network-config.sh" /opt/led-matrix/
    sudo chmod +x /opt/led-matrix/network-config.sh
    echo "  ✓ network-config.sh installed"
else
    echo "  ⚠  network-config.sh not found in source directory"
    echo "  Creating default network-config.sh..."
    
    cat <<'NETCONFIG_EOF' | sudo tee /opt/led-matrix/network-config.sh > /dev/null
#!/bin/bash
# Network configuration helper for LED Matrix WebUI
# Called via sudo by daemon user to modify network settings

set -e

MODE="$1"
IP="$2"
SUBNET="${3:-24}"

case "$MODE" in
    dhcp)
        # Remove static configuration, enable DHCP
        sudo sed -i '/^interface eth0$/,/^$/d' /etc/dhcpcd.conf
        echo "DHCP enabled on eth0"
        sudo systemctl restart dhcpcd
        ;;
    static)
        # Validate inputs
        if [[ ! "$IP" =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
            echo "Error: Invalid IP address format"
            exit 1
        fi
        if [[ ! "$SUBNET" =~ ^[0-9]{1,2}$ ]] || [ "$SUBNET" -lt 8 ] || [ "$SUBNET" -gt 30 ]; then
            echo "Error: Invalid subnet (must be 8-30)"
            exit 1
        fi
        
        # Remove existing eth0 configuration
        sudo sed -i '/^interface eth0$/,/^$/d' /etc/dhcpcd.conf
        
        # Add new static configuration
        cat << EOF | sudo tee -a /etc/dhcpcd.conf > /dev/null

interface eth0
static ip_address=${IP}/${SUBNET}
EOF
        echo "Static IP configured: ${IP}/${SUBNET}"
        sudo systemctl restart dhcpcd
        ;;
    *)
        echo "Usage: $0 {dhcp|static} [ip] [subnet]"
        exit 1
        ;;
esac
NETCONFIG_EOF

    sudo chmod +x /opt/led-matrix/network-config.sh
    echo "  ✓ Default network-config.sh created"
fi
echo ""

# ── 8. Configure sudoers for daemon user ──────────────────────────────────
echo "[8/9] Configuring sudoers for network configuration..."

SUDOERS_FILE="/etc/sudoers.d/led-matrix"

if [ -f "$SUDOERS_FILE" ]; then
    echo "  ℹ  Sudoers configuration already exists"
else
    echo "daemon ALL=(ALL) NOPASSWD: /opt/led-matrix/network-config.sh" | sudo tee "$SUDOERS_FILE" > /dev/null
    sudo chmod 0440 "$SUDOERS_FILE"
    echo "  ✓ Sudoers configured for daemon user"
fi
echo ""

# ── 9. Systemd service ─────────────────────────────────────────────────────
echo "[9/9] Installing systemd service..."
sudo mkdir -p /var/lib/led-matrix
sudo cp "$SCRIPT_DIR/led-matrix.service" /etc/systemd/system/led-matrix.service
sudo systemctl daemon-reload
sudo systemctl enable led-matrix.service

# Remove NO_DISPLAY override if present from a previous test install
if [ -f /etc/systemd/system/led-matrix.service.d/override.conf ]; then
    echo "  Removing NO_DISPLAY override..."
    sudo rm /etc/systemd/system/led-matrix.service.d/override.conf
    sudo rmdir /etc/systemd/system/led-matrix.service.d 2>/dev/null || true
    sudo systemctl daemon-reload
fi

sudo systemctl start led-matrix.service
echo "  ✓ Service installed and started"

echo ""
echo "========================================="
echo "Installation complete!"
echo "========================================="
echo ""
echo "Features configured:"
echo "  ✅ LED Matrix display driver"
echo "  ✅ UDP control (port 21324)"
echo "  ✅ Web UI (port 8080)"
echo "  ✅ Network fallback IP: 10.20.30.40/24"
echo "  ✅ Network configuration via WebUI"
echo "  ✅ Dynamic IP display on panel"
echo ""
echo "Current stable configuration (config.py):"
echo "  - gpio_slowdown: 2 (balanced ~250-300Hz refresh)"
echo "  - PWM bits: 7 (128 color levels)"
echo "  - Scan mode: 0 (progressive)"
echo "  - Font threshold: 128 (sharp, no anti-aliasing)"
echo "  - Hardware: RPi 4 / RPi Zero 2 W compatible"
echo ""
echo "Network behavior:"
echo "  - Panel shows IP address on startup"
echo "  - Automatically updates if IP changes (DHCP)"
echo "  - Fallback to 10.20.30.40 if no DHCP available"
echo ""
echo "Commands:"
echo "  Status:    sudo systemctl status led-matrix"
echo "  Logs:      sudo journalctl -u led-matrix -f"
echo "  Stop:      sudo systemctl stop led-matrix"
echo "  Restart:   sudo systemctl restart led-matrix"
echo ""
echo "Access:"
echo "  Web UI:    http://$(hostname -I | awk '{print $1}'):8080/"
echo "  UDP:       Send JSON to port 21324"
echo ""
echo "NOTE: A reboot is recommended to fully apply:"
echo "      - snd_bcm2835 blacklist (hardware PWM)"
echo "      - Network fallback configuration"
echo ""
echo "Reboot now? [y/N]"
read -r REBOOT_CONFIRM
if [[ "$REBOOT_CONFIRM" =~ ^[Yy]$ ]]; then
    echo "Rebooting in 5 seconds... (Ctrl+C to cancel)"
    sleep 5
    sudo reboot
fi
echo ""
