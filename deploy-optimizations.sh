#!/bin/bash
# deploy-optimizations.sh - Deploy optimized code to Raspberry Pi
# Usage: bash deploy-optimizations.sh [pi_ip] [pi_user] [pi_password]

set -e

# Default values
PI_IP="${1:-10.1.1.21}"
PI_USER="${2:-node}"
PI_PASS="${3:-node}"

echo "========================================="
echo "LED Matrix Optimization Deployment"
echo "========================================="
echo "Target: $PI_USER@$PI_IP"
echo ""

# Check if Pi is reachable
echo "[1/7] Checking Pi connectivity..."
if /sbin/ping -c 2 "$PI_IP" > /dev/null 2>&1; then
    echo "✓ Pi is reachable at $PI_IP"
else
    echo "✗ ERROR: Pi is not reachable at $PI_IP"
    echo "Please check:"
    echo "  - Pi is powered on"
    echo "  - Network connection is working"
    echo "  - IP address is correct"
    exit 1
fi

# Check if sshpass is available
if ! command -v sshpass &> /dev/null; then
    echo "✗ ERROR: sshpass is not installed"
    echo "Install with: brew install sshpass"
    exit 1
fi

echo ""
echo "[2/7] Copying optimized files to Pi..."
sshpass -p "$PI_PASS" scp rpi/main.py "$PI_USER@$PI_IP:/tmp/main.py.optimized" || {
    echo "✗ Failed to copy main.py"
    exit 1
}
sshpass -p "$PI_PASS" scp rpi/text_renderer.py "$PI_USER@$PI_IP:/tmp/text_renderer.py.optimized" || {
    echo "✗ Failed to copy text_renderer.py"
    exit 1
}
echo "✓ Files copied to /tmp/"

echo ""
echo "[3/7] Creating backup of current files..."
sshpass -p "$PI_PASS" ssh "$PI_USER@$PI_IP" << 'EOF'
    BACKUP_DIR="/opt/led-matrix/backup-$(date +%Y%m%d-%H%M%S)"
    sudo mkdir -p "$BACKUP_DIR"
    sudo cp /opt/led-matrix/main.py "$BACKUP_DIR/"
    sudo cp /opt/led-matrix/text_renderer.py "$BACKUP_DIR/"
    echo "✓ Backup created at: $BACKUP_DIR"
EOF

echo ""
echo "[4/7] Deploying optimized files..."
sshpass -p "$PI_PASS" ssh "$PI_USER@$PI_IP" << 'EOF'
    sudo cp /tmp/main.py.optimized /opt/led-matrix/main.py
    sudo cp /tmp/text_renderer.py.optimized /opt/led-matrix/text_renderer.py
    sudo chown root:root /opt/led-matrix/main.py /opt/led-matrix/text_renderer.py
    sudo chmod 644 /opt/led-matrix/main.py /opt/led-matrix/text_renderer.py
    echo "✓ Files deployed to /opt/led-matrix/"
EOF

echo ""
echo "[5/7] Validating Python syntax..."
sshpass -p "$PI_PASS" ssh "$PI_USER@$PI_IP" << 'EOF'
    python3 -m py_compile /opt/led-matrix/main.py || {
        echo "✗ Syntax error in main.py!"
        exit 1
    }
    python3 -m py_compile /opt/led-matrix/text_renderer.py || {
        echo "✗ Syntax error in text_renderer.py!"
        exit 1
    }
    echo "✓ Syntax validation passed"
EOF

echo ""
echo "[6/7] Restarting LED Matrix service..."
sshpass -p "$PI_PASS" ssh "$PI_USER@$PI_IP" << 'EOF'
    sudo systemctl restart led-matrix
    sleep 2
    if sudo systemctl is-active --quiet led-matrix; then
        echo "✓ Service restarted successfully"
    else
        echo "✗ Service failed to start!"
        echo "Check logs: sudo journalctl -u led-matrix -n 50"
        exit 1
    fi
EOF

echo ""
echo "[7/7] Checking service status and logs..."
sshpass -p "$PI_PASS" ssh "$PI_USER@$PI_IP" << 'EOF'
    sudo systemctl status led-matrix --no-pager | head -15
    echo ""
    echo "Recent logs:"
    sudo journalctl -u led-matrix -n 20 --no-pager | grep -E "FONT|NET-MON|RENDER|Prewarmed"
EOF

echo ""
echo "========================================="
echo "✓ Deployment Complete!"
echo "========================================="
echo ""
echo "Optimizations Applied:"
echo "  - Test mode polling: 90% reduction (60/sec → 6/sec)"
echo "  - Network monitor: Exponential backoff"
echo "  - Watchdog check: 90% reduction"
echo "  - Render loop: Adaptive timing"
echo "  - Color cache: 15 colors prepopulated"
echo "  - Font cache: Prewarmed common sizes"
echo ""
echo "Expected Improvement: 15-25% CPU reduction"
echo ""
echo "Monitor CPU usage:"
echo "  ssh $PI_USER@$PI_IP"
echo "  top -d 1 | grep python3"
echo ""
echo "View logs:"
echo "  sudo journalctl -u led-matrix -f"
echo ""
echo "Rollback if needed:"
echo "  See backup directory on Pi"
echo "========================================="
