#!/bin/bash
# Deploy network fallback fix to Raspberry Pi CM4
# Run this script from your Mac to deploy to the Pi

set -e

PI_IP="10.1.1.26"
PI_USER="user"
FALLBACK_IP="10.10.10.99"

echo "================================================"
echo "Deploying Network Fallback Fix to Raspberry Pi"
echo "================================================"
echo ""

# Try primary IP first
echo "Testing connection to Pi at $PI_IP..."
if /sbin/ping -c 1 -W 2 "$PI_IP" &> /dev/null; then
    echo "✅ Pi is reachable at $PI_IP"
    TARGET_IP="$PI_IP"
elif /sbin/ping -c 1 -W 2 "$FALLBACK_IP" &> /dev/null; then
    echo "⚠️  Primary IP not reachable, using fallback $FALLBACK_IP"
    TARGET_IP="$FALLBACK_IP"
else
    echo "❌ Error: Pi not reachable at $PI_IP or $FALLBACK_IP"
    echo ""
    echo "Please ensure:"
    echo "  1. Pi is powered on"
    echo "  2. Network cable is connected"
    echo "  3. You're on the same network"
    echo ""
    echo "Try manual SSH:"
    echo "  ssh $PI_USER@$PI_IP"
    echo "  ssh $PI_USER@$FALLBACK_IP"
    exit 1
fi

echo ""
echo "📦 Copying fix script to Pi..."
scp rpi/fix-fallback-ip.sh "$PI_USER@$TARGET_IP:/tmp/"

echo ""
echo "🔧 Running fix script on Pi..."
ssh "$PI_USER@$TARGET_IP" "sudo bash /tmp/fix-fallback-ip.sh"

echo ""
echo "================================================"
echo "Deployment Complete!"
echo "================================================"
echo ""
echo "Testing fallback IP..."
sleep 3

if /sbin/ping -c 1 -W 2 "$FALLBACK_IP" &> /dev/null; then
    echo "✅ Fallback IP ($FALLBACK_IP) is now reachable!"
else
    echo "⚠️  Fallback IP not immediately reachable"
    echo "   This may be normal if you're not on the 10.10.10.x network"
    echo "   Try connecting directly or through that network"
fi

echo ""
echo "You can now access the Pi at:"
echo "  ssh $PI_USER@$FALLBACK_IP"
echo "  http://$FALLBACK_IP:8080/"
echo ""
echo "The fallback IP will persist across reboots."
echo "================================================"
