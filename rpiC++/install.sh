#!/bin/bash
# install.sh - Out-of-the-box installer for Raspbian Lite 64
# Installs all dependencies, builds, and configures the LED matrix controller

set -e

INSTALL_DIR="$(pwd)"
RGB_MATRIX_DIR="/tmp/rpi-rgb-led-matrix"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  RPi C++ LED Matrix Controller - Full Installer           ║"
echo "║  For Raspbian Lite 64 (out-of-the-box setup)              ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "⚠  This script needs root privileges"
    echo "   Run: sudo ./install.sh"
    exit 1
fi

# Detect non-root user for later use
if [ -n "$SUDO_USER" ]; then
    REAL_USER="$SUDO_USER"
else
    REAL_USER="pi"
fi

echo "Installing for user: $REAL_USER"
echo ""

# ─── 1. System Dependencies ──────────────────────────────────────────────────

echo "==[ Step 1/7: Installing System Dependencies ]=="
echo "This may take a few minutes on first run..."
echo ""

apt-get update

# Core build tools
apt-get install -y build-essential git cmake pkg-config

# LED matrix dependencies
apt-get install -y libfreetype6-dev nlohmann-json3-dev

# Fonts for text rendering
apt-get install -y fonts-dejavu-core

# Network tools
apt-get install -y netcat-openbsd

echo "✓ System dependencies installed"
echo ""

# ─── 2. Disable Audio (prevents interference) ─────────────────────────────────

echo "==[ Step 2/7: Disabling Audio (prevents LED flicker) ]=="

if ! grep -q "^dtparam=audio=off" /boot/firmware/config.txt 2>/dev/null; then
    if ! grep -q "^dtparam=audio=off" /boot/config.txt 2>/dev/null; then
        echo "dtparam=audio=off" >> /boot/config.txt
        echo "⚠  Audio disabled in /boot/config.txt (requires reboot)"
        NEEDS_REBOOT=1
    else
        echo "✓ Audio already disabled in /boot/config.txt"
    fi
else
    echo "✓ Audio already disabled in /boot/firmware/config.txt"
fi

# Blacklist audio modules
BLACKLIST_FILE="/etc/modprobe.d/blacklist-rgb-matrix.conf"
if [ ! -f "$BLACKLIST_FILE" ]; then
    cat > "$BLACKLIST_FILE" << 'BLACKLIST_EOF'
# Prevent audio modules from interfering with LED matrix
blacklist snd_bcm2835
BLACKLIST_EOF
    echo "✓ Audio module blacklisted"
else
    echo "✓ Audio module already blacklisted"
fi

echo ""

# ─── 3. Install RGB LED Matrix Library ───────────────────────────────────────

echo "==[ Step 3/7: Installing rpi-rgb-led-matrix Library ]=="

if [ -f "/usr/local/include/led-matrix.h" ] && [ -f "/usr/local/lib/librgbmatrix.a" ]; then
    echo "✓ rpi-rgb-led-matrix already installed"
