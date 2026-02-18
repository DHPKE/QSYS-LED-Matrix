#!/bin/bash
# Quick fix script - run this on your Rock Pi S

echo "Downloading fixed driver..."
cd /opt/led-matrix-native

# Download the updated driver
sudo curl -o hub75_driver.py.new https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/hub75_driver.py

# Check if download succeeded
if [ -f hub75_driver.py.new ]; then
    # Verify the file is valid Python
    if python3 -m py_compile hub75_driver.py.new 2>/dev/null; then
        echo "✓ File is valid Python"
        sudo mv hub75_driver.py.new hub75_driver.py
        echo "✓ Driver updated"
    else
        echo "✗ Downloaded file is invalid"
        exit 1
    fi
else
    echo "✗ Download failed"
    exit 1
fi

echo ""
echo "Restarting service..."
sudo systemctl restart led-matrix
sleep 2

echo ""
echo "Service status:"
sudo systemctl status led-matrix --no-pager | head -15

echo ""
echo "Recent logs:"
sudo journalctl -u led-matrix -n 20 --no-pager | tail -10
