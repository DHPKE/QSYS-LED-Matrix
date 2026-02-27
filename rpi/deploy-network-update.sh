#!/bin/bash
# Deploy network configuration updates to Pi

PI_IP="10.1.1.24"
PI_USER="node"
PI_PASS="node"

echo "Deploying updated web_server.py to LED Matrix Pi..."

sshpass -p "$PI_PASS" scp -o StrictHostKeyChecking=no \
  web_server.py ${PI_USER}@${PI_IP}:~/led-matrix/

echo "Restarting LED Matrix service..."
sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no \
  ${PI_USER}@${PI_IP} 'sudo systemctl restart led-matrix'

echo "Done! WebUI now has static IP/DHCP configuration."
echo "Access at: http://${PI_IP}"
