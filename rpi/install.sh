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

# ── 1. Blacklist snd_bcm2835 (on-board sound conflicts with LED matrix) ────
# rpi-rgb-led-matrix uses the BCM PWM hardware for OE- timing.
# The Pi's on-board sound driver (snd_bcm2835) claims the same peripheral and
# causes the library to hard-exit.  We disable it here.
# NOTE: the app already sets disable_hardware_pulsing=True as a safe fallback,
# but blacklisting the module gives better (hardware-timed) output quality.
echo "[1/6] Blacklisting snd_bcm2835 (on-board sound)..."
cat <<'EOF' | tee /etc/modprobe.d/blacklist-rgb-matrix.conf > /dev/null
# Blacklisted by led-matrix install.sh — conflicts with rpi-rgb-led-matrix PWM
blacklist snd_bcm2835
EOF
# Unload immediately if already loaded (best-effort, may fail if in use)
modprobe -r snd_bcm2835 2>/dev/null || true
# Also disable the dtparam in /boot/firmware/config.txt (bookworm path)
for cfg in /boot/firmware/config.txt /boot/config.txt; do
  if [ -f "$cfg" ]; then
    # Comment out dtparam=audio=on if present
    sed -i 's/^\(dtparam=audio=on\)/#\1  # disabled by led-matrix install/' "$cfg"
    echo "  ✓ Commented out dtparam=audio=on in $cfg"
    break
  fi
done
# Regenerate initramfs so the blacklist takes effect after reboot
if command -v update-initramfs &>/dev/null; then
  update-initramfs -u -k all 2>/dev/null || true
fi
echo "  ✓ snd_bcm2835 blacklisted (reboot to apply fully)"

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
