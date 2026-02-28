#!/bin/bash
# apply-network-config.sh - Apply saved network configuration

CONFIG_FILE="/var/lib/led-matrix/network-config.json"
DHCPCD_CONF="/etc/dhcpcd.conf"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "No network config found, using system defaults"
    exit 0
fi

MODE=$(jq -r '.mode // "dhcp"' "$CONFIG_FILE")

echo "Applying network config: $MODE"

if [ "$MODE" == "static" ]; then
    STATIC_IP=$(jq -r '.staticIP' "$CONFIG_FILE")
    SUBNET=$(jq -r '.subnet' "$CONFIG_FILE")
    GATEWAY=$(jq -r '.gateway' "$CONFIG_FILE")
    
    if [ "$STATIC_IP" == "null" ] || [ "$SUBNET" == "null" ] || [ "$GATEWAY" == "null" ]; then
        echo "❌ Invalid static config"
        exit 1
    fi
    
    # Calculate CIDR notation
    # Convert subnet mask to CIDR (simple cases)
    case "$SUBNET" in
        "255.255.255.0") CIDR="24" ;;
        "255.255.0.0") CIDR="16" ;;
        "255.0.0.0") CIDR="8" ;;
        *) CIDR="24" ;; # default
    esac
    
    # Remove any existing static config for eth0
    sed -i '/^interface eth0/,/^$/d' "$DHCPCD_CONF"
    
    # Add new static config
    cat >> "$DHCPCD_CONF" << EOF

# Static IP configuration (set via LED Matrix web config)
interface eth0
static ip_address=${STATIC_IP}/${CIDR}
static routers=${GATEWAY}
static domain_name_servers=${GATEWAY}
EOF
    
    echo "✓ Static IP configured: $STATIC_IP"
    
    # Restart networking
    systemctl restart dhcpcd
    
elif [ "$MODE" == "dhcp" ]; then
    # Remove static config if exists
    if grep -q "interface eth0" "$DHCPCD_CONF"; then
        sed -i '/^interface eth0/,/^$/d' "$DHCPCD_CONF"
        echo "✓ DHCP mode enabled (removed static config)"
        systemctl restart dhcpcd
    else
        echo "✓ Already using DHCP"
    fi
fi

exit 0
