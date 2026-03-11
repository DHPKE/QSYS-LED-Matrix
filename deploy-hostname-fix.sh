#!/bin/bash
# Deploy hostname persistence fix to Raspberry Pi
# Run from Mac: ./deploy-hostname-fix.sh

set -e

PI_IP="10.1.1.21"
PI_USER="node"
PI_PASS="node"

echo "================================================"
echo "Deploying Hostname Persistence Fix to Pi"
echo "================================================"
echo ""

# Check if Pi is reachable
echo "Testing connection to Pi..."
if ! /sbin/ping -c 1 -W 2 "$PI_IP" &> /dev/null; then
    echo "❌ Error: Pi not reachable at $PI_IP"
    echo ""
    echo "Please ensure:"
    echo "  1. Pi is powered on"
    echo "  2. Network cable is connected"
    echo "  3. You're on the same network (10.1.1.x)"
    exit 1
fi

echo "✅ Pi is reachable"
echo ""

# Copy set-hostname.sh script
echo "📦 Copying set-hostname.sh to Pi..."
sshpass -p "$PI_PASS" scp -o StrictHostKeyChecking=no rpi/set-hostname.sh "$PI_USER@$PI_IP:/tmp/"

# Deploy to /opt/led-matrix/
echo "🔧 Installing script..."
sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" << 'ENDSSH'
    # Copy to /opt/led-matrix/
    sudo cp /tmp/set-hostname.sh /opt/led-matrix/
    
    # Set correct permissions
    sudo chmod +x /opt/led-matrix/set-hostname.sh
    sudo chown daemon:daemon /opt/led-matrix/set-hostname.sh
    
    # Verify
    if [ -f /opt/led-matrix/set-hostname.sh ]; then
        echo "✅ Script installed to /opt/led-matrix/set-hostname.sh"
        ls -l /opt/led-matrix/set-hostname.sh
    else
        echo "❌ Failed to install script"
        exit 1
    fi
ENDSSH

echo ""
echo "================================================"
echo "Deployment Complete!"
echo "================================================"
echo ""
echo "The hostname persistence fix has been deployed."
echo ""
echo "To test:"
echo "  1. Open http://$PI_IP:8080/"
echo "  2. Enter a new hostname (e.g., 'led-panel-01')"
echo "  3. Click 'Apply Changes'"
echo "  4. Verify: ssh $PI_USER@$PI_IP 'hostname'"
echo "  5. Reboot: ssh $PI_USER@$PI_IP 'sudo reboot'"
echo "  6. After boot, verify hostname persists"
echo ""
echo "================================================"
