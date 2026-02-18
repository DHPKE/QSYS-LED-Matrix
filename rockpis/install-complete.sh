#!/bin/bash
# install-complete.sh
# Complete installation of LED matrix system on Rock Pi S
# This script handles everything from library installation to service setup

set -e

echo "========================================="
echo "Rock Pi S LED Matrix - Complete Install"
echo "========================================="
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
   echo "ERROR: Do not run this script as root (don't use sudo)"
   echo "The script will prompt for sudo when needed"
   exit 1
fi

# Check hardware
if [ ! -f /proc/device-tree/model ]; then
    echo "WARNING: Cannot detect hardware model"
else
    MODEL=$(cat /proc/device-tree/model 2>/dev/null | tr -d '\0' || echo "unknown")
    echo "Detected: $MODEL"
    echo ""
fi

# Step 1: Install dependencies
echo "========================================="
echo "Step 1: Installing Dependencies"
echo "========================================="
echo ""
sudo apt update
sudo apt install -y \
    git \
    build-essential \
    python3 \
    python3-dev \
    python3-pillow \
    fonts-dejavu-core \
    libgraphicsmagick++-dev \
    libwebp-dev
echo "✓ Dependencies installed"
echo ""

# Step 2: Clone rpi-rgb-led-matrix library
echo "========================================="
echo "Step 2: Installing rpi-rgb-led-matrix"
echo "========================================="
echo ""

if [ -d ~/rpi-rgb-led-matrix ]; then
    echo "ℹ Library already exists at ~/rpi-rgb-led-matrix"
    read -p "Update existing library? [y/N]: " UPDATE_LIB
    if [[ "$UPDATE_LIB" =~ ^[Yy]$ ]]; then
        cd ~/rpi-rgb-led-matrix
        git pull
        echo "✓ Library updated"
    fi
else
    echo "Cloning rpi-rgb-led-matrix library..."
    cd ~
    git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
    echo "✓ Library cloned"
fi
echo ""

# Step 3: Apply Rock Pi S hardware mapping
echo "========================================="
echo "Step 3: Applying Rock Pi S Hardware Mapping"
echo "========================================="
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HWMAP_C=~/rpi-rgb-led-matrix/lib/hardware-mapping.c

# Backup original
if [ ! -f "$HWMAP_C.backup" ]; then
    echo "Creating backup of hardware-mapping.c..."
    cp "$HWMAP_C" "$HWMAP_C.backup"
    echo "✓ Backup saved"
fi

# Check if already patched
if grep -q "rockpis_mapping" "$HWMAP_C"; then
    echo "ℹ Rock Pi S mapping already present"
    echo "  Removing old version..."
    cp "$HWMAP_C.backup" "$HWMAP_C"
fi

# Find insertion point
LINE_NUM=$(grep -n "struct HardwareMapping matrix_hardware_mappings\[\]" "$HWMAP_C" | head -1 | cut -d: -f1)

if [ -z "$LINE_NUM" ]; then
    echo "ERROR: Could not find matrix_hardware_mappings array"
    exit 1
fi

