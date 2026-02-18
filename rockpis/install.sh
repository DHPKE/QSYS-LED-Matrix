#!/bin/bash
# install.sh — Install the LED Matrix controller on a RADXA Rock Pi S
#              running Armbian (Bookworm / Jammy recommended)
#
# Run as root:  sudo bash install.sh
#
# What this does:
#   0. Pre-flight checks (Armbian, UART console warning)
#   1. Disable serial console on UART0 (pins 8/10 used by HUB75 C/D lines)
#   2. Install system packages (Python 3, Pillow, fonts, build tools)
#   3. Build and install rpi-rgb-led-matrix Python bindings
#   4. Copy app files to /opt/led-matrix
#   5. Create config storage directory
#   6. Install and enable the systemd service

set -e

# Capture script directory FIRST — before any cd commands change $PWD.
# ${BASH_SOURCE[0]} is the path to this script file regardless of CWD.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== LED Matrix Controller — Rock Pi S Install Script ==="
echo ""

# ── Check running as root ──────────────────────────────────────────────────
if [ "$EUID" -ne 0 ]; then
  echo "ERROR: Please run as root:  sudo bash install.sh"
  exit 1
fi

# ── 0. Pre-flight ──────────────────────────────────────────────────────────
echo "[0/6] Pre-flight checks..."

# Warn if not Armbian
if [ ! -f /etc/armbian-release ]; then
  echo "  ⚠  /etc/armbian-release not found."
  echo "     This script is written for Armbian. Other distros may work but"
  echo "     you may need to adjust the serial console disable step manually."
  read -p "  Continue anyway? [y/N] " confirm
  [[ "$confirm" =~ ^[Yy]$ ]] || exit 1
fi

echo "  ✓ Pre-flight passed"

# ── 1. Disable UART0 serial console ───────────────────────────────────────
# GPIO0_B5 (Linux 13) and GPIO0_B6 (Linux 14) are used as HUB75 C and D
# address lines respectively.  These same pins are UART0 TX/RX which Armbian
# uses as the default serial console.  We must disable it before the GPIO
# pins can be used freely.
#
# Method A — systemd (runtime, takes effect immediately):
echo "[1/6] Disabling UART0 serial console (pins 8 and 10 needed for HUB75)..."
systemctl disable --now serial-getty@ttyS0 2>/dev/null || true
systemctl disable --now serial-getty@ttyFIQ0 2>/dev/null || true

# Method B — Armbian boot args (survives reboot):
# Remove console=ttyS0,... from /boot/armbianEnv.txt
if [ -f /boot/armbianEnv.txt ]; then
  if grep -q "console=ttyS0" /boot/armbianEnv.txt; then
    sed -i 's/console=ttyS0,[0-9]*//' /boot/armbianEnv.txt
    sed -i 's/  */ /g; s/^ //; s/ $//' /boot/armbianEnv.txt
    echo "  ✓ Removed ttyS0 from /boot/armbianEnv.txt"
  else
    echo "  ✓ ttyS0 not present in /boot/armbianEnv.txt (already clean)"
  fi
else
  echo "  ⚠  /boot/armbianEnv.txt not found — skip boot-arg edit"
  echo "     Make sure serial console is not using pins 8/10 after reboot."
fi

# Remove UART0 device-tree overlay if present
if grep -q "overlays=.*uart" /boot/armbianEnv.txt 2>/dev/null; then
  sed -i 's/uart[0-9]\{1,2\}[a-z]*//' /boot/armbianEnv.txt
  echo "  ✓ Removed uart overlay from /boot/armbianEnv.txt"
fi

echo "  ✓ UART0 console disabled"

# ── 2. System packages ─────────────────────────────────────────────────────
echo "[2/6] Installing system packages..."
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

# ── 3. rpi-rgb-led-matrix ──────────────────────────────────────────────────
echo "[3/6] Building rpi-rgb-led-matrix..."
echo "      (This takes 2-3 minutes on the Rock Pi S — please wait)"
TMPDIR=$(mktemp -d)
git clone --depth 1 https://github.com/hzeller/rpi-rgb-led-matrix.git "$TMPDIR/rpi-rgb-led-matrix"
cd "$TMPDIR/rpi-rgb-led-matrix"

# Build without hardware-specific HARDWARE_DESC so it uses generic GPIO access
# via /dev/mem (works on RK3308 / Armbian).
make -j"$(nproc)"

cd bindings/python
PYTHON3="$(which python3)"
make build-python PYTHON="$PYTHON3"
make install-python PYTHON="$PYTHON3"
cd /
rm -rf "$TMPDIR"
echo "  ✓ rpi-rgb-led-matrix installed"

# ── 4. Copy app files ──────────────────────────────────────────────────────
echo "[4/6] Copying application files to /opt/led-matrix..."
mkdir -p /opt/led-matrix
cp "$SCRIPT_DIR"/*.py    /opt/led-matrix/
echo "  ✓ Files copied"

# ── 5. Config storage directory ────────────────────────────────────────────
echo "[5/6] Creating config storage..."
mkdir -p /var/lib/led-matrix
echo "  ✓ /var/lib/led-matrix created"

# ── 6. Systemd service ─────────────────────────────────────────────────────
echo "[6/6] Installing systemd service..."
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
echo "⚠  IMPORTANT: A reboot is required to fully apply the UART0"
echo "   console disable.  The service is already running — after"
echo "   rebooting, the HUB75 address lines on pins 8/10 will work."
echo ""
echo "   sudo reboot"
echo ""
