#!/bin/bash
# Check font status on Pi

PI_IP="10.1.1.25"
PI_USER="node"
PI_PASS="node"

echo "📝 Checking font configuration on $PI_IP..."

sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" << 'EOF'
    echo "Current FONT_PATH setting:"
    grep "FONT_PATH" /opt/led-matrix/config.py
    
    echo ""
    echo "Checking if Arial Bold exists:"
    ls -lh /usr/share/fonts/truetype/msttcorefonts/Arial_Bold.ttf 2>&1
    
    echo ""
    echo "Checking if DejaVu fallback exists:"
    ls -lh /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf 2>&1
    
    echo ""
    echo "Service log (last 30 lines):"
    sudo journalctl -u led-matrix -n 30 --no-pager
EOF
