#!/bin/bash
# install.sh — Install the LED Matrix controller on Raspberry Pi
#              Supports: Raspberry Pi OS (Bookworm/Bullseye) and DietPi
#
# Version 7.1.0 includes:
#   - NetworkManager fallback IP configuration (10.10.10.99/24)
#   - DietPi compatibility (dietpi-software integration)
#   - Improved error handling and verification
#   - Anti-flicker optimizations (PWM_BITS=5)
#   - Text padding (1px margins)
#
# Run WITHOUT sudo:  bash install.sh
# (Script will prompt for sudo when needed)

set -e

# Capture script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================="
echo "LED Matrix Controller - Installation"
echo "========================================="
echo ""

# ── Check NOT running as root ──────────────────────────────────────────────
if [ "$EUID" -eq 0 ]; then
  echo "ERROR: Do not run this script as root (don't use sudo)"
  echo "The script will prompt for sudo when needed"
  exit 1
fi

# ── 0. Pre-flight ──────────────────────────────────────────────────────────
echo "[0/10] Pre-flight checks..."

# Detect hardware model
if [ -f /proc/device-tree/model ]; then
    MODEL=$(cat /proc/device-tree/model 2>/dev/null | tr -d '\0' || echo "unknown")
    echo "  Detected: $MODEL"
else
    echo "  ⚠  Cannot detect hardware model"
fi

# Detect OS (Raspberry Pi OS vs DietPi)
if [ -f /boot/dietpi/.version ]; then
    OS_TYPE="DietPi"
    echo "  OS: DietPi"
elif [ -f /etc/rpi-issue ] || grep -qi "raspberry" /etc/os-release 2>/dev/null; then
    OS_TYPE="RaspberryPiOS"
    echo "  OS: Raspberry Pi OS"
else
    echo "  ⚠  Unknown OS - attempting generic Debian-based install"
    OS_TYPE="Generic"
fi

# Check for NetworkManager (modern) vs dhcpcd (legacy)
if systemctl is-active --quiet NetworkManager; then
    NETWORK_MGR="NetworkManager"
    echo "  Network: NetworkManager"
elif systemctl is-active --quiet dhcpcd; then
    NETWORK_MGR="dhcpcd"
    echo "  Network: dhcpcd"
else
    NETWORK_MGR="unknown"
    echo "  ⚠  No network manager detected"
fi

echo "  ✓ Pre-flight passed"
echo ""

# ── 1. Blacklist snd_bcm2835 ───────────────────────────────────────────────
echo "[1/10] Blacklisting snd_bcm2835 (conflicts with LED PWM)..."
cat <<'EOF' | sudo tee /etc/modprobe.d/blacklist-rgb-matrix.conf > /dev/null
# Blacklisted by led-matrix install.sh — conflicts with rpi-rgb-led-matrix PWM
blacklist snd_bcm2835
EOF
sudo modprobe -r snd_bcm2835 2>/dev/null || true

# Comment out audio in config.txt (try both paths)
for cfg in /boot/firmware/config.txt /boot/config.txt /DietPi/config.txt; do
  if [ -f "$cfg" ]; then
    sudo sed -i 's/^\(dtparam=audio=on\)/#\1  # disabled by led-matrix install/' "$cfg"
    echo "  ✓ Disabled audio in $cfg"
    break
  fi
done

# Regenerate initramfs if available
if command -v update-initramfs &>/dev/null; then
  sudo update-initramfs -u -k all 2>/dev/null || true
fi
echo "  ✓ Audio driver blacklisted (reboot to apply)"
echo ""

# ── 2. System packages ─────────────────────────────────────────────────────
echo "[2/10] Installing system packages..."

# Update package list
sudo apt-get update -qq

# Core dependencies
sudo apt-get install -y \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-pillow \
    python3-numpy \
    fonts-dejavu-core \
    git \
    build-essential \
    cython3 \
    libgraphicsmagick++-dev \
    libwebp-dev \
    sshpass

# Install NetworkManager if not present (for fallback IP)
if [ "$NETWORK_MGR" = "unknown" ] || [ "$NETWORK_MGR" = "dhcpcd" ]; then
    echo "  Installing NetworkManager for fallback IP support..."
    sudo apt-get install -y network-manager
    
    # Disable dhcpcd if present
    if systemctl is-active --quiet dhcpcd; then
        sudo systemctl stop dhcpcd
        sudo systemctl disable dhcpcd
    fi
    
    # Enable and start NetworkManager
    sudo systemctl enable NetworkManager
    sudo systemctl start NetworkManager
    NETWORK_MGR="NetworkManager"
    echo "  ✓ NetworkManager installed and enabled"
fi

echo "  ✓ Dependencies installed"
echo ""

# ── 3. Clone / update rpi-rgb-led-matrix ──────────────────────────────────
echo "[3/10] Installing rpi-rgb-led-matrix library..."

