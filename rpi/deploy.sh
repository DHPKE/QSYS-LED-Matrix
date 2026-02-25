#!/bin/bash
# deploy.sh — Deploy LED Matrix controller to Raspberry Pi remotely
#
# Usage: ./deploy.sh [IP_ADDRESS]
# Default IP: 10.1.1.25
# Default credentials: user=node, password=node
#
# Requires: sshpass (install with: brew install hudochenkov/sshpass/sshpass on macOS)

set -e

# Configuration
PI_IP="${1:-10.1.1.25}"
PI_USER="node"
PI_PASS="node"
REMOTE_DIR="/opt/led-matrix"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================="
echo "LED Matrix - Remote Deployment"
echo "========================================="
echo "Target: $PI_USER@$PI_IP"
echo "Remote: $REMOTE_DIR"
echo ""

# Check if sshpass is available
if ! command -v sshpass &> /dev/null; then
    echo "ERROR: sshpass is not installed"
    echo "Install with: brew install hudochenkov/sshpass/sshpass"
    exit 1
fi

# Test connection
echo "[1/4] Testing connection..."
if ! sshpass -p "$PI_PASS" ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" 'echo "Connected"' > /dev/null 2>&1; then
    echo "ERROR: Cannot connect to $PI_IP"
    exit 1
fi
echo "  ✓ Connection successful"

# Copy Python files
echo "[2/4] Copying Python files..."
cd "$SCRIPT_DIR"
sshpass -p "$PI_PASS" scp -o StrictHostKeyChecking=no \
    config.py \
    main.py \
    segment_manager.py \
    text_renderer.py \
    udp_handler.py \
    web_server.py \
    "$PI_USER@$PI_IP:/tmp/"
echo "  ✓ Files copied to /tmp"

# Move files to installation directory
echo "[3/4] Installing files..."
sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" << 'EOF'
sudo mv /tmp/config.py /tmp/main.py /tmp/segment_manager.py \
        /tmp/text_renderer.py /tmp/udp_handler.py /tmp/web_server.py \
        /opt/led-matrix/
sudo chown root:root /opt/led-matrix/*.py
sudo chmod 644 /opt/led-matrix/*.py
sudo chmod 755 /opt/led-matrix/main.py
EOF
echo "  ✓ Files installed"

# Restart service
echo "[4/4] Restarting service..."
sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" \
    'sudo systemctl restart led-matrix'
sleep 2
echo "  ✓ Service restarted"

# Show status
echo ""
echo "Deployment complete!"
echo ""
sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" \
    'systemctl status led-matrix --no-pager | head -12'

echo ""
echo "Web UI: http://$PI_IP:8080/"
echo "Logs:   ssh $PI_USER@$PI_IP 'journalctl -u led-matrix -f'"
echo ""
