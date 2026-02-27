#!/bin/bash
# network-config.sh
# Helper script to configure network settings (must be run with sudo)

set -e

ACTION="$1"
IP="$2"
SUBNET="$3"
GATEWAY="$4"

DHCPCD_CONF="/etc/dhcpcd.conf"
BACKUP_CONF="/etc/dhcpcd.conf.backup"

# Backup original if not exists
if [ ! -f "$BACKUP_CONF" ]; then
    cp "$DHCPCD_CONF" "$BACKUP_CONF"
fi

case "$ACTION" in
    static)
        if [ -z "$IP" ] || [ -z "$SUBNET" ]; then
            echo "Error: IP and subnet required for static configuration"
            exit 1
        fi
        
        # Calculate gateway if not provided
        if [ -z "$GATEWAY" ]; then
            GATEWAY=$(echo "$IP" | sed 's/\.[0-9]*$/\.1/')
        fi
        
        # Remove existing static config
        sed -i '/^# Static IP configuration/,/^$/d' "$DHCPCD_CONF"
        sed -i '/^interface eth0$/,/^$/d' "$DHCPCD_CONF"
        
        # Add new static config
        cat >> "$DHCPCD_CONF" << EOF

# Static IP configuration
interface eth0
static ip_address=${IP}/${SUBNET}
static routers=${GATEWAY}
static domain_name_servers=8.8.8.8 1.1.1.1
EOF
        
        echo "Static IP ${IP}/${SUBNET} configured"
        exit 0
        ;;
        
    dhcp)
        # Remove static configuration
        sed -i '/^# Static IP configuration/,/^$/d' "$DHCPCD_CONF"
        sed -i '/^interface eth0$/,/^$/d' "$DHCPCD_CONF"
        
        echo "DHCP enabled"
        exit 0
        ;;
        
    *)
        echo "Usage: $0 {static|dhcp} [ip] [subnet] [gateway]"
        exit 1
        ;;
esac
