#!/bin/bash
# deploy-network-helper.sh
# Deploy network configuration helper with proper sudo permissions

PI_IP="${1:-10.1.1.24}"
PI_USER="node"
PI_PASS="node"

echo "Deploying network configuration helper to $PI_IP..."

# Copy the helper script
echo "→ Copying network-config.sh..."
SSHPASS="$PI_PASS" sshpass -e scp -o StrictHostKeyChecking=no \
    network-config.sh ${PI_USER}@${PI_IP}:/tmp/

# Install to /opt/led-matrix and set permissions
echo "→ Installing and setting permissions..."
SSHPASS="$PI_PASS" sshpass -e ssh -o StrictHostKeyChecking=no ${PI_USER}@${PI_IP} << 'REMOTE_COMMANDS'
# Move to production location
sudo mv /tmp/network-config.sh /opt/led-matrix/
sudo chmod +x /opt/led-matrix/network-config.sh
sudo chown root:root /opt/led-matrix/network-config.sh

# Add sudoers entry for the daemon user (LED matrix service runs as daemon)
SUDOERS_LINE="daemon ALL=(ALL) NOPASSWD: /opt/led-matrix/network-config.sh"
echo "$SUDOERS_LINE" | sudo tee /etc/sudoers.d/led-matrix > /dev/null
sudo chmod 0440 /etc/sudoers.d/led-matrix

# Verify sudoers syntax
if ! sudo visudo -c -f /etc/sudoers.d/led-matrix; then
    echo "✗ Sudoers syntax error!"
    sudo rm /etc/sudoers.d/led-matrix
    exit 1
fi

echo "✓ Network helper installed successfully"
echo "✓ Sudoers configured for daemon user"
REMOTE_COMMANDS

# Copy updated web_server.py
echo "→ Deploying updated web_server.py..."
SSHPASS="$PI_PASS" sshpass -e scp -o StrictHostKeyChecking=no \
    web_server.py ${PI_USER}@${PI_IP}:~/QSYS-LED-Matrix/rpi/

SSHPASS="$PI_PASS" sshpass -e ssh -o StrictHostKeyChecking=no ${PI_USER}@${PI_IP} \
    'sudo cp ~/QSYS-LED-Matrix/rpi/web_server.py /opt/led-matrix/'

# Restart service
echo "→ Restarting LED Matrix service..."
SSHPASS="$PI_PASS" sshpass -e ssh -o StrictHostKeyChecking=no ${PI_USER}@${PI_IP} \
    'sudo systemctl restart led-matrix'

echo ""
echo "✅ Deployment complete!"
echo "   WebUI: http://${PI_IP}:8080"
echo "   Network config now works with proper permissions (daemon user)"