if [ -d ~/rpi-rgb-led-matrix ]; then
    echo "  ℹ  Library already exists"
    
    if [ -d ~/rpi-rgb-led-matrix/.git ]; then
        cd ~/rpi-rgb-led-matrix
        echo "  Updating library..."
        git pull || true
    else
        echo "  ⚠  Existing directory is not a git repo - recloning..."
        cd ~
        sudo rm -rf rpi-rgb-led-matrix
        git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
    fi
else
    echo "  Cloning library..."
    cd ~
    git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
fi
echo "  ✓ Library ready"
echo ""

# ── 4. Compile library + Python bindings ──────────────────────────────────
echo "[4/10] Compiling library (~3 min on Pi Zero 2 W)..."
cd ~/rpi-rgb-led-matrix

# Verify source files
if [ ! -f "lib/Makefile" ]; then
    echo "  ERROR: Library source files not found"
    exit 1
fi

# Check Python binding sources
if [ ! -f "bindings/python/rgbmatrix/core.pyx" ]; then
    echo "  ERROR: Python binding sources missing - recloning..."
    cd ~
    sudo rm -rf rpi-rgb-led-matrix
    git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
    cd ~/rpi-rgb-led-matrix
    
    if [ ! -f "bindings/python/rgbmatrix/core.pyx" ]; then
        echo "  ERROR: Python bindings still missing after reclone"
        exit 1
    fi
fi
echo "  ✓ Sources verified"

# Clean and compile
make clean
make -j"$(nproc)"
echo "  ✓ Library compiled"

# Install Python bindings
echo "  Installing Python bindings..."
cd ~/rpi-rgb-led-matrix/bindings/python

# Verify core.pyx exists
if [ ! -f "rgbmatrix/core.pyx" ]; then
    echo "  ERROR: core.pyx not found"
    ls -la rgbmatrix/ || echo "  rgbmatrix/ directory missing!"
    exit 1
fi

# Clean previous builds
sudo python3 setup.py clean --all 2>/dev/null || true
sudo rm -rf build dist *.egg-info 2>/dev/null || true

# Build and install
sudo python3 setup.py build
sudo python3 setup.py install

# Verify installation
echo "  Verifying Python module..."
python3 -c "from rgbmatrix import RGBMatrix, RGBMatrixOptions; print('✓ rgbmatrix module installed')" || {
    echo "  ERROR: Python module verification failed"
    exit 1
}
echo "  ✓ Python bindings installed"
echo ""

# ── 5. Copy app files ──────────────────────────────────────────────────────
echo "[5/10] Copying application files..."
sudo mkdir -p /opt/led-matrix

