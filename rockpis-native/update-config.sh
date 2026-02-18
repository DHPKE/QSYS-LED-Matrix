#!/bin/bash
# Quick update script to pull the latest fixes

echo "Updating files on Rock Pi S..."
cd /opt/led-matrix-native
curl -o config.py https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/config.py
curl -o hub75_driver.py https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/hub75_driver.py
curl -o main.py https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/main.py
echo "Files updated. Restarting service..."
systemctl restart led-matrix
sleep 2
systemctl status led-matrix --no-pager
echo ""
echo "Checking latest logs..."
journalctl -u led-matrix -n 30 --no-pager
