#!/bin/bash
# build-and-test.sh - Quick build and test script

set -e

echo "=================================================="
echo "Building RPi C++ LED Matrix Controller"
echo "=================================================="

# Check dependencies
echo "Checking dependencies..."

if ! pkg-config --exists freetype2; then
    echo "⚠  FreeType not found. Install with:"
    echo "   sudo apt install libfreetype6-dev"
    exit 1
fi

if ! pkg-config --exists json; then
    if [ ! -f "/usr/include/nlohmann/json.hpp" ]; then
        echo "⚠  nlohmann-json not found. Install with:"
        echo "   sudo apt install nlohmann-json3-dev"
        exit 1
    fi
fi

if [ ! -f "/usr/local/include/led-matrix.h" ]; then
    echo "⚠  rpi-rgb-led-matrix library not found."
    echo "   Install from: https://github.com/hzeller/rpi-rgb-led-matrix"
    echo ""
    echo "   Quick install:"
    echo "   cd /tmp"
    echo "   git clone https://github.com/hzeller/rpi-rgb-led-matrix.git"
    echo "   cd rpi-rgb-led-matrix"
    echo "   make"
    echo "   sudo make install"
    echo "   sudo ldconfig"
    exit 1
fi

echo "✓ All dependencies found"
echo ""

# Clean previous build
echo "Cleaning previous build..."
make clean 2>/dev/null || true

# Build
echo "Building..."
make

echo ""
echo "✓ Build complete!"
echo ""
echo "Next steps:"
echo "  1. Test: sudo ./led-matrix"
echo "  2. Install: sudo ./install.sh"
echo "  3. Send test command:"
echo "     echo '{\"cmd\":\"text\",\"seg\":0,\"text\":\"HELLO\",\"color\":\"FF0000\"}' | nc -u -w1 <IP> 21324"
echo ""
