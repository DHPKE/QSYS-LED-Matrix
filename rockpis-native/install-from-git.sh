#!/bin/bash
# Install native HUB75 driver on Rock Pi S by cloning from GitHub

set -e

echo "=========================================="
echo "Rock Pi S LED Matrix - Install from Git"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
   echo "Please run as root: sudo bash install-from-git.sh"
   exit 1
fi

# Install git if not present
if ! command -v git &> /dev/null; then
    echo "Installing git..."
    apt update
    apt install -y git
fi

# Clone or pull repository
REPO_DIR="/root/QSYS-LED-Matrix"
if [ -d "$REPO_DIR" ]; then
    echo "Repository exists, pulling latest..."
    cd "$REPO_DIR"
    git pull
else
    echo "Cloning repository..."
    cd /root
    git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
    cd "$REPO_DIR"
fi

echo ""
echo "Repository ready at $REPO_DIR"
echo ""

# Run installation
cd "$REPO_DIR/rockpis-native"
echo "Running installation script..."
bash install.sh

echo ""
echo "=========================================="
echo "Installation complete!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Reboot: reboot"
echo "  2. Start service: systemctl start led-matrix"
echo "  3. Check status: systemctl status led-matrix"
echo "  4. View logs: journalctl -u led-matrix -f"
echo ""
