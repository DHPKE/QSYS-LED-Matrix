#!/bin/bash
# validate-install.sh - Verify installation completed successfully

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Installation Validation                                   ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

PASS=0
FAIL=0

check() {
    if eval "$2" > /dev/null 2>&1; then
        echo "✓ $1"
        PASS=$((PASS + 1))
    else
        echo "❌ $1"
        FAIL=$((FAIL + 1))
    fi
}

echo "System Dependencies:"
check "build-essential installed" "dpkg -l | grep -q build-essential"
check "git installed" "command -v git"
check "cmake installed" "command -v cmake"
check "libfreetype6-dev installed" "dpkg -l | grep -q libfreetype6-dev"
check "nlohmann-json3-dev installed" "dpkg -l | grep -q nlohmann-json3-dev"
check "fonts-dejavu-core installed" "dpkg -l | grep -q fonts-dejavu-core"
check "jq installed" "command -v jq"
check "netcat installed" "command -v nc"

echo ""
echo "RGB Matrix Library:"
check "Library headers installed" "test -f /usr/local/include/led-matrix.h"
check "Static library installed" "test -f /usr/local/lib/librgbmatrix.a"
check "Shared library installed" "test -f /usr/local/lib/librgbmatrix.so.1"

echo ""
echo "LED Matrix Controller:"
check "Binary installed" "test -f /usr/local/bin/led-matrix"
check "Binary executable" "test -x /usr/local/bin/led-matrix"
check "Network config script installed" "test -f /usr/local/bin/apply-network-config.sh"
check "Network config script executable" "test -x /usr/local/bin/apply-network-config.sh"

echo ""
echo "Systemd Services:"
check "led-matrix.service exists" "test -f /etc/systemd/system/led-matrix.service"
check "led-matrix.service enabled" "systemctl is-enabled led-matrix"
check "led-matrix-network.service exists" "test -f /etc/systemd/system/led-matrix-network.service"
check "led-matrix-network.service enabled" "systemctl is-enabled led-matrix-network"

echo ""
echo "Configuration:"
check "Config directory exists" "test -d /var/lib/led-matrix"
check "Config directory writable" "test -w /var/lib/led-matrix"
check "Audio disabled in config.txt" "grep -q 'dtparam=audio=off' /boot/config.txt /boot/firmware/config.txt 2>/dev/null"
check "Audio modules blacklisted" "test -f /etc/modprobe.d/blacklist-rgb-matrix.conf"

echo ""
echo "Runtime:"
if systemctl is-active led-matrix > /dev/null 2>&1; then
    echo "✓ Service is running"
    PASS=$((PASS + 1))
    
    # Check if listening on port
    if ss -ulnp | grep -q ":21324"; then
        echo "✓ UDP port 21324 is listening"
        PASS=$((PASS + 1))
    else
        echo "❌ UDP port 21324 not listening"
        FAIL=$((FAIL + 1))
    fi
    
    # Check if web server is up
    if ss -tlnp | grep -q ":8080"; then
        echo "✓ Web server port 8080 is listening"
        PASS=$((PASS + 1))
    else
        echo "❌ Web server port 8080 not listening"
        FAIL=$((FAIL + 1))
    fi
else
    echo "⚠  Service is not running (start with: sudo systemctl start led-matrix)"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "══════════════════════════════════════════════════════════════"
echo "Checks: $PASS passed, $FAIL failed"

if [ $FAIL -eq 0 ]; then
    echo ""
    echo "🎉 Installation fully validated!"
    echo ""
    
    IP=$(hostname -I | awk '{print $1}')
    if [ -n "$IP" ]; then
        echo "Access Points:"
        echo "  UDP Protocol:  $IP:21324"
        echo "  Web Config UI: http://$IP:8080"
        echo ""
        echo "Quick Test:"
        echo "  echo '{\"cmd\":\"text\",\"seg\":0,\"text\":\"WORKS!\"}' | nc -u -w1 $IP 21324"
    fi
    
    exit 0
else
    echo ""
    echo "⚠️  Some checks failed - review output above"
    exit 1
fi