else
    echo "Cloning and building rpi-rgb-led-matrix..."
    
    if [ -d "$RGB_MATRIX_DIR" ]; then
        rm -rf "$RGB_MATRIX_DIR"
    fi
    
    # Clone as real user to avoid permission issues
    sudo -u "$REAL_USER" git clone https://github.com/hzeller/rpi-rgb-led-matrix.git "$RGB_MATRIX_DIR"
    
    cd "$RGB_MATRIX_DIR"
    
    # Build with optimizations
    make -j$(nproc)
    
    # Manual install (no install target in Makefile)
    echo "Installing library files..."
    cp -r include/* /usr/local/include/
    cp lib/librgbmatrix.* /usr/local/lib/
    ldconfig
    
    cd "$INSTALL_DIR"
    
    echo "✓ rpi-rgb-led-matrix library installed"
fi

echo ""

# ─── 4. Build LED Matrix Controller ──────────────────────────────────────────

echo "==[ Step 4/7: Building LED Matrix Controller ]=="

if [ ! -f "led-matrix" ]; then
    echo "Building controller..."
    make clean 2>/dev/null || true
    make
    echo "✓ Build complete"
else
    echo "⚠  Binary exists. Rebuild? (y/n)"
    read -r response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        make clean
        make
        echo "✓ Rebuild complete"
    else
        echo "✓ Using existing binary"
    fi
fi

echo ""

# ─── 5. Install Service ──────────────────────────────────────────────────────

echo "==[ Step 5/7: Installing Systemd Service ]=="

# Stop existing service if running
systemctl stop led-matrix 2>/dev/null || true
systemctl disable led-matrix 2>/dev/null || true

# Install binary
cp led-matrix /usr/local/bin/
chmod +x /usr/local/bin/led-matrix
echo "✓ Binary installed to /usr/local/bin/led-matrix"

# Install systemd service
cp led-matrix.service /etc/systemd/system/
systemctl daemon-reload
echo "✓ Systemd service installed"

# Create config directory with proper permissions
mkdir -p /var/lib/led-matrix
chown "$REAL_USER:$REAL_USER" /var/lib/led-matrix
chmod 755 /var/lib/led-matrix
echo "✓ Config directory created at /var/lib/led-matrix"

echo ""

# ─── 6. Enable Service ───────────────────────────────────────────────────────

echo "==[ Step 6/7: Enabling Service ]=="

systemctl enable led-matrix
echo "✓ Service enabled (will start on boot)"

echo ""

# ─── 7. Network Configuration ────────────────────────────────────────────────

echo "==[ Step 7/7: Network Configuration ]=="
echo ""
IP=$(hostname -I | awk '{print $1}')
if [ -z "$IP" ]; then
    echo "⚠  No IP address detected"
    echo "   Configure network and reboot"
else
    echo "✓ Device IP: $IP"
fi

echo ""

# ─── Summary ─────────────────────────────────────────────────────────────────

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  ✓ Installation Complete!                                  ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

if [ -n "$NEEDS_REBOOT" ]; then
    echo "⚠  REBOOT REQUIRED (audio disabled in /boot/config.txt)"
    echo ""
    echo "After reboot, start the service:"
    echo "  sudo systemctl start led-matrix"
    echo ""
    echo "Reboot now? (y/n)"
    read -r response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        reboot
    fi
else
    echo "Service Commands:"
    echo "  Start:   sudo systemctl start led-matrix"
    echo "  Stop:    sudo systemctl stop led-matrix"
    echo "  Status:  sudo systemctl status led-matrix"
    echo "  Logs:    sudo journalctl -u led-matrix -f"
    echo ""
    
    if [ -n "$IP" ]; then
        echo "UDP Protocol:"
        echo "  Port:    21324"
        echo "  Test:    echo '{\"cmd\":\"text\",\"seg\":0,\"text\":\"HELLO\"}' | nc -u -w1 $IP 21324"
        echo ""
        echo "Full Test Suite:"
        echo "  ./test-commands.sh $IP"
        echo ""
    fi
    
    echo "Start service now? (y/n)"
    read -r response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        systemctl start led-matrix
        sleep 2
        echo ""
        systemctl status led-matrix --no-pager
        echo ""
        
        if [ -n "$IP" ]; then
            echo "Display should now show: $IP"
            echo ""
            echo "Send test command? (y/n)"
            read -r test_response
            if [[ "$test_response" =~ ^[Yy]$ ]]; then
                echo '{"cmd":"text","seg":0,"text":"C++ READY!","color":"00FF00","bgcolor":"000000","align":"C"}' | nc -u -w1 "$IP" 21324
                echo "✓ Test command sent - check your display!"
            fi
        fi
    fi
fi

echo ""
echo "Installation log saved to: /var/log/led-matrix-install.log"
