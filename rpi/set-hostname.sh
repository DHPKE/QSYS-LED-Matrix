#!/bin/bash
# set-hostname.sh - Properly set hostname with persistence
# Usage: sudo ./set-hostname.sh <new-hostname>

set -e

NEW_HOSTNAME="$1"

if [ -z "$NEW_HOSTNAME" ]; then
    echo "Error: No hostname provided"
    echo "Usage: sudo $0 <new-hostname>"
    exit 1
fi

# Validate hostname
if ! echo "$NEW_HOSTNAME" | grep -qE '^[a-zA-Z0-9-]+$'; then
    echo "Error: Invalid hostname. Use only letters, numbers, and hyphens."
    exit 1
fi

echo "Setting hostname to: $NEW_HOSTNAME"

# 1. Set current hostname (immediate)
hostnamectl set-hostname "$NEW_HOSTNAME"

# 2. Update /etc/hostname (persists across reboots)
echo "$NEW_HOSTNAME" | sudo tee /etc/hostname > /dev/null

# 3. Update /etc/hosts (required for proper name resolution)
# Remove old hostname entries
sudo sed -i "/127.0.1.1/d" /etc/hosts

# Add new hostname entry
echo "127.0.1.1       $NEW_HOSTNAME" | sudo tee -a /etc/hosts > /dev/null

# 4. If using Avahi/mDNS, update avahi configuration
if [ -f /etc/avahi/avahi-daemon.conf ]; then
    sudo sed -i "s/^#*host-name=.*/host-name=$NEW_HOSTNAME/" /etc/avahi/avahi-daemon.conf
    
    # Restart avahi if running
    if systemctl is-active avahi-daemon &> /dev/null; then
        sudo systemctl restart avahi-daemon
    fi
fi

# 5. Verify
CURRENT=$(hostname)
if [ "$CURRENT" = "$NEW_HOSTNAME" ]; then
    echo "✅ Hostname successfully set to: $NEW_HOSTNAME"
    echo "✅ Configuration persisted to /etc/hostname and /etc/hosts"
    echo ""
    echo "The new hostname will be visible after next login."
    echo "For immediate effect in current shell: exec bash"
else
    echo "❌ Failed to set hostname"
    exit 1
fi

exit 0
