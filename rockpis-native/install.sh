#!/bin/bash
# Installation script for Rock Pi S Native HUB75 Driver

set -e

echo "=================================="
echo "Rock Pi S LED Matrix Installation"
echo "=================================="
echo ""

# Check if running as root (allow root for automated installation)
if [ "$EUID" -ne 0 ]; then
   echo "This script requires root privileges"
   echo "Please run: sudo bash install.sh"
   exit 1
fi

# Detect Rock Pi S
echo "Checking system..."
MODEL=$(tr -d '\0' < /proc/device-tree/model 2>/dev/null || echo "Unknown")
echo "Detected: $MODEL"

if [[ ! "$MODEL" =~ "Radxa ROCK Pi S" ]]; then
    echo "⚠️  Warning: This doesn't appear to be a Rock Pi S"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Update package list
echo ""
echo "Step 1: Updating package list..."
apt update

# Install dependencies
echo ""
echo "Step 2: Installing dependencies..."
apt install -y \
    python3 \
    python3-pip \
    python3-dev \
    python3-pil \
    gpiod \
    libgpiod-dev \
    libgpiod2 \
    fonts-dejavu-core \
    git

# Install Python gpiod via pip (not available in all repos)
echo ""
echo "Step 3: Installing Python gpiod library..."
pip3 install --break-system-packages gpiod 2>/dev/null || pip3 install gpiod

# Verify gpiod installation
echo ""
echo "Step 4: Verifying GPIO access..."
if ! command -v gpiodetect &> /dev/null; then
    echo "✗ gpiod tools not found"
    exit 1
fi

echo "Available GPIO chips:"
gpiodetect
echo ""

# Check for /dev/gpiochip*
if [ ! -e /dev/gpiochip0 ]; then
    echo "✗ /dev/gpiochip0 not found"
    exit 1
fi
echo "✓ GPIO chip found"

# Disable UART0 console (frees GPIO 64 & 65)
echo ""
echo "Step 5: Disabling UART0 console..."
if systemctl is-enabled serial-getty@ttyS0.service &> /dev/null; then
    systemctl disable --now serial-getty@ttyS0.service
    echo "✓ UART0 console disabled"
else
    echo "✓ UART0 console already disabled"
fi

# Remove console from kernel command line (check common locations)
BOOT_CONFIG=""
if [ -f /boot/armbianEnv.txt ]; then
    BOOT_CONFIG="/boot/armbianEnv.txt"
elif [ -f /boot/dietpiEnv.txt ]; then
    BOOT_CONFIG="/boot/dietpiEnv.txt"
elif [ -f /boot/cmdline.txt ]; then
    BOOT_CONFIG="/boot/cmdline.txt"
fi

if [ -n "$BOOT_CONFIG" ] && [ -f "$BOOT_CONFIG" ]; then
    if grep -q "console=ttyS0" "$BOOT_CONFIG"; then
        echo "Removing ttyS0 from kernel console in $BOOT_CONFIG..."
        sed -i 's/console=ttyS0[^ ]* //g' "$BOOT_CONFIG"
        echo "✓ Updated $BOOT_CONFIG"
        REBOOT_NEEDED=1
    fi
fi

# Create installation directory
echo ""
echo "Step 6: Installing application..."
INSTALL_DIR="/opt/led-matrix-native"
# Create systemd service
echo ""
echo "Step 6: Installing systemd service..."
tee /etc/systemd/system/led-matrix.service > /dev/null <<'EOF'
cp "$SCRIPT_DIR"/*.py "$INSTALL_DIR/"
chmod +x "$INSTALL_DIR"/*.py

echo "✓ Files copied to $INSTALL_DIR"

# Create systemd service
echo ""
echo "Step 6: Installing systemd service..."
sudo tee /etc/systemd/system/led-matrix.service > /dev/null <<'EOF'
[Unit]
Description=LED Matrix Display Controller
After=network.target
[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable led-matrix.service

echo "✓ Service installed"

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable led-matrix.service

echo "✓ Service installed"

# Test GPIO access
echo ""
echo "Step 8: Testing GPIO access..."
python3 <<'PYEOF'
import sys
try:
    import gpiod
    chip = gpiod.Chip('/dev/gpiochip0')
    print(f"✓ GPIO chip: {chip.get_info().name}")
    print(f"✓ GPIO lines: {chip.get_info().num_lines}")
    chip.close()
except Exception as e:
    print(f"✗ GPIO test failed: {e}")
    sys.exit(1)
PYEOF

if [ $? -ne 0 ]; then
    echo "GPIO access test failed"
    exit 1
fi

# Summary
echo ""
echo "=================================="
echo "Installation Complete!"
echo "=================================="
echo ""
echo "Next steps:"
echo "1. Connect your 64×32 HUB75 LED panel to Rock Pi S Header 1"
echo "2. Verify wiring matches the pinout in NATIVE_DRIVER_PLAN.md"
echo "3. Start the service: sudo systemctl start led-matrix"
echo "4. Check status: sudo systemctl status led-matrix"
echo "5. View logs: sudo journalctl -u led-matrix -f"
echo ""

if [ -n "$REBOOT_NEEDED" ]; then
    echo "⚠️  REBOOT REQUIRED to disable UART0 console"
    echo "Run: sudo reboot"
    echo ""
fi

echo "UDP API: Port 21324 (JSON commands)"
echo "Web UI: http://$(hostname -I | awk '{print $1}')"
echo ""
echo "To test display manually:"
echo "  cd $INSTALL_DIR"
echo "  sudo python3 hub75_driver.py"
echo ""
