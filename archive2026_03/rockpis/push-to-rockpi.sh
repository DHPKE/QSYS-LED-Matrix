#!/bin/bash
# push-to-rockpi.sh — Push updated files from dev machine to Rock Pi S
#
# Usage from your dev machine:
#   cd QSYS-LED-Matrix/rockpis
#   bash push-to-rockpi.sh <rockpi-ip-address>
#
# Example:
#   bash push-to-rockpi.sh 192.168.1.100

ROCKPI_IP="$1"

if [ -z "$ROCKPI_IP" ]; then
  echo "Usage: bash push-to-rockpi.sh <rockpi-ip-address>"
  echo "Example: bash push-to-rockpi.sh 192.168.1.100"
  exit 1
fi

echo "=== Pushing updates to Rock Pi S at $ROCKPI_IP ==="
echo ""

# Get the directory where this script lives
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "[1/3] Copying Python files to Rock Pi S..."
scp "$SCRIPT_DIR"/main.py           root@"$ROCKPI_IP":/opt/led-matrix/ || exit 1
scp "$SCRIPT_DIR"/config.py         root@"$ROCKPI_IP":/opt/led-matrix/ || exit 1
scp "$SCRIPT_DIR"/segment_manager.py root@"$ROCKPI_IP":/opt/led-matrix/ || exit 1
scp "$SCRIPT_DIR"/udp_handler.py    root@"$ROCKPI_IP":/opt/led-matrix/ || exit 1
scp "$SCRIPT_DIR"/text_renderer.py  root@"$ROCKPI_IP":/opt/led-matrix/ || exit 1
scp "$SCRIPT_DIR"/web_server.py     root@"$ROCKPI_IP":/opt/led-matrix/ || exit 1
echo "  ✓ Files copied"

echo "[2/3] Restarting led-matrix service..."
ssh root@"$ROCKPI_IP" "systemctl restart led-matrix.service" || exit 1
echo "  ✓ Service restarted"

echo "[3/3] Checking service status..."
echo ""
ssh root@"$ROCKPI_IP" "systemctl status led-matrix.service --no-pager -l | head -20"
echo ""

echo "=== Update complete ==="
echo ""
echo "To view live logs, run:"
echo "  ssh root@$ROCKPI_IP"
echo "  sudo journalctl -u led-matrix -f"
echo ""
