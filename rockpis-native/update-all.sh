#!/bin/bash
# Complete update script for Rock Pi S LED Matrix

echo "========================================"
echo "LED Matrix System Update"
echo "========================================"
echo ""
echo "Downloading all updated files..."
cd /opt/led-matrix-native

# Download all fixed files
echo "  - hub75_driver.py (multi-chip GPIO + max refresh rate)"
sudo curl -s -o hub75_driver.py.new https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/hub75_driver.py

echo "  - segment_manager.py (added get_active_segments)"
sudo curl -s -o segment_manager.py.new https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/segment_manager.py

echo "  - text_renderer.py (bgcolor support, auto-sizing, flashing fix)"
sudo curl -s -o text_renderer.py.new https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/text_renderer.py

echo "  - main.py (improved render timing)"
sudo curl -s -o main.py.new https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/main.py

echo ""
echo "Validating and installing files..."
SUCCESS=0
for file in hub75_driver segment_manager text_renderer main; do
    if [ -f ${file}.py.new ]; then
        if sudo python3 -m py_compile ${file}.py.new 2>/dev/null; then
            sudo mv ${file}.py.new ${file}.py
            echo "  ✓ ${file}.py updated"
            SUCCESS=$((SUCCESS + 1))
        else
            echo "  ✗ ${file}.py validation failed"
            sudo rm -f ${file}.py.new
        fi
    else
        echo "  ✗ ${file}.py download failed"
    fi
done

echo ""
echo "Files updated: $SUCCESS/4"

if [ $SUCCESS -eq 0 ]; then
    echo "❌ No files were updated. Check your internet connection."
    exit 1
fi

echo ""
echo "Restarting service..."
sudo systemctl restart led-matrix
sleep 3

echo ""
echo "========================================"
echo "Service Status:"
echo "========================================"
sudo systemctl status led-matrix --no-pager | head -12

echo ""
echo "========================================"
echo "Recent Logs:"
echo "========================================"
sudo journalctl -u led-matrix -n 10 --no-pager | tail -10

echo ""
echo "========================================"
IP=$(hostname -I | awk '{print $1}')
if systemctl is-active --quiet led-matrix; then
    echo "✓ Service is running!"
    echo ""
    echo "Access your LED matrix:"
    echo "  • Web UI: http://$IP"
    echo "  • UDP Port: 21324"
else
    echo "⚠ Service is not running"
    echo "Check logs above for errors"
fi
echo "========================================"
