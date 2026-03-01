#!/bin/bash
# Deploy updated Python files to the Pi and restart service

PI_IP="10.1.1.25"
PI_USER="node"
PI_PASS="node"

echo "📦 Deploying to $PI_USER@$PI_IP..."

# Copy Python files
sshpass -p "$PI_PASS" scp -o StrictHostKeyChecking=no \
    rpi/*.py \
    "$PI_USER@$PI_IP:/tmp/"

# Move files and restart service
sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" << 'EOF'
    echo "🔧 Installing files..."
    sudo cp /tmp/*.py /opt/led-matrix/
    echo "🔄 Restarting service..."
    sudo systemctl restart led-matrix
    echo "✅ Done! Checking status..."
    sleep 2
    sudo systemctl status led-matrix --no-pager -l | head -20
EOF

echo ""
echo "✅ Deployment complete!"
