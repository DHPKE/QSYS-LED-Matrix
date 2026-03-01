#!/bin/bash
# Network diagnostic for LED Matrix

PI_IP="10.1.1.99"
PI_USER="node"
PI_PASS="node"

echo "=== Network Interface Status ==="
sshpass -p "$PI_PASS" ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 "$PI_USER@$PI_IP" << 'EOF'
    echo "--- ip addr show ---"
    ip -4 addr show
    echo ""
    echo "--- ip link show ---"
    ip link show
    echo ""
    echo "--- nmcli connection show ---"
    nmcli connection show
    echo ""
    echo "--- Current routing ---"
    ip route
EOF