# Insert the mapping before the array
echo "Patching hardware-mapping.c..."
{
    head -n $((LINE_NUM - 1)) "$HWMAP_C"
    cat << 'MAPPING_END'

// ========== Rock Pi S Hardware Mapping (RK3308) ==========
// All pins on Header 1 (26-pin GPIO)
// See: https://docs.radxa.com/en/rock-pi-s/hardware/rock-pi-s
static const struct HardwareMapping rockpis_mapping = {
  .name          = "rockpis",
  
  // Control signals
  .output_enable = GPIO_BIT(54),  // OE  - GPIO1_C6 - Header 1 Pin 21
  .clock         = GPIO_BIT(71),  // CLK - GPIO2_A7 - Header 1 Pin 22
  .strobe        = GPIO_BIT(55),  // LAT - GPIO1_C7 - Header 1 Pin 19
  
  // Row address pins
  .a             = GPIO_BIT(11),  // GPIO0_B3 - Header 1 Pin 3
  .b             = GPIO_BIT(12),  // GPIO0_B4 - Header 1 Pin 5
  .c             = GPIO_BIT(65),  // GPIO2_A1 - Header 1 Pin 8  (UART0_TX - disable console!)
  .d             = GPIO_BIT(64),  // GPIO2_A0 - Header 1 Pin 10 (UART0_RX - disable console!)
  .e             = 0,             // Not used for 32px height (1/16 scan)
  
  // Upper half RGB
  .p0_r1         = GPIO_BIT(16),  // GPIO0_C0 - Header 1 Pin 13
  .p0_g1         = GPIO_BIT(17),  // GPIO0_C1 - Header 1 Pin 15
  .p0_b1         = GPIO_BIT(15),  // GPIO0_B7 - Header 1 Pin 11
  
  // Lower half RGB
  .p0_r2         = GPIO_BIT(68),  // GPIO2_A4 - Header 1 Pin 7
  .p0_g2         = GPIO_BIT(69),  // GPIO2_A5 - Header 1 Pin 12
  .p0_b2         = GPIO_BIT(74),  // GPIO2_B2 - Header 1 Pin 16
};
// ==========================================================

MAPPING_END
    tail -n +$LINE_NUM "$HWMAP_C"
} > "$HWMAP_C.tmp"
mv "$HWMAP_C.tmp" "$HWMAP_C"

# Add to the end of the array (before the last closing brace and semicolon)
if ! grep -q "rockpis_mapping" "$HWMAP_C"; then
    # Find the line with "};" that ends the array
    LAST_BRACE=$(grep -n "^};" "$HWMAP_C" | tail -1 | cut -d: -f1)
    if [ -n "$LAST_BRACE" ]; then
        # Insert before the closing brace
        sed -i "${LAST_BRACE}i\\  rockpis_mapping," "$HWMAP_C"
        echo "✓ Mapping registered"
    fi
else
    echo "ℹ Mapping already registered"
fi
echo ""

# Step 4: Compile library
echo "========================================="
echo "Step 4: Compiling Library"
echo "========================================="
echo ""
cd ~/rpi-rgb-led-matrix
make clean
make -j$(nproc)
echo "✓ Library compiled"
echo ""

# Step 5: Install Python bindings
echo "========================================="
echo "Step 5: Installing Python Bindings"
echo "========================================="
echo ""
cd ~/rpi-rgb-led-matrix/bindings/python
sudo python3 setup.py install
echo "✓ Python bindings installed"
echo ""

# Step 6: Deploy application files
echo "========================================="
echo "Step 6: Deploying Application"
echo "========================================="
echo ""

# Find the repository
REPO_DIR=""
if [ -f "$SCRIPT_DIR/main.py" ]; then
    REPO_DIR="$SCRIPT_DIR"
elif [ -f "$SCRIPT_DIR/../rockpis/main.py" ]; then
    REPO_DIR="$SCRIPT_DIR/../rockpis"
fi

if [ -z "$REPO_DIR" ]; then
    echo "ERROR: Cannot find application files"
    exit 1
fi

sudo mkdir -p /opt/led-matrix
sudo cp "$REPO_DIR/main.py" /opt/led-matrix/
sudo cp "$REPO_DIR/config.py" /opt/led-matrix/
sudo cp "$REPO_DIR/segment_manager.py" /opt/led-matrix/
sudo cp "$REPO_DIR/udp_handler.py" /opt/led-matrix/
sudo cp "$REPO_DIR/text_renderer.py" /opt/led-matrix/
sudo cp "$REPO_DIR/web_server.py" /opt/led-matrix/
echo "✓ Application files deployed"
echo ""

# Step 7: Install systemd service
echo "========================================="
echo "Step 7: Installing Service"
echo "========================================="
echo ""
sudo cp "$REPO_DIR/led-matrix.service" /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable led-matrix
echo "✓ Service installed and enabled"
echo ""

