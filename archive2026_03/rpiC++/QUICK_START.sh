#!/bin/bash
# QUICK_START.sh - One-command setup for fresh Raspbian Lite 64

cat << 'EOF'
╔══════════════════════════════════════════════════════════════╗
║  RPi C++ LED Matrix Controller - Quick Start                ║
╚══════════════════════════════════════════════════════════════╝

This will install everything from scratch on Raspbian Lite 64:
  • System dependencies (build tools, libraries, fonts)
  • rpi-rgb-led-matrix library
  • LED matrix controller (build from source)
  • Systemd services (auto-start on boot)
  • Web config UI (port 8080)
  • Audio disable (prevents LED flicker)
  • Optional CPU isolation

Requirements:
  - Raspberry Pi (Zero 2W, 3, 4, or 5)
  - Raspbian Lite 64-bit
  - Internet connection
  - Root access (sudo)

Press ENTER to continue or Ctrl+C to abort...
EOF

read

echo ""
echo "Running pre-installation check..."
./pre-install-check.sh

if [ $? -ne 0 ]; then
    echo "❌ Pre-check failed - cannot continue"
    exit 1
fi

echo ""
echo "Starting installation..."
echo ""

# Run the full installer
sudo ./install.sh
