#!/bin/bash
# Helper script to reboot device - called by web server with sudo
# Usage: sudo ./reboot-device.sh

echo "Rebooting device..."
/usr/sbin/reboot
exit 0
