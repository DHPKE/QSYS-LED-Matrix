#!/bin/bash
# install.sh — Install the LED Matrix controller on a fresh RPi Zero 2 W
#
# Run as root:  sudo bash install.sh
#
# What this does:
#   1. Installs system packages (Python 3, Pillow, fonts)
#   2. Builds and installs rpi-rgb-led-matrix Python bindings
#   3. Copies app files to /opt/led-matrix
#   4. Creates config storage directory
#   5. Installs and enables the systemd service

set -e

# Capture script directory FIRST — before any cd commands change $PWD.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== LED Matrix Controller — Install Script ==="
echo ""

# ── Check running as root ──────────────────────────────────────────────────
if [ "$EUID" -ne 0 ]; then
  echo "ERROR: Please run as root:  sudo bash install.sh"
  exit 1
fi

# ── 1. System packages ─────────────────────────────────────────────────────
echo "[1/5] Installing system packages..."
apt-get update -qq
apt-get install -y \
    python3 \
    python3-pip \
    python3-pillow \
    fonts-dejavu-core \
    git \
    build-essential \
    python3-dev \
    cython3

# ── 2. rpi-rgb-led-matrix ──────────────────────────────────────────────────
echo "[2/5] Building rpi-rgb-led-matrix..."
TMPDIR=$(mktemp -d)
git clone --depth 1 https://github.com/hzeller/rpi-rgb-led-matrix.git "$TMPDIR/rpi-rgb-led-matrix"
cd "$TMPDIR/rpi-rgb-led-matrix"
make -j"$(nproc)" HARDWARE_DESC=adafruit-hat 2>/dev/null || make -j"$(nproc)"
cd bindings/python
PYTHON3="$(which python3)"
make build-python PYTHON="$PYTHON3"
make install-python PYTHON="$PYTHON3"
cd /
rm -rf "$TMPDIR"
echo "  ✓ rpi-rgb-led-matrix installed"

# ── 3. Copy app files ──────────────────────────────────────────────────────
echo "[3/5] Copying application files to /opt/led-matrix..."
mkdir -p /opt/led-matrix
cp "$SCRIPT_DIR"/*.py    /opt/led-matrix/
echo "  ✓ Files copied"

# ── 4. Config storage directory ────────────────────────────────────────────
echo "[4/5] Creating config storage..."
mkdir -p /var/lib/led-matrix
echo "  ✓ /var/lib/led-matrix created"

# ── 5. Systemd service ─────────────────────────────────────────────────────
echo "[5/5] Installing systemd service..."
cp "$SCRIPT_DIR/led-matrix.service" /etc/systemd/system/led-matrix.service
systemctl daemon-reload
systemctl enable led-matrix.service
systemctl start  led-matrix.service
echo "  ✓ Service installed and started"

echo ""
echo "=== Installation complete ==="
echo ""
echo "Check status:   sudo systemctl status led-matrix"
echo "View logs:      sudo journalctl -u led-matrix -f"
echo "Stop:           sudo systemctl stop led-matrix"
echo "Web UI:         http://$(hostname -I | awk '{print $1}')/"
echo ""
