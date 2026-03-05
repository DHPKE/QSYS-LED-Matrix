#!/bin/bash
# build-with-hardware-mapping.sh
# Compile rpi-rgb-led-matrix library with Rock Pi S hardware mapping

set -e

echo "========================================="
echo "Rock Pi S Hardware Mapping Installation"
echo "========================================="
echo ""

# Check if library exists
if [ ! -d ~/rpi-rgb-led-matrix ]; then
    echo "ERROR: ~/rpi-rgb-led-matrix not found"
    echo ""
    echo "Please install the library first:"
    echo "  cd ~"
    echo "  git clone https://github.com/hzeller/rpi-rgb-led-matrix.git"
    echo "  cd rpi-rgb-led-matrix"
    echo "  make"
    exit 1
fi

echo "Found rpi-rgb-led-matrix library"
echo ""

# Get the hardware mapping file location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAPPING_FILE="$SCRIPT_DIR/hardware-mapping-rockpis.h"

if [ ! -f "$MAPPING_FILE" ]; then
    echo "ERROR: hardware-mapping-rockpis.h not found in $SCRIPT_DIR"
    exit 1
fi

echo "Hardware mapping file: $MAPPING_FILE"
echo ""

# Backup the original hardware-mapping.c
HWMAP_C=~/rpi-rgb-led-matrix/lib/hardware-mapping.c
if [ ! -f "$HWMAP_C.backup" ]; then
    echo "Creating backup of hardware-mapping.c..."
    cp "$HWMAP_C" "$HWMAP_C.backup"
    echo "✓ Backup saved to $HWMAP_C.backup"
else
    echo "ℹ Backup already exists at $HWMAP_C.backup"
fi
echo ""

# Check if our mapping is already added
if grep -q "rockpis_mapping" "$HWMAP_C"; then
    echo "⚠ Rock Pi S mapping already present in hardware-mapping.c"
    echo "  Updating with latest version..."
    # Remove old section
    sed -i '/rockpis_mapping/,/^};/d' "$HWMAP_C"
fi

# Add our hardware mapping definition before the hardware_mappings array
echo "Adding Rock Pi S hardware mapping to hardware-mapping.c..."

# Find the line with "static HardwareMapping *hardware_mappings"
LINE_NUM=$(grep -n "static HardwareMapping \*hardware_mappings" "$HWMAP_C" | head -1 | cut -d: -f1)

if [ -z "$LINE_NUM" ]; then
    echo "ERROR: Could not find hardware_mappings array in hardware-mapping.c"
    exit 1
fi

# Insert our mapping definition before the array
{
    head -n $((LINE_NUM - 1)) "$HWMAP_C"
    echo ""
    echo "// ========== Rock Pi S Hardware Mapping =========="
    echo "static struct HardwareMapping rockpis_mapping = {"
    echo "  .name          = \"rockpis\","
    echo "  "
    echo "  // Control signals"
    echo "  .output_enable = 54,  // OE  - GPIO1_C6 - Header 1 Pin 21"
    echo "  .clock         = 71,  // CLK - GPIO2_A7 - Header 1 Pin 22"
    echo "  .strobe        = 55,  // LAT - GPIO1_C7 - Header 1 Pin 19"
    echo "  "
    echo "  // Row address pins"
    echo "  .a             = 11,  // GPIO0_B3 - Header 1 Pin 3"
    echo "  .b             = 12,  // GPIO0_B4 - Header 1 Pin 5"
    echo "  .c             = 65,  // GPIO2_A1 - Header 1 Pin 8  (UART0_TX)"
    echo "  .d             = 64,  // GPIO2_A0 - Header 1 Pin 10 (UART0_RX)"
    echo "  .e             = -1,  // Not used for 1/16 scan (32px height)"
    echo "  "
    echo "  // Upper half RGB"
    echo "  .p0_r1         = 16,  // GPIO0_C0 - Header 1 Pin 13"
    echo "  .p0_g1         = 17,  // GPIO0_C1 - Header 1 Pin 15"
    echo "  .p0_b1         = 15,  // GPIO0_B7 - Header 1 Pin 11"
    echo "  "
    echo "  // Lower half RGB"
    echo "  .p0_r2         = 68,  // GPIO2_A4 - Header 1 Pin 7"
    echo "  .p0_g2         = 69,  // GPIO2_A5 - Header 1 Pin 12"
    echo "  .p0_b2         = 74,  // GPIO2_B2 - Header 1 Pin 16"
    echo "};"
    echo "// =================================================="
    echo ""
    tail -n +$LINE_NUM "$HWMAP_C"
} > "$HWMAP_C.tmp"
mv "$HWMAP_C.tmp" "$HWMAP_C"

echo "✓ Hardware mapping definition added"
echo ""

# Now add it to the array
echo "Registering rockpis_mapping in hardware_mappings array..."

