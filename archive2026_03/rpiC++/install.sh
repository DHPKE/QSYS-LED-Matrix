#!/bin/bash
# install.sh - Out-of-the-box installer for Raspbian Lite 64
# Fetches latest code from GitHub, installs dependencies, builds, and configures

set -e

REPO_URL="https://github.com/DHPKE/QSYS-LED-Matrix.git"
INSTALL_DIR="/home/$SUDO_USER/rpiC++"
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

# Detect non-root user
if [ -n "$SUDO_USER" ]; then
    REAL_USER="$SUDO_USER"
else
    REAL_USER="pi"
fi

echo "Installing for user: $REAL_USER"
echo ""

# ─── 1. System Dependencies ──────────────────────────────────────────────────

echo "==[ Step 1/9: Installing System Dependencies ]=="
echo "This may take a few minutes on first run..."
echo ""

apt-get update

# Core build tools
apt-get install -y build-essential git cmake pkg-config

# LED matrix dependencies
apt-get install -y libfreetype6-dev nlohmann-json3-dev

# Fonts for text rendering (Arial + DejaVu)
# Accept EULA for msttcorefonts automatically
echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections
apt-get install -y ttf-mscorefonts-installer fonts-dejavu-core

# Network tools
apt-get install -y netcat-openbsd jq

echo "✓ System dependencies installed"
echo ""

# ─── 2. Disable Audio (prevents interference) ─────────────────────────────────

echo "==[ Step 2/9: Disabling Audio (prevents LED flicker) ]=="

# Try both locations (Bookworm uses /boot/firmware)
BOOT_CONFIG="/boot/config.txt"
if [ -f "/boot/firmware/config.txt" ]; then
    BOOT_CONFIG="/boot/firmware/config.txt"
fi

if ! grep -q "^dtparam=audio=off" "$BOOT_CONFIG"; then
    echo "dtparam=audio=off" >> "$BOOT_CONFIG"
    echo "⚠  Audio disabled in $BOOT_CONFIG (requires reboot)"
    NEEDS_REBOOT=1
else
    echo "✓ Audio already disabled in $BOOT_CONFIG"
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

echo "==[ Step 3/9: Installing rpi-rgb-led-matrix Library ]=="

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
    
    echo "✓ rpi-rgb-led-matrix library installed"
fi

echo ""

# ─── 4. Fetch Latest Controller Code ─────────────────────────────────────────

echo "==[ Step 4/9: Fetching Latest Controller Code ]=="

# Create install directory
if [ ! -d "$INSTALL_DIR" ]; then
    mkdir -p "$INSTALL_DIR"
    chown "$REAL_USER:$REAL_USER" "$INSTALL_DIR"
fi

cd "$INSTALL_DIR"

