#!/bin/bash
# pre-install-check.sh - Verify all required files before installation

set -e

REQUIRED_FILES=(
    "install.sh"
    "Makefile"
    "config.h"
    "main.cpp"
    "segment_manager.h"
    "segment_manager.cpp"
    "udp_handler.h"
    "udp_handler.cpp"
    "text_renderer.h"
    "text_renderer.cpp"
    "web_server.h"
    "web_server.cpp"
    "led-matrix.service"
    "led-matrix-network.service"
    "apply-network-config.sh"
)

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Pre-Installation Check                                    ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

MISSING=0

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "❌ MISSING: $file"
        MISSING=$((MISSING + 1))
    fi
done

echo ""

if [ $MISSING -gt 0 ]; then
    echo "❌ $MISSING required file(s) missing!"
    echo "   Cannot proceed with installation."
    exit 1
fi

echo "✓ All required files present"
echo ""
echo "Ready to install!"
echo ""
echo "Run: sudo ./install.sh"
echo ""
