#!/bin/bash
# Install Microsoft TrueType fonts (including Arial) on the Pi

PI_IP="10.1.1.25"
PI_USER="node"
PI_PASS="node"

echo "📦 Installing Microsoft TrueType fonts on $PI_IP..."

sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" << 'EOF'
    echo "📥 Installing ttf-mscorefonts-installer..."
    echo "ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true" | sudo debconf-set-selections
    sudo apt-get update
    sudo apt-get install -y ttf-mscorefonts-installer
    
    echo ""
    echo "🔄 Rebuilding font cache..."
    sudo fc-cache -f -v
    
    echo ""
    echo "✅ Checking Arial installation:"
    ls -lh /usr/share/fonts/truetype/msttcorefonts/Arial*.ttf 2>&1 || echo "❌ Arial not found after install"
    
    echo ""
    echo "🔄 Restarting LED Matrix service..."
    sudo systemctl restart led-matrix
    
    echo ""
    echo "✅ Done! Arial fonts should now be available."
EOF

echo ""
echo "✅ Installation complete!"
