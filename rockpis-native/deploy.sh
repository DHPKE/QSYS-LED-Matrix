#!/bin/bash
# Quick deployment script - Run from Mac to deploy to Rock Pi S

set -e

ROCK_PI_IP="${1:-10.1.1.23}"
ROCK_PI_USER="node"
ROCK_PI_PASS="node"

echo "=========================================="
echo "Deploy Native HUB75 Driver to Rock Pi S"
echo "=========================================="
echo ""
echo "Target: $ROCK_PI_USER@$ROCK_PI_IP"
echo ""

# Check if sshpass is installed
if ! command -v sshpass &> /dev/null; then
    echo "Installing sshpass..."
    brew install sshpass
fi

# Check if host is reachable
echo "Testing connection..."
if ! ping -c 1 -W 1 "$ROCK_PI_IP" &> /dev/null; then
    echo "✗ Cannot reach $ROCK_PI_IP"
    exit 1
fi
echo "✓ Host reachable"
echo ""

# Create remote directory
echo "Creating remote directory..."
sshpass -p "$ROCK_PI_PASS" ssh -o StrictHostKeyChecking=no "$ROCK_PI_USER@$ROCK_PI_IP" \
    "mkdir -p ~/led-matrix-native"

# Copy all files
echo "Copying files..."
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
sshpass -p "$ROCK_PI_PASS" scp -o StrictHostKeyChecking=no \
    "$SCRIPT_DIR"/*.py \
    "$SCRIPT_DIR"/*.sh \
    "$SCRIPT_DIR"/README.md \
    "$ROCK_PI_USER@$ROCK_PI_IP:~/led-matrix-native/"

echo "✓ Files copied"
echo ""

# Make install script executable
sshpass -p "$ROCK_PI_PASS" ssh -o StrictHostKeyChecking=no "$ROCK_PI_USER@$ROCK_PI_IP" \
    "cd ~/led-matrix-native && chmod +x *.sh"

echo "=========================================="
echo "Deployment Complete!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. SSH to Rock Pi S:  ssh $ROCK_PI_USER@$ROCK_PI_IP"
echo "  2. Run installer:     cd ~/led-matrix-native && ./install.sh"
echo "  3. Reboot:            sudo reboot"
echo "  4. Start service:     sudo systemctl start led-matrix"
echo "  5. Check logs:        sudo journalctl -u led-matrix -f"
echo ""
echo "Or run installation remotely:"
echo "  sshpass -p '$ROCK_PI_PASS' ssh $ROCK_PI_USER@$ROCK_PI_IP 'cd ~/led-matrix-native && ./install.sh'"
echo ""