# Copy all Python files
sudo cp "$SCRIPT_DIR"/*.py /opt/led-matrix/

# Verify main.py exists
if [ ! -f /opt/led-matrix/main.py ]; then
    echo "  ERROR: main.py not found in $SCRIPT_DIR"
    exit 1
fi

echo "  ✓ Files copied to /opt/led-matrix"
echo ""

# ── 6. Configure network fallback IP ──────────────────────────────────────
echo "[6/10] Configuring network fallback IP (10.10.10.99/24)..."

if [ "$NETWORK_MGR" = "NetworkManager" ]; then
    # Use NetworkManager for fallback IP
    CONN_NAME=$(nmcli -t -f NAME connection show --active | grep -E "eth|Wired" | head -1)
    
    if [ -z "$CONN_NAME" ]; then
        echo "  ⚠  No active wired connection found - using first available"
        CONN_NAME=$(nmcli -t -f NAME connection show | head -1)
    fi
    
    echo "  Using connection: $CONN_NAME"
    
    # Check if fallback IP already configured
    if nmcli connection show "$CONN_NAME" | grep -q "10.10.10.99"; then
        echo "  ℹ  Fallback IP already configured"
    else
        echo "  Configuring fallback IP..."
        sudo nmcli connection modify "$CONN_NAME" ipv4.method auto
        sudo nmcli connection modify "$CONN_NAME" +ipv4.addresses 10.10.10.99/24
        sudo nmcli connection modify "$CONN_NAME" ipv4.may-fail no
        
        # Restart connection
        sudo nmcli connection down "$CONN_NAME" 2>/dev/null || true
        sleep 2
        sudo nmcli connection up "$CONN_NAME"
        
        echo "  ✓ Fallback IP configured: 10.10.10.99/24"
    fi
    
elif [ "$NETWORK_MGR" = "dhcpcd" ]; then
    # Legacy dhcpcd fallback configuration
    if [ ! -f /etc/dhcpcd.conf.backup ]; then
        sudo cp /etc/dhcpcd.conf /etc/dhcpcd.conf.backup
    fi
    
    if sudo grep -q "profile static_fallback" /etc/dhcpcd.conf 2>/dev/null; then
        echo "  ℹ  Fallback IP already configured in dhcpcd.conf"
    else
        cat <<'DHCPCD_EOF' | sudo tee -a /etc/dhcpcd.conf > /dev/null

# LED Matrix fallback static IP
profile static_fallback
static ip_address=10.10.10.99/24
static routers=10.10.10.1
static domain_name_servers=8.8.8.8

interface eth0
fallback static_fallback
DHCPCD_EOF
        sudo systemctl restart dhcpcd
        echo "  ✓ Fallback IP configured: 10.10.10.99/24"
    fi
fi
echo ""

# ── 7. Install helper scripts ─────────────────────────────────────────────
echo "[7/10] Installing helper scripts..."

for script in configure-network.sh set-hostname.sh reboot-device.sh; do
    if [ -f "$SCRIPT_DIR/$script" ]; then
        sudo cp "$SCRIPT_DIR/$script" /opt/led-matrix/
        sudo chmod +x "/opt/led-matrix/$script"
        echo "  ✓ $script installed"
    fi
done
echo ""

# ── 8. Configure sudoers ───────────────────────────────────────────────────
echo "[8/10] Configuring sudoers..."

SUDOERS_FILE="/etc/sudoers.d/led-matrix"
cat <<'EOF' | sudo tee "$SUDOERS_FILE" > /dev/null
# LED Matrix daemon permissions
daemon ALL=(ALL) NOPASSWD: /opt/led-matrix/configure-network.sh
daemon ALL=(root) NOPASSWD: /opt/led-matrix/set-hostname.sh
daemon ALL=(root) NOPASSWD: /opt/led-matrix/reboot-device.sh
EOF
sudo chmod 0440 "$SUDOERS_FILE"
echo "  ✓ Sudoers configured"
echo ""

# ── 9. Create storage directory ────────────────────────────────────────────
echo "[9/10] Creating storage directory..."
sudo mkdir -p /var/lib/led-matrix
sudo chown daemon:daemon /var/lib/led-matrix
echo "  ✓ Storage directory created"
echo ""

# ── 10. Install systemd service ────────────────────────────────────────────
echo "[10/10] Installing systemd service..."

# Copy service file
if [ -f "$SCRIPT_DIR/led-matrix.service" ]; then
    sudo cp "$SCRIPT_DIR/led-matrix.service" /etc/systemd/system/led-matrix.service
else
    echo "  ERROR: led-matrix.service not found"
    exit 1
fi

# Remove any override files
if [ -d /etc/systemd/system/led-matrix.service.d ]; then
    sudo rm -rf /etc/systemd/system/led-matrix.service.d
fi

# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable led-matrix.service
sudo systemctl start led-matrix.service

# Wait for service to start
sleep 3

# Check service status
if systemctl is-active --quiet led-matrix.service; then
    echo "  ✓ Service installed and running"
else
    echo "  ⚠  Service installed but not running - check logs"
fi
echo ""

# ── Installation complete ──────────────────────────────────────────────────
echo "========================================="
echo "Installation Complete!"
echo "========================================="
echo ""
echo "Features:"
echo "  ✅ 128×64 BGR LED Matrix driver"
echo "  ✅ UDP control (port 21324)"
echo "  ✅ Web UI (port 8080)"
echo "  ✅ Fallback IP: 10.10.10.99/24"
echo "  ✅ Anti-flicker: PWM_BITS=5 (optimized)"
echo "  ✅ Text padding: 1px margins"
echo "  ✅ Curtain auto-scale"
echo ""
echo "Network:"
if [ "$NETWORK_MGR" = "NetworkManager" ]; then
    echo "  Manager: NetworkManager"
    echo "  Primary: DHCP (current: $(hostname -I | awk '{print $1}'))"
    echo "  Fallback: 10.10.10.99/24 (always active)"
else
    echo "  Manager: dhcpcd"
    echo "  Fallback: 10.10.10.99/24 (when DHCP fails)"
fi
echo ""
echo "Commands:"
echo "  Status:  sudo systemctl status led-matrix"
echo "  Logs:    sudo journalctl -u led-matrix -f"
echo "  Restart: sudo systemctl restart led-matrix"
echo ""
echo "Access:"
echo "  Web UI:  http://$(hostname -I | awk '{print $1}'):8080/"
echo "  Fallback: http://10.10.10.99:8080/"
echo "  UDP:     port 21324"
echo ""
echo "⚠  REBOOT REQUIRED to fully apply:"
echo "   - Audio driver blacklist (hardware PWM)"
echo "   - Network configuration"
echo ""
read -p "Reboot now? [y/N]: " REBOOT_CONFIRM
if [[ "$REBOOT_CONFIRM" =~ ^[Yy]$ ]]; then
    echo "Rebooting in 5 seconds... (Ctrl+C to cancel)"
    sleep 5
    sudo reboot
fi
echo ""
echo "✅ Installation complete - reboot when ready"
echo ""
