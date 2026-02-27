#!/bin/bash
# deploy-grouping.sh — Deploy grouping feature updates to Raspberry Pi
#
# Usage:
#   ./deploy-grouping.sh [hostname] [username]
#
# Examples:
#   ./deploy-grouping.sh led-matrix.local user
#   ./deploy-grouping.sh 10.1.1.10 pi
#   ./deploy-grouping.sh 192.168.1.100 user

set -e

RPI_HOST="${1:-led-matrix.local}"
RPI_USER="${2:-user}"
REMOTE_DIR="/home/${RPI_USER}/led-matrix"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================="
echo "LED Matrix - Grouping Feature Deployment"
echo "========================================="
echo ""
echo "Target:   ${RPI_USER}@${RPI_HOST}"
echo "Remote:   ${REMOTE_DIR}"
echo ""
echo "This will deploy the grouping feature with:"
echo "  • Group ID configuration (8 groups + broadcast)"
echo "  • Visual group indicator (bottom-left 4×4 pixels)"
echo "  • UDP group filtering"
echo "  • Web UI group selection"
echo ""
read -p "Press Enter to continue or Ctrl+C to cancel..."
echo ""

# Check files exist
REQUIRED_FILES=(
    "config.py"
    "udp_handler.py"
    "text_renderer.py"
    "web_server.py"
    "main.py"
    "segment_manager.py"
)

echo "[1/4] Checking files..."
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$SCRIPT_DIR/$file" ]; then
        echo "  ✗ ERROR: $file not found in $SCRIPT_DIR"
        exit 1
    fi
    echo "  ✓ $file"
done
echo ""

# Copy files to remote Pi
echo "[2/4] Copying updated Python files to ${RPI_HOST}..."
scp -q \
    "$SCRIPT_DIR/config.py" \
    "$SCRIPT_DIR/udp_handler.py" \
    "$SCRIPT_DIR/text_renderer.py" \
    "$SCRIPT_DIR/web_server.py" \
    "$SCRIPT_DIR/main.py" \
    "$SCRIPT_DIR/segment_manager.py" \
    ${RPI_USER}@${RPI_HOST}:${REMOTE_DIR}/

if [ $? -ne 0 ]; then
    echo "  ✗ ERROR: Failed to copy files"
    echo ""
    echo "Troubleshooting:"
    echo "  • Check SSH connection: ssh ${RPI_USER}@${RPI_HOST}"
    echo "  • Verify hostname/IP: ${RPI_HOST}"
    echo "  • Check SSH key setup or try: ssh-copy-id ${RPI_USER}@${RPI_HOST}"
    exit 1
fi
echo "  ✓ Files transferred successfully"
echo ""

# Move files to /opt/led-matrix and restart service
echo "[3/4] Installing files and restarting service on Pi..."
ssh ${RPI_USER}@${RPI_HOST} << 'ENDSSH'
    echo "  → Moving files to /opt/led-matrix..."
    sudo cp ~/led-matrix/*.py /opt/led-matrix/
    
    echo "  → Restarting led-matrix.service..."
    sudo systemctl restart led-matrix.service
    
    echo "  → Waiting for service to start..."
    sleep 3
ENDSSH

if [ $? -ne 0 ]; then
    echo "  ✗ ERROR: Failed to install files or restart service"
    exit 1
fi
echo "  ✓ Service restarted"
echo ""

# Check service status
echo "[4/4] Verifying deployment..."
ssh ${RPI_USER}@${RPI_HOST} << 'ENDSSH'
    if sudo systemctl is-active --quiet led-matrix.service; then
        echo "  ✓ Service is running"
        
        # Get IP address
        IP=$(hostname -I | awk '{print $1}')
        
        echo ""
        echo "========================================="
        echo "Deployment Successful!"
        echo "========================================="
        echo ""
        echo "Grouping Feature Status:"
        echo "  • Visual indicators: Ready"
        echo "  • UDP filtering: Active"
        echo "  • Web UI controls: Available"
        echo ""
        echo "Next Steps:"
        echo "  1. Open Web UI: http://$IP:8080/"
        echo "  2. Go to 'Display Settings' section"
        echo "  3. Select a group (None, 1-8)"
        echo "  4. Look for colored square in bottom-left corner"
        echo ""
        echo "Q-SYS Plugin:"
        echo "  • Use LEDMatrix_v5.qplug for group routing"
        echo "  • Plugin includes 9 group buttons (All, 1-8)"
        echo ""
        echo "Commands:"
        echo "  View logs: sudo journalctl -u led-matrix -f"
        echo "  Restart:   sudo systemctl restart led-matrix"
        echo "  Status:    sudo systemctl status led-matrix"
        echo ""
    else
        echo "  ✗ WARNING: Service failed to start"
        echo ""
        echo "Check logs with:"
        echo "  ssh ${RPI_USER}@${RPI_HOST} 'sudo journalctl -u led-matrix -n 50'"
        exit 1
    fi
ENDSSH

exit $?
