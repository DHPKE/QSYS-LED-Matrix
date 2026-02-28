#!/bin/bash
# QUICK_START.sh - One-command setup for fresh Raspbian Lite 64
# This is now just a wrapper around install.sh

cat << 'EOF'
╔══════════════════════════════════════════════════════════════╗
║  RPi C++ LED Matrix Controller - Quick Start                ║
╚══════════════════════════════════════════════════════════════╝

This will install everything from scratch on Raspbian Lite 64:
  • System dependencies (build tools, libraries)
  • rpi-rgb-led-matrix library
  • LED matrix controller (build from source)
  • Systemd service (auto-start on boot)
  • Audio disable (prevents LED flicker)

Requirements:
  - Raspberry Pi (Zero 2W, 3, 4, or 5)
  - Raspbian Lite 64-bit
  - Internet connection
  - Root access (sudo)

Press ENTER to continue or Ctrl+C to abort...
EOF

read

# Just call the full installer
sudo ./install.sh
