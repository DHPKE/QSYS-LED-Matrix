#!/bin/bash
# update.sh — Update an existing LED Matrix installation
#
# This script updates the Python files on a running system.
# Use this after pulling git updates or making local changes.
#
# Run WITHOUT sudo:  bash update.sh
# (Script will prompt for sudo when needed)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================="
echo "RPi LED Matrix - Update Application"
echo "========================================="
echo ""

# Check if installation exists
if [ ! -d /opt/led-matrix ]; then
    echo "ERROR: /opt/led-matrix does not exist"
    echo "Run install.sh first to set up the system"
    exit 1
fi

echo "[1/3] Copying updated Python files to /opt/led-matrix..."
sudo cp "$SCRIPT_DIR"/*.py /opt/led-matrix/
echo "  ✓ Files updated"
echo ""

echo "[2/3] Restarting led-matrix service..."
sudo systemctl restart led-matrix.service
echo "  ✓ Service restarted"
echo ""

echo "[3/3] Checking service status..."
sleep 2
if sudo systemctl is-active --quiet led-matrix.service; then
    echo "  ✓ Service is running"
    IP=$(hostname -I | awk '{print $1}')
    echo ""
    echo "========================================="
    echo "Update complete!"
    echo "========================================="
    echo ""
    echo "Web UI:    http://$IP:8080/"
    echo "View logs: sudo journalctl -u led-matrix -f"
    echo ""
else
    echo "  ⚠  Service failed to start"
    echo ""
    echo "Check logs with: sudo journalctl -u led-matrix -n 50"
    exit 1
fi
