#!/bin/bash
# optimize-system.sh — System-level optimizations for LED matrix performance
# Run once after initial setup: sudo ./optimize-system.sh

set -e

echo "═══════════════════════════════════════════════════════════"
echo "  LED Matrix System Optimization for Raspberry Pi"
echo "═══════════════════════════════════════════════════════════"
echo

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "❌ This script must be run as root (use sudo)"
    exit 1
fi

echo "1️⃣  Disabling Desktop GUI (unnecessary for headless operation)..."
if systemctl get-default | grep -q "graphical.target"; then
    systemctl set-default multi-user.target
    echo "   ✓ Desktop GUI will be disabled on next reboot"
else
    echo "   ✓ Desktop GUI already disabled"
fi

echo
echo "2️⃣  Optimizing swappiness (reduce swap usage for better timing)..."
if ! grep -q "vm.swappiness" /etc/sysctl.conf; then
    echo "vm.swappiness=10" >> /etc/sysctl.conf
    sysctl -w vm.swappiness=10
    echo "   ✓ Swappiness set to 10 (was 60)"
else
    echo "   ✓ Swappiness already configured"
fi

echo
echo "3️⃣  Disabling unnecessary services..."
SERVICES_TO_DISABLE=(
    "triggerhappy.service"
    "avahi-daemon.service"
    "bluetooth.service"
)

for service in "${SERVICES_TO_DISABLE[@]}"; do
    if systemctl is-enabled "$service" 2>/dev/null | grep -q "enabled"; then
        systemctl disable "$service" 2>/dev/null || true
        systemctl stop "$service" 2>/dev/null || true
        echo "   ✓ Disabled $service"
    fi
done

echo
echo "4️⃣  Configuring boot parameters for better performance..."
CMDLINE="/boot/cmdline.txt"
if [ ! -f "$CMDLINE" ]; then
    CMDLINE="/boot/firmware/cmdline.txt"
fi

if [ -f "$CMDLINE" ]; then
    # Backup original
    if [ ! -f "${CMDLINE}.backup" ]; then
        cp "$CMDLINE" "${CMDLINE}.backup"
        echo "   ✓ Backed up $CMDLINE"
    fi
    
    # Add isolcpus if not present (reserve CPU 3 for LED matrix)
    if ! grep -q "isolcpus=" "$CMDLINE"; then
        sed -i 's/$/ isolcpus=3/' "$CMDLINE"
        echo "   ✓ Added isolcpus=3 to reserve CPU core for LED matrix"
    else
        echo "   ✓ CPU isolation already configured"
    fi
else
    echo "   ⚠️  Could not find cmdline.txt"
fi

echo
echo "5️⃣  Optimizing filesystem (noatime for reduced disk I/O)..."
if ! grep -q "noatime" /etc/fstab; then
    sed -i 's/defaults/defaults,noatime/' /etc/fstab
    echo "   ✓ Added noatime to filesystem mounts"
else
    echo "   ✓ Filesystem already optimized"
fi

echo
echo "6️⃣  Updating LED matrix service with priority scheduling..."
cp /opt/led-matrix/led-matrix.service /etc/systemd/system/led-matrix.service
systemctl daemon-reload
echo "   ✓ Service configuration updated"

echo
echo "═══════════════════════════════════════════════════════════"
echo "✅ Optimization complete!"
echo
echo "Next steps:"
echo "  1. Reboot the system: sudo reboot"
echo "  2. After reboot, check CPU usage: htop"
echo "  3. Verify service: sudo systemctl status led-matrix"
echo
echo "Expected improvements:"
echo "  • CPU usage: ~60% → ~30-40%"
echo "  • Memory pressure: reduced (less swap)"
echo "  • Display flicker: significantly reduced"
echo "  • Response time: improved"
echo "═══════════════════════════════════════════════════════════"
