#!/bin/bash
# deploy-updates.sh - Deploy recent updates to the Pi
# Run this when Pi is back online

set -e

PI_IP="10.1.1.24"
PI_USER="node"
PI_PASS="node"

echo "========================================="
echo "Deploying LED Matrix Updates"
echo "========================================="
echo ""

# Check if Pi is reachable
echo "[1/4] Checking Pi connectivity..."
if ! SSHPASS="$PI_PASS" sshpass -e ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 "$PI_USER@$PI_IP" 'echo "Connected"' 2>/dev/null; then
    echo "✗ Pi not reachable at $PI_IP"
    echo "Please check network connection and try again"
    exit 1
fi
echo "✓ Pi is online"
echo ""

# Upload updated files
echo "[2/4] Uploading updated files..."
SSHPASS="$PI_PASS" sshpass -e scp -o StrictHostKeyChecking=no \
    main.py config.py \
    "$PI_USER@$PI_IP:~/QSYS-LED-Matrix/rpi/"
echo "✓ Files uploaded to home directory"
echo ""

# Install to /opt/led-matrix
echo "[3/4] Installing to /opt/led-matrix..."
SSHPASS="$PI_PASS" sshpass -e ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" \
    'sudo cp ~/QSYS-LED-Matrix/rpi/main.py /opt/led-matrix/ && \
     sudo cp ~/QSYS-LED-Matrix/rpi/config.py /opt/led-matrix/'
echo "✓ Files installed"
echo ""

# Restart service
echo "[4/4] Restarting LED Matrix service..."
SSHPASS="$PI_PASS" sshpass -e ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" \
    'sudo systemctl restart led-matrix'
sleep 3
echo "✓ Service restarted"
echo ""

# Check service status
echo "Checking service status..."
SSHPASS="$PI_PASS" sshpass -e ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" \
    'sudo systemctl status led-matrix --no-pager | head -15'
echo ""

echo "========================================="
echo "Deployment complete!"
echo "========================================="
echo ""
echo "Updates deployed:"
echo "  ✅ Network monitor (automatic IP display updates)"
echo "  ✅ Fallback IP configuration (10.20.30.40)"
echo ""
echo "The panel will now:"
echo "  - Show current IP on startup"
echo "  - Update display when IP changes"
echo "  - Fall back to 10.20.30.40 if no DHCP"
echo ""
echo "Web UI: http://$PI_IP:8080/"
echo ""
