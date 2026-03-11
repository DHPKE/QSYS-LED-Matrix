#!/bin/bash
# Fix and configure fallback IP for Raspberry Pi CM4
# Date: 2026-03-11
# Author: DHPKE

set -e

echo "================================================"
echo "Network Fallback IP Configuration Fix"
echo "================================================"
echo ""

# Configuration
FALLBACK_IP="10.10.10.99"
FALLBACK_CIDR="24"
FALLBACK_GATEWAY="10.10.10.1"
FALLBACK_DNS="8.8.8.8,8.8.4.4"

# Detect primary ethernet interface
INTERFACE=$(ip -o link show | grep -E "eth[0-9]|enp[0-9]" | head -1 | awk '{print $2}' | sed 's/://')

if [ -z "$INTERFACE" ]; then
    echo "❌ Error: No ethernet interface found!"
    exit 1
fi

echo "✅ Detected interface: $INTERFACE"
echo ""

# Check if NetworkManager is installed
if ! command -v nmcli &> /dev/null; then
    echo "⚠️  NetworkManager not installed. Installing..."
    sudo apt-get update
    sudo apt-get install -y network-manager
    
    # Disable dhcpcd if present
    if systemctl is-enabled dhcpcd &> /dev/null; then
        echo "Disabling dhcpcd (conflicts with NetworkManager)..."
        sudo systemctl stop dhcpcd
        sudo systemctl disable dhcpcd
    fi
    
    # Enable and start NetworkManager
    sudo systemctl enable NetworkManager
    sudo systemctl start NetworkManager
    sleep 3
fi

echo "✅ NetworkManager is installed"
echo ""

# Detect connection name
CONNECTION_NAME=$(nmcli -t -f NAME,DEVICE connection show | grep ":$INTERFACE$" | cut -d: -f1 | head -1)

if [ -z "$CONNECTION_NAME" ]; then
    echo "⚠️  No connection found for $INTERFACE. Creating one..."
    sudo nmcli connection add type ethernet ifname "$INTERFACE" con-name "Wired connection 1"
    CONNECTION_NAME="Wired connection 1"
    sleep 2
fi

echo "✅ Using connection: $CONNECTION_NAME"
echo ""

# Get current configuration
CURRENT_METHOD=$(nmcli -t -f ipv4.method connection show "$CONNECTION_NAME")
CURRENT_ADDRESSES=$(nmcli -t -f ipv4.addresses connection show "$CONNECTION_NAME")

echo "Current configuration:"
echo "  Method: $CURRENT_METHOD"
echo "  Addresses: $CURRENT_ADDRESSES"
echo ""

# Configure DHCP + fallback
echo "🔧 Configuring DHCP with static fallback..."

# Set method to auto (DHCP)
sudo nmcli connection modify "$CONNECTION_NAME" ipv4.method auto

# Remove any existing addresses first
sudo nmcli connection modify "$CONNECTION_NAME" ipv4.addresses ""

# Add fallback static IP
sudo nmcli connection modify "$CONNECTION_NAME" +ipv4.addresses "${FALLBACK_IP}/${FALLBACK_CIDR}"

# Set gateway (optional for fallback network)
sudo nmcli connection modify "$CONNECTION_NAME" ipv4.gateway "${FALLBACK_GATEWAY}"

# Set DNS
sudo nmcli connection modify "$CONNECTION_NAME" ipv4.dns "${FALLBACK_DNS}"

# Allow connection to work even if DHCP fails
sudo nmcli connection modify "$CONNECTION_NAME" ipv4.may-fail no

# Restart connection
echo ""
echo "🔄 Restarting network connection..."
sudo nmcli connection down "$CONNECTION_NAME" 2>/dev/null || true
sleep 1
sudo nmcli connection up "$CONNECTION_NAME"
sleep 3

echo ""
echo "================================================"
echo "Configuration Complete!"
echo "================================================"
echo ""

# Verify configuration
echo "New configuration:"
nmcli -f ipv4.method,ipv4.addresses,ipv4.gateway,ipv4.dns connection show "$CONNECTION_NAME" | grep -E "ipv4\.(method|addresses|gateway|dns):"
echo ""

# Show current IP addresses
echo "Current IP addresses on $INTERFACE:"
ip addr show "$INTERFACE" | grep "inet " | awk '{print "  " $2}'
echo ""

# Test connectivity
echo "Testing connectivity..."

# Test fallback IP
if ping -c 1 -W 1 "$FALLBACK_IP" &> /dev/null; then
    echo "  ✅ Fallback IP ($FALLBACK_IP) is reachable"
else
    echo "  ⚠️  Fallback IP ($FALLBACK_IP) not responding (may be normal if no device on that network)"
fi

# Show DHCP IP if present
DHCP_IP=$(ip -4 addr show "$INTERFACE" | grep "inet " | grep -v "$FALLBACK_IP" | awk '{print $2}' | cut -d/ -f1 | head -1)
if [ -n "$DHCP_IP" ]; then
    echo "  ✅ DHCP IP ($DHCP_IP) is active"
    if ping -c 1 -W 1 "$DHCP_IP" &> /dev/null; then
        echo "  ✅ DHCP IP ($DHCP_IP) is reachable"
    fi
else
    echo "  ⚠️  No DHCP IP assigned (expected if no DHCP server)"
fi

echo ""
echo "================================================"
echo "Summary"
echo "================================================"
echo "Primary IP (DHCP): Will be assigned by DHCP server"
echo "Fallback IP: $FALLBACK_IP/$FALLBACK_CIDR (always active)"
echo ""
echo "To access the Pi when DHCP fails:"
echo "  ssh user@$FALLBACK_IP"
echo "  http://$FALLBACK_IP:8080/"
echo ""
echo "Configuration persists across reboots."
echo "================================================"
