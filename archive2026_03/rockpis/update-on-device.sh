#!/bin/bash
# update-on-device.sh — Quick update script for Rock Pi S
#
# Run this script ON the Rock Pi S after pulling new code:
#   cd ~/QSYS-LED-Matrix/rockpis
#   sudo bash update-on-device.sh
#
# Or copy just the updated files via SCP:
#   scp rockpis/main.py root@rockpi-s:/opt/led-matrix/
#   ssh root@rockpi-s "systemctl restart led-matrix"

set -e

if [ "$EUID" -ne 0 ]; then
  echo "ERROR: Please run as root:  sudo bash update-on-device.sh"
  exit 1
fi

echo "=== Updating LED Matrix Controller on Rock Pi S ==="
echo ""

# Get the directory where this script lives
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "[1/3] Stopping led-matrix service..."
systemctl stop led-matrix.service
echo "  ✓ Service stopped"

echo "[2/3] Copying updated Python files to /opt/led-matrix/..."
cp "$SCRIPT_DIR"/main.py           /opt/led-matrix/
cp "$SCRIPT_DIR"/config.py         /opt/led-matrix/
cp "$SCRIPT_DIR"/segment_manager.py /opt/led-matrix/
cp "$SCRIPT_DIR"/udp_handler.py    /opt/led-matrix/
cp "$SCRIPT_DIR"/text_renderer.py  /opt/led-matrix/
cp "$SCRIPT_DIR"/web_server.py     /opt/led-matrix/
echo "  ✓ Files copied"

echo "[3/3] Starting led-matrix service..."
systemctl start led-matrix.service
echo "  ✓ Service started"

echo ""
echo "=== Update complete ==="
echo ""
echo "Check logs:  sudo journalctl -u led-matrix -f"
echo "Check status: sudo systemctl status led-matrix"
echo ""
