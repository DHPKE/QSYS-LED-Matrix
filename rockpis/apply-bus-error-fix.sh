#!/bin/bash
# apply-bus-error-fix.sh
# Quick deployment script to apply NO_DISPLAY mode fix on Rock Pi S

set -e

echo "========================================="
echo "Rock Pi S - Bus Error Fix Deployment"
echo "========================================="
echo ""

# Check if running on Rock Pi S
if [ ! -f /proc/device-tree/model ]; then
    echo "ERROR: /proc/device-tree/model not found"
    exit 1
fi

MODEL=$(cat /proc/device-tree/model 2>/dev/null | tr -d '\0' || echo "unknown")
echo "Detected hardware: $MODEL"
echo ""

# Update from git
REPO_DIR=""
if [ -d ~/QSYS-LED-Matrix ]; then
    REPO_DIR=~/QSYS-LED-Matrix
elif [ -d ./QSYS-LED-Matrix ]; then
    REPO_DIR=./QSYS-LED-Matrix
elif [ -f ../main.py ]; then
    # Already in rockpis/ subdirectory
    REPO_DIR=..
elif [ -f ./main.py ]; then
    # Already in repository root
    REPO_DIR=.
fi

if [ -z "$REPO_DIR" ]; then
    echo "ERROR: QSYS-LED-Matrix repository not found"
    echo "Clone first: git clone https://github.com/DHPKE/QSYS-LED-Matrix.git"
    exit 1
fi

echo "Updating repository..."
cd "$REPO_DIR"
git pull origin main || echo "⚠ Git pull failed (continue anyway)"
echo "✓ Repository updated"

# Make sure we're in the right place
if [ ! -f rockpis/main.py ]; then
    echo "ERROR: rockpis/main.py not found in $REPO_DIR"
    exit 1
fi

# Copy files
echo ""
echo "Copying files to /opt/led-matrix/..."
sudo mkdir -p /opt/led-matrix
sudo cp rockpis/main.py /opt/led-matrix/
sudo cp rockpis/config.py /opt/led-matrix/
sudo cp rockpis/segment_manager.py /opt/led-matrix/
sudo cp rockpis/udp_handler.py /opt/led-matrix/
sudo cp rockpis/text_renderer.py /opt/led-matrix/
sudo cp rockpis/web_server.py /opt/led-matrix/
echo "✓ Python files copied"

# Update service file
echo ""
echo "Updating systemd service..."
sudo cp rockpis/led-matrix.service /etc/systemd/system/
sudo systemctl daemon-reload
echo "✓ Service file updated"

# Prompt for mode selection
echo ""
echo "========================================="
echo "Select operation mode:"
echo "========================================="
echo ""
echo "1. NORMAL MODE (default)"
echo "   - Attempts to initialize physical LED display"
echo "   - May crash with bus error if library isn't patched"
echo ""
echo "2. NO_DISPLAY MODE"
echo "   - UDP and Web server work"
echo "   - NO physical LED display output"
echo "   - Use for testing/troubleshooting"
echo ""
read -p "Enter choice [1/2]: " MODE_CHOICE

if [ "$MODE_CHOICE" = "2" ]; then
    echo ""
    echo "Enabling NO_DISPLAY mode..."
    sudo systemctl edit --full led-matrix
    echo ""
    echo "INSTRUCTIONS:"
    echo "In the editor, add this line under [Service]:"
    echo "    Environment=\"LED_MATRIX_NO_DISPLAY=1\""
    echo ""
    echo "Save and exit to continue..."
    read -p "Press Enter when done..."
else
    echo ""
    echo "Using NORMAL mode (physical display)"
    echo "If you see bus errors, rerun and select NO_DISPLAY mode"
fi

# Restart service
echo ""
echo "Restarting led-matrix service..."
sudo systemctl restart led-matrix

echo ""
echo "========================================="
echo "Checking service status..."
echo "========================================="
sleep 2
sudo systemctl status led-matrix --no-pager -n 20 || true

echo ""
echo "========================================="
echo "Deployment complete!"
echo "========================================="
echo ""
echo "Monitor logs: sudo journalctl -u led-matrix -f"
echo ""
echo "Expected output (NO_DISPLAY mode):"
echo "  LED_MATRIX_NO_DISPLAY set — running in NO-DISPLAY test mode"
echo "  ↳ UDP and web server functional, but NO physical panel output"
echo ""
echo "Expected output (NORMAL mode - if working):"
echo "  ✓ LED matrix initialised"
echo "  ↳ Hardware pulsing disabled (bit-bang OE- mode for RK3308)"
echo ""
echo "If you see 'signal 7/BUS' errors:"
echo "  1. Run: sudo systemctl stop led-matrix"
echo "  2. Enable NO_DISPLAY mode: sudo systemctl edit led-matrix"
echo "  3. Add: Environment=\"LED_MATRIX_NO_DISPLAY=1\""
echo "  4. Run: sudo systemctl start led-matrix"
echo ""
echo "See BUS_ERROR_FIX.md for permanent solutions"
