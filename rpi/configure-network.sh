#!/bin/bash
# Helper script to configure network - called by web server with sudo
# Usage: sudo ./configure-network.sh <mode> [ip] [netmask] [gateway] [dns]

MODE="$1"
IP="$2"
NETMASK="$3"
GATEWAY="$4"
DNS="$5"

INTERFACE="eth1"

# Detect connection name for eth1
CONNECTION_NAME=$(nmcli -t -f NAME,DEVICE con show | grep ":$INTERFACE$" | cut -d: -f1)

if [ -z "$CONNECTION_NAME" ]; then
    echo "Error: No NetworkManager connection found for $INTERFACE"
    exit 1
fi

echo "Using connection: $CONNECTION_NAME"

if [ "$MODE" == "dhcp" ]; then
    echo "Configuring $INTERFACE for DHCP..."
    nmcli con mod "$CONNECTION_NAME" ipv4.method auto
    nmcli con mod "$CONNECTION_NAME" ipv4.addresses ""
    nmcli con mod "$CONNECTION_NAME" ipv4.gateway ""
    nmcli con mod "$CONNECTION_NAME" ipv4.dns ""
    echo "DHCP configured. Reboot to apply."
    
elif [ "$MODE" == "static" ]; then
    if [ -z "$IP" ] || [ -z "$NETMASK" ]; then
        echo "Error: IP and netmask required for static mode"
        exit 1
    fi
    
    # Calculate CIDR from netmask
    CIDR=$(python3 -c "import ipaddress; print(ipaddress.IPv4Network('0.0.0.0/$NETMASK').prefixlen)")
    
    echo "Configuring $INTERFACE for static IP: $IP/$CIDR"
    nmcli con mod "$CONNECTION_NAME" ipv4.method manual
    nmcli con mod "$CONNECTION_NAME" ipv4.addresses "$IP/$CIDR"
    
    if [ -n "$GATEWAY" ]; then
        nmcli con mod "$CONNECTION_NAME" ipv4.gateway "$GATEWAY"
    fi
    
    if [ -n "$DNS" ]; then
        nmcli con mod "$CONNECTION_NAME" ipv4.dns "$DNS"
    fi
    
    echo "Static IP configured. Reboot to apply."
else
    echo "Error: Invalid mode '$MODE'. Use 'dhcp' or 'static'."
    exit 1
fi

exit 0