# Step 8: UART0 configuration
echo "========================================="
echo "Step 8: UART0 Console Configuration"
echo "========================================="
echo ""
echo "CRITICAL: Pins 8 & 10 are used for C and D address lines"
echo "The UART0 serial console MUST be disabled!"
echo ""
echo "Steps:"
echo "  1. Disable serial-getty service"
echo "  2. Edit /boot/armbianEnv.txt"
echo "  3. Remove 'console=ttyS0,1500000'"
echo "  4. Reboot"
echo ""
read -p "Disable UART0 now? [Y/n]: " DISABLE_UART

if [[ ! "$DISABLE_UART" =~ ^[Nn]$ ]]; then
    echo ""
    echo "Disabling serial-getty@ttyS0..."
    sudo systemctl disable --now serial-getty@ttyS0 2>/dev/null || echo "  (service not active)"
    
    echo ""
    echo "Checking /boot/armbianEnv.txt..."
    if [ -f /boot/armbianEnv.txt ]; then
        if grep -q "console=ttyS0" /boot/armbianEnv.txt; then
            echo "⚠ Found UART0 console configuration"
            echo ""
            echo "Opening editor - remove 'console=ttyS0,1500000' from the file"
            echo "Press Enter to continue..."
            read
            sudo nano /boot/armbianEnv.txt
            echo "✓ Configuration updated"
        else
            echo "✓ No UART0 console found in armbianEnv.txt"
        fi
    else
        echo "ℹ /boot/armbianEnv.txt not found (check /boot/boot.ini or /boot/uEnv.txt)"
    fi
fi
echo ""

# Step 9: Start service
echo "========================================="
echo "Step 9: Starting Service"
echo "========================================="
echo ""

# Remove NO_DISPLAY override if present
if [ -f /etc/systemd/system/led-matrix.service.d/override.conf ]; then
    echo "Removing NO_DISPLAY override..."
    sudo rm /etc/systemd/system/led-matrix.service.d/override.conf
    sudo rmdir /etc/systemd/system/led-matrix.service.d 2>/dev/null || true
    sudo systemctl daemon-reload
fi

echo "Starting led-matrix service..."
sudo systemctl start led-matrix
sleep 3

echo ""
echo "Service status:"
sudo systemctl status led-matrix --no-pager -n 15 || true

echo ""
echo "Recent logs:"
sudo journalctl -u led-matrix -n 20 --no-pager

echo ""
echo "========================================="
echo "Installation Complete!"
echo "========================================="
echo ""

# Check if successful
if systemctl is-active --quiet led-matrix; then
    echo "✓ Service is running"
    
    # Get IP address
    IP_ADDR=$(hostname -I | awk '{print $1}')
    
    echo ""
    echo "Access points:"
    echo "  Web UI:  http://$IP_ADDR"
    echo "  UDP:     $IP_ADDR:21324"
    echo ""
    
    if grep -q "LED matrix initialised" <(sudo journalctl -u led-matrix -n 50); then
        echo "✓ Physical display initialized successfully!"
    else
        echo "⚠ Check if display is working (monitor logs below)"
    fi
else
    echo "⚠ Service failed to start - check logs above"
fi

echo ""
echo "Commands:"
echo "  Monitor logs:    sudo journalctl -u led-matrix -f"
echo "  Restart service: sudo systemctl restart led-matrix"
echo "  Stop service:    sudo systemctl stop led-matrix"
echo "  Service status:  sudo systemctl status led-matrix"
echo ""

if [[ ! "$DISABLE_UART" =~ ^[Nn]$ ]]; then
    echo "========================================="
    echo "⚠ REBOOT REQUIRED"
    echo "========================================="
    echo ""
    echo "UART0 configuration changes require a reboot"
    echo "to take effect. The display may not work"
    echo "correctly until you reboot."
    echo ""
    read -p "Reboot now? [y/N]: " REBOOT_NOW
    if [[ "$REBOOT_NOW" =~ ^[Yy]$ ]]; then
        echo "Rebooting in 5 seconds..."
        sleep 5
        sudo reboot
    fi
fi

echo ""
echo "Installation script finished!"
