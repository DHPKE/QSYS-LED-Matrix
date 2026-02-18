#!/bin/bash
# Quick update script to pull the latest config.py fix

echo "Updating config.py on Rock Pi S..."
cd /opt/led-matrix-native
curl -o config.py https://raw.githubusercontent.com/DHPKE/QSYS-LED-Matrix/main/rockpis-native/config.py
echo "Config updated. Restarting service..."
systemctl restart led-matrix
sleep 2
systemctl status led-matrix --no-pager
echo ""
echo "Checking latest logs..."
journalctl -u led-matrix -n 20 --no-pager
