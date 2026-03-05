#!/bin/bash
# uninstall.sh - Uninstaller for RPi C++ LED Matrix Controller

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  RPi C++ LED Matrix Controller - Uninstaller               ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "⚠  This script needs root privileges"
    echo "   Run: sudo ./uninstall.sh"
    exit 1
fi

echo "This will remove:"
echo "  • Systemd service"
echo "  • Binary from /usr/local/bin"
echo "  • (optionally) Configuration files"
echo "  • (optionally) rpi-rgb-led-matrix library"
echo ""
echo "Continue? (y/n)"
read -r response

if [[ ! "$response" =~ ^[Yy]$ ]]; then
    echo "Cancelled."
    exit 0
fi

echo ""

# ─── 1. Stop and Disable Service ─────────────────────────────────────────────

echo "==[ Step 1/4: Stopping Service ]=="

systemctl stop led-matrix 2>/dev/null || echo "Service not running"
systemctl disable led-matrix 2>/dev/null || echo "Service not enabled"

echo "✓ Service stopped and disabled"
echo ""

# ─── 2. Remove Files ─────────────────────────────────────────────────────────

echo "==[ Step 2/4: Removing Files ]=="

rm -f /etc/systemd/system/led-matrix.service
systemctl daemon-reload
echo "✓ Systemd service removed"

rm -f /usr/local/bin/led-matrix
echo "✓ Binary removed"

echo ""

# ─── 3. Configuration Files ──────────────────────────────────────────────────

echo "==[ Step 3/4: Configuration Files ]=="
echo ""
echo "Remove configuration directory /var/lib/led-matrix? (y/n)"
echo "(Contains saved orientation, group, and brightness settings)"
read -r config_response

if [[ "$config_response" =~ ^[Yy]$ ]]; then
    rm -rf /var/lib/led-matrix
    echo "✓ Configuration removed"
else
    echo "✓ Configuration preserved"
fi

echo ""

# ─── 4. RGB Matrix Library ───────────────────────────────────────────────────

echo "==[ Step 4/4: RGB Matrix Library ]=="
echo ""
echo "Remove rpi-rgb-led-matrix library? (y/n)"
echo "(May be used by other projects)"
read -r lib_response

if [[ "$lib_response" =~ ^[Yy]$ ]]; then
    rm -f /usr/local/include/led-matrix*.h
    rm -f /usr/local/lib/librgbmatrix.*
    ldconfig
    echo "✓ Library removed"
else
    echo "✓ Library preserved"
fi

echo ""

# ─── Summary ─────────────────────────────────────────────────────────────────

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  ✓ Uninstallation Complete!                                ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "The LED matrix controller has been removed."
echo ""
echo "To reinstall: sudo ./install.sh"
echo ""
