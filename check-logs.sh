#!/bin/bash
# Check service logs

PI_IP="10.1.1.99"
PI_USER="node"
PI_PASS="node"

sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no "$PI_USER@$PI_IP" << 'EOF'
    echo "Last 50 lines of service log:"
    sudo journalctl -u led-matrix -n 50 --no-pager
EOF