# Clone or update repository
if [ ! -d ".git" ]; then
    echo "Cloning from GitHub..."
    # Remove any existing files
    rm -rf *
    # Clone as real user
    sudo -u "$REAL_USER" git clone "$REPO_URL" temp_clone
    # Move rpiC++ contents to current directory
    sudo -u "$REAL_USER" mv temp_clone/rpiC++/* .
    sudo -u "$REAL_USER" rm -rf temp_clone
    echo "✓ Code fetched from GitHub"
else
    echo "Updating from GitHub..."
    sudo -u "$REAL_USER" git pull
    echo "✓ Code updated"
fi

echo ""

# ─── 5. Build LED Matrix Controller ──────────────────────────────────────────

echo "==[ Step 5/9: Building LED Matrix Controller ]=="

echo "Building controller..."
make clean 2>/dev/null || true
make
echo "✓ Build complete"

echo ""

# ─── 6. Install Service ──────────────────────────────────────────────────────

echo "==[ Step 6/9: Installing Systemd Service ]=="

# Stop existing service if running
systemctl stop led-matrix 2>/dev/null || true
systemctl disable led-matrix 2>/dev/null || true

# Install binary
cp led-matrix /usr/local/bin/
chmod +x /usr/local/bin/led-matrix
echo "✓ Binary installed to /usr/local/bin/led-matrix"

# Install network config applier
if [ -f "apply-network-config.sh" ]; then
    cp apply-network-config.sh /usr/local/bin/
    chmod +x /usr/local/bin/apply-network-config.sh
    echo "✓ Network config script installed"
fi

# Install systemd services
cp led-matrix.service /etc/systemd/system/
if [ -f "led-matrix-network.service" ]; then
    cp led-matrix-network.service /etc/systemd/system/
fi

# Install sudoers file for reboot permission
if [ -f "led-matrix-sudoers" ]; then
    cp led-matrix-sudoers /etc/sudoers.d/led-matrix
    chmod 440 /etc/sudoers.d/led-matrix
    echo "✓ Sudoers file installed (reboot permission)"
fi

systemctl daemon-reload
echo "✓ Systemd services installed"

# Create config directory with proper permissions
mkdir -p /var/lib/led-matrix
chown daemon:daemon /var/lib/led-matrix
chmod 755 /var/lib/led-matrix
echo "✓ Config directory created at /var/lib/led-matrix (daemon:daemon)"

echo ""

# ─── 7. Enable Service ───────────────────────────────────────────────────────

echo "==[ Step 7/9: Enabling Services ]=="

systemctl enable led-matrix
if [ -f "/etc/systemd/system/led-matrix-network.service" ]; then
    systemctl enable led-matrix-network
fi
echo "✓ Services enabled (will start on boot)"

echo ""

# ─── 8. CPU Optimization (Optional) ──────────────────────────────────────────

echo "==[ Step 8/9: CPU Isolation (Optional) ]=="
echo ""
echo "The RGB matrix library uses significant CPU to refresh the display."
echo "On multi-core devices (Pi 3/4/5/CM4), you can dedicate one core to it."
echo ""
echo "Add 'isolcpus=3' to /boot/cmdline.txt?"
echo "This isolates CPU core 3 for the LED matrix only."
echo ""
echo "Configure CPU isolation? (y/n)"
read -r cpu_response

if [[ "$cpu_response" =~ ^[Yy]$ ]]; then
    CMDLINE_FILE="/boot/cmdline.txt"
    if [ -f "/boot/firmware/cmdline.txt" ]; then
        CMDLINE_FILE="/boot/firmware/cmdline.txt"
    fi
    
    if ! grep -q "isolcpus=3" "$CMDLINE_FILE"; then
        # Add isolcpus=3 at the end (remove trailing newline first)
        sed -i 's/$//' "$CMDLINE_FILE"
        echo -n " isolcpus=3" >> "$CMDLINE_FILE"
        echo "✓ CPU isolation configured (core 3 dedicated to LED matrix)"
        NEEDS_REBOOT=1
    else
        echo "✓ CPU isolation already configured"
    fi
else
    echo "⏭  Skipped CPU isolation"
fi

echo ""

# ─── 9. Network Configuration ────────────────────────────────────────────────

echo "==[ Step 9/9: Network Configuration ]=="
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
    echo "⚠  REBOOT REQUIRED (audio/CPU config changed)"
    echo ""
    echo "After reboot, the service will start automatically."
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
        echo "Web UI:"
        echo "  http://$IP:8080"
        echo ""
        echo "UDP Protocol:"
        echo "  Port:    21324"
        echo "  Test:    echo '{\"cmd\":\"text\",\"seg\":0,\"text\":\"HELLO\"}' | nc -u -w1 $IP 21324"
        echo ""
    fi
    
    echo "Start service now? (y/n)"
    read -r response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        systemctl start led-matrix
        sleep 3
        echo ""
        systemctl status led-matrix --no-pager | head -15
        echo ""
        
        if [ -n "$IP" ]; then
            echo "Display should now show: $IP"
            echo ""
            echo "Send test command? (y/n)"
            read -r test_response
            if [[ "$test_response" =~ ^[Yy]$ ]]; then
                echo '{"cmd":"text","seg":0,"text":"C++ READY!","color":"00FF00","bgcolor":"000000","align":"center"}' | nc -u -w1 "$IP" 21324
                echo "✓ Test command sent - check your display!"
            fi
        fi
    fi
fi

echo ""
echo "Documentation: $INSTALL_DIR/README.md"
echo "Quick Start:   $INSTALL_DIR/QUICK_START.md"