# Find the line with the array and add our mapping
if ! grep -q "&rockpis_mapping," "$HWMAP_C"; then
    # Find NULL terminator line
    sed -i "/static HardwareMapping \*hardware_mappings/,/NULL/s/  NULL/  \&rockpis_mapping,\n  NULL/" "$HWMAP_C"
    echo "✓ Mapping registered in array"
else
    echo "ℹ Mapping already in array"
fi
echo ""

# Clean and rebuild
echo "========================================="
echo "Rebuilding library..."
echo "========================================="
cd ~/rpi-rgb-led-matrix
make clean
make -j$(nproc)
echo ""
echo "✓ Library compiled successfully"
echo ""

# Rebuild Python bindings
echo "========================================="
echo "Rebuilding Python bindings..."
echo "========================================="
cd bindings/python
sudo make install
echo ""
echo "✓ Python bindings installed"
echo ""

# Update config.py
echo "========================================="
echo "Updating config.py..."
echo "========================================="

CONFIG_FILE=/opt/led-matrix/config.py
if [ -f "$CONFIG_FILE" ]; then
    if grep -q "MATRIX_HARDWARE_MAPPING.*regular" "$CONFIG_FILE"; then
        sudo sed -i "s/MATRIX_HARDWARE_MAPPING = .*/MATRIX_HARDWARE_MAPPING = \"rockpis\"/" "$CONFIG_FILE"
        echo "✓ Updated MATRIX_HARDWARE_MAPPING to 'rockpis'"
    else
        echo "ℹ MATRIX_HARDWARE_MAPPING already set (check manually)"
    fi
else
    echo "⚠ $CONFIG_FILE not found - update manually"
fi
echo ""

# Prompt to disable NO_DISPLAY mode
echo "========================================="
echo "Remove NO_DISPLAY mode?"
echo "========================================="
echo ""
echo "The hardware mapping is now installed."
echo "To enable the physical display, we need to"
echo "remove the NO_DISPLAY environment variable."
echo ""
read -p "Remove NO_DISPLAY override now? [y/N]: " REMOVE_OVERRIDE

if [[ "$REMOVE_OVERRIDE" =~ ^[Yy]$ ]]; then
    if [ -f /etc/systemd/system/led-matrix.service.d/override.conf ]; then
        echo "Removing NO_DISPLAY override..."
        sudo rm /etc/systemd/system/led-matrix.service.d/override.conf
        sudo rmdir /etc/systemd/system/led-matrix.service.d 2>/dev/null || true
        sudo systemctl daemon-reload
        echo "✓ Override removed"
    else
        echo "ℹ No override file found"
    fi
fi

# Prompt for UART0 disable
echo ""
echo "========================================="
echo "CRITICAL: Disable UART0 Console"
echo "========================================="
echo ""
echo "Pins 8 and 10 (UART0_TX/RX) are used for"
echo "the C and D address lines. The serial"
echo "console MUST be disabled or the display"
echo "will not work correctly."
echo ""
echo "Commands to run:"
echo "  sudo systemctl disable --now serial-getty@ttyS0"
echo "  sudo nano /boot/armbianEnv.txt"
echo "  (Remove 'console=ttyS0,1500000' from the line)"
echo "  sudo reboot"
echo ""
read -p "Disable UART0 console now? [y/N]: " DISABLE_UART

if [[ "$DISABLE_UART" =~ ^[Yy]$ ]]; then
    echo ""
    echo "Disabling serial console service..."
    sudo systemctl disable --now serial-getty@ttyS0 || true
    echo "✓ Service disabled"
    echo ""
    echo "Please edit /boot/armbianEnv.txt and remove:"
    echo "  console=ttyS0,1500000"
    echo ""
    echo "Press Enter to open the file..."
    read
    sudo nano /boot/armbianEnv.txt
    echo ""
    echo "⚠ REBOOT REQUIRED for UART changes to take effect"
fi

# Restart service
echo ""
echo "========================================="
echo "Restart led-matrix service?"
echo "========================================="
echo ""
read -p "Restart service now? [y/N]: " RESTART_SERVICE

if [[ "$RESTART_SERVICE" =~ ^[Yy]$ ]]; then
    echo "Restarting led-matrix service..."
    sudo systemctl restart led-matrix
    sleep 2
    echo ""
    sudo systemctl status led-matrix --no-pager -n 20 || true
    echo ""
    echo "Monitor logs: sudo journalctl -u led-matrix -f"
fi

echo ""
echo "========================================="
echo "Installation Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "  1. If you disabled UART0, REBOOT the system"
echo "  2. Verify physical wiring matches CORRECTED_PINOUT.md"
echo "  3. Monitor logs: sudo journalctl -u led-matrix -f"
echo "  4. Look for: ✓ LED matrix initialised"
echo ""
echo "If display doesn't work:"
echo "  - Check all 13 wires are connected correctly"
echo "  - Verify UART0 console is disabled (check dmesg | grep ttyS0)"
echo "  - Try gpio_slowdown=3 in config.py if you see color issues"
echo "  - Check power supply (5V, 2A minimum)"
echo ""
