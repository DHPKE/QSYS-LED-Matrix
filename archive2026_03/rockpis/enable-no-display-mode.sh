#!/bin/bash
# enable-no-display-mode.sh
# Quick fix to enable NO_DISPLAY mode and stop bus errors

set -e

echo "========================================="
echo "Enabling NO_DISPLAY Mode"
echo "========================================="
echo ""
echo "This will allow the service to run without"
echo "the physical LED display (UDP and Web UI"
echo "will still work for testing)."
echo ""

# Stop the service first
echo "Stopping led-matrix service..."
sudo systemctl stop led-matrix

# Create override directory
sudo mkdir -p /etc/systemd/system/led-matrix.service.d

# Create override file
echo "Creating service override..."
sudo tee /etc/systemd/system/led-matrix.service.d/override.conf > /dev/null << 'EOF'
[Service]
Environment="LED_MATRIX_NO_DISPLAY=1"
EOF

echo "✓ Override file created"

# Reload systemd
echo "Reloading systemd..."
sudo systemctl daemon-reload

# Start the service
echo "Starting led-matrix service..."
sudo systemctl start led-matrix

# Wait a moment
sleep 2

# Check status
echo ""
echo "========================================="
echo "Service Status:"
echo "========================================="
sudo systemctl status led-matrix --no-pager -n 15 || true

echo ""
echo "========================================="
echo "Recent Logs:"
echo "========================================="
sudo journalctl -u led-matrix -n 20 --no-pager

echo ""
echo "========================================="
echo "NO_DISPLAY mode enabled!"
echo "========================================="
echo ""
echo "The service should now run without bus errors."
echo ""
echo "✓ UDP handler: port 21324"
echo "✓ Web UI: http://$(hostname -I | awk '{print $1}')"
echo ""
echo "To disable NO_DISPLAY mode (requires library fix):"
echo "  sudo rm /etc/systemd/system/led-matrix.service.d/override.conf"
echo "  sudo systemctl daemon-reload"
echo "  sudo systemctl restart led-matrix"
echo ""
echo "Monitor live logs: sudo journalctl -u led-matrix -f"
