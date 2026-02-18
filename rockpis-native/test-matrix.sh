#!/bin/bash
# Test script for LED Matrix functionality
# Run this on your Rock Pi S to verify everything works

echo "========================================"
echo "LED Matrix Test Script"
echo "========================================"
echo ""

# Get IP address
IP=$(hostname -I | awk '{print $1}')
echo "Your Rock Pi S IP: $IP"
echo ""

echo "1. Checking service status..."
systemctl status led-matrix --no-pager | head -10
echo ""

echo "2. Checking if web server is responsive..."
if curl -s http://localhost/api/config > /dev/null; then
    echo "✓ Web server is running!"
    echo ""
    echo "API Config:"
    curl -s http://localhost/api/config | python3 -m json.tool
    echo ""
else
    echo "✗ Web server not responding"
    echo "Check logs: journalctl -u led-matrix -n 50"
    exit 1
fi

echo "3. Checking segments..."
curl -s http://localhost/api/segments | python3 -m json.tool
echo ""

echo "4. Testing UDP command (segment 1: 'Hello')..."
echo '{"cmd":"text","seg":0,"text":"Hello","color":"FF0000","bgcolor":"000000","align":"L"}' | nc -u -w1 localhost 21324
sleep 1
echo ""

echo "5. Testing UDP command (segment 2: 'World')..."
echo '{"cmd":"text","seg":1,"text":"World","color":"00FF00","bgcolor":"000000","align":"L"}' | nc -u -w1 localhost 21324
sleep 1
echo ""

echo "========================================"
echo "Tests complete!"
echo ""
echo "To access the web UI:"
echo "  Open a browser to: http://$IP"
echo ""
echo "To send UDP commands from Q-SYS:"
echo "  IP: $IP"
echo "  Port: 21324"
echo "========================================"
