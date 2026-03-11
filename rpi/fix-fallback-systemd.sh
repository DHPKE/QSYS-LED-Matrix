#!/bin/bash
# Alternative method: Use systemd-networkd instead of NetworkManager
# More reliable for embedded devices like Raspberry Pi

set -e

echo "================================================"
echo "Alternative: systemd-networkd Fallback Config"
echo "================================================"
echo ""

INTERFACE="eth0"
FALLBACK_IP="10.10.10.99"
FALLBACK_CIDR="24"

# Check if systemd-networkd is available
if ! systemctl is-enabled systemd-networkd &> /dev/null; then
    echo "Enabling systemd-networkd..."
    sudo systemctl enable systemd-networkd
fi

# Create network configuration file
NETWORK_FILE="/etc/systemd/network/10-$INTERFACE.network"

echo "Creating $NETWORK_FILE..."

sudo tee "$NETWORK_FILE" > /dev/null <<EOF
[Match]
Name=$INTERFACE

[Network]
# Try DHCP first
DHCP=yes

# If DHCP fails, use fallback
Address=$FALLBACK_IP/$FALLBACK_CIDR
Gateway=10.10.10.1
DNS=8.8.8.8
DNS=8.8.4.4

[DHCP]
# Use DHCP if available
UseDomains=yes
RouteMetric=100

[Address]
# Fallback static IP (always assigned alongside DHCP)
Address=$FALLBACK_IP/$FALLBACK_CIDR
EOF

echo "✅ Configuration file created"
echo ""

# Disable NetworkManager if present (avoid conflicts)
if systemctl is-active NetworkManager &> /dev/null; then
    echo "⚠️  NetworkManager is running. Disabling to avoid conflicts..."
    sudo systemctl stop NetworkManager
    sudo systemctl disable NetworkManager
fi

# Enable and restart systemd-networkd
echo "🔄 Restarting systemd-networkd..."
sudo systemctl restart systemd-networkd
sleep 3

echo ""
echo "✅ systemd-networkd configured!"
echo ""

# Show current IP addresses
echo "Current IP addresses on $INTERFACE:"
ip addr show "$INTERFACE" | grep "inet " | awk '{print "  " $2}'
echo ""

echo "================================================"
echo "Configuration complete!"
echo "Fallback IP: $FALLBACK_IP/$FALLBACK_CIDR"
echo "This will persist across reboots."
echo "================================================"
