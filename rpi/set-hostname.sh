#!/bin/bash
# Helper script to set hostname - called by web server with sudo
# Usage: sudo ./set-hostname.sh <new-hostname>

if [ $# -ne 1 ]; then
    echo "Usage: $0 <hostname>"
    exit 1
fi

NEW_HOSTNAME="$1"

# Validate hostname (alphanumeric and hyphens only)
if ! [[ "$NEW_HOSTNAME" =~ ^[a-zA-Z0-9-]+$ ]]; then
    echo "Error: Invalid hostname - use only letters, numbers, and hyphens"
    exit 1
fi

# Write to /etc/hostname
echo "$NEW_HOSTNAME" > /etc/hostname

# Update running hostname
/usr/bin/hostname "$NEW_HOSTNAME"

echo "Hostname set to: $NEW_HOSTNAME"
exit 0
