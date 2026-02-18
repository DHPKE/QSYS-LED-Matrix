#!/bin/bash
# install.sh — Install the LED Matrix controller on a RADXA Rock Pi S
#              running Armbian (Bookworm / Jammy recommended)
#
# Run WITHOUT sudo:  bash install.sh
# (Script will prompt for sudo when needed)
#
# What this does:
#   0. Pre-flight checks (Armbian, UART console warning)
#   1. Install system packages (Python 3, Pillow, fonts, build tools)
#   2. Clone and build rpi-rgb-led-matrix library with Rock Pi S hardware mapping
#   3. Install Python bindings
#   4. Copy app files to /opt/led-matrix
#   5. Install and enable the systemd service
#   6. Disable serial console on UART0 (pins 8/10 used by HUB75 C/D lines)
#   7. Remove NO_DISPLAY override if present
#   8. Start service

set -e

# Capture script directory FIRST — before any cd commands change $PWD.
# ${BASH_SOURCE[0]} is the path to this script file regardless of CWD.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================="
echo "Rock Pi S LED Matrix - Installation"
echo "========================================="
echo ""

# ── Check NOT running as root ──────────────────────────────────────────────
if [ "$EUID" -eq 0 ]; then
  echo "ERROR: Do not run this script as root (don't use sudo)"
  echo "The script will prompt for sudo when needed"
  exit 1
fi

# ── 0. Pre-flight ──────────────────────────────────────────────────────────
echo "[0/8] Pre-flight checks..."

# Check hardware
if [ ! -f /proc/device-tree/model ]; then
    echo "  ⚠  Cannot detect hardware model"
else
    MODEL=$(cat /proc/device-tree/model 2>/dev/null | tr -d '\0' || echo "unknown")
    echo "  Detected: $MODEL"
fi

# Warn if not Armbian
if [ ! -f /etc/armbian-release ]; then
  echo "  ⚠  /etc/armbian-release not found."
  echo "     This script is written for Armbian. Other distros may work but"
  echo "     you may need to adjust the serial console disable step manually."
  read -p "  Continue anyway? [y/N] " confirm
  [[ "$confirm" =~ ^[Yy]$ ]] || exit 1
fi

echo "  ✓ Pre-flight passed"
echo ""

# ── 1. System packages ─────────────────────────────────────────────────────
echo "[1/8] Installing system packages..."
sudo apt-get update -qq
sudo apt-get install -y \
    python3 \
    python3-dev \
    python3-pillow \
    fonts-dejavu-core \
    git \
    build-essential \
    cython3 \
    libgraphicsmagick++-dev \
    libwebp-dev
echo "  ✓ Dependencies installed"
echo ""

# ── 2. Clone rpi-rgb-led-matrix library ────────────────────────────────────
echo "[2/8] Installing rpi-rgb-led-matrix library..."

if [ -d ~/rpi-rgb-led-matrix ]; then
    echo "  ℹ  Library already exists at ~/rpi-rgb-led-matrix"
    read -p "  Update existing library? [y/N]: " UPDATE_LIB
    if [[ "$UPDATE_LIB" =~ ^[Yy]$ ]]; then
        cd ~/rpi-rgb-led-matrix
        git pull
        echo "  ✓ Library updated"
    fi
else
    echo "  Cloning rpi-rgb-led-matrix library..."
    cd ~
    git clone https://github.com/hzeller/rpi-rgb-led-matrix.git
    echo "  ✓ Library cloned"
fi
echo ""

# ── 3. Apply Rock Pi S hardware mapping ────────────────────────────────────
echo "[3/8] Applying Rock Pi S hardware mapping..."
HWMAP_C=~/rpi-rgb-led-matrix/lib/hardware-mapping.c

# Backup original
if [ ! -f "$HWMAP_C.backup" ]; then
    echo "  Creating backup of hardware-mapping.c..."
    cp "$HWMAP_C" "$HWMAP_C.backup"
    echo "  ✓ Backup saved"
fi

# Check if already patched
if grep -q "rockpis_mapping" "$HWMAP_C"; then
    echo "  ℹ  Rock Pi S mapping already present"
    echo "     Removing old version..."
    cp "$HWMAP_C.backup" "$HWMAP_C"
fi

# Find insertion point
LINE_NUM=$(grep -n "static HardwareMapping \*hardware_mappings" "$HWMAP_C" | head -1 | cut -d: -f1)

if [ -z "$LINE_NUM" ]; then
    echo "  ERROR: Could not find hardware_mappings array"
    exit 1
fi

# Insert the mapping
echo "  Patching hardware-mapping.c..."
{
    head -n $((LINE_NUM - 1)) "$HWMAP_C"
    cat << 'MAPPING_END'

// ========== Rock Pi S Hardware Mapping (RK3308) ==========
// All pins on Header 1 (26-pin GPIO)
// GPIO numbers corrected per CORRECTED_PINOUT.md
// See: https://docs.radxa.com/en/rock-pi-s/hardware/rock-pi-s
static struct HardwareMapping rockpis_mapping = {
  .name          = "rockpis",
  
  // Control signals
  .output_enable = 54,  // OE  - GPIO1_C6 - Header 1 Pin 21
  .clock         = 71,  // CLK - GPIO2_A7 - Header 1 Pin 22
  .strobe        = 55,  // LAT - GPIO1_C7 - Header 1 Pin 19
  
  // Row address pins
  .a             = 11,  // GPIO0_B3 - Header 1 Pin 3
  .b             = 12,  // GPIO0_B4 - Header 1 Pin 5
  .c             = 65,  // GPIO2_A1 - Header 1 Pin 8  (UART0_TX - disable console!)
  .d             = 64,  // GPIO2_A0 - Header 1 Pin 10 (UART0_RX - disable console!)
  .e             = -1,  // Not used for 32px height (1/16 scan)
  
  // Upper half RGB
  .p0_r1         = 16,  // GPIO0_C0 - Header 1 Pin 13
  .p0_g1         = 17,  // GPIO0_C1 - Header 1 Pin 15
  .p0_b1         = 15,  // GPIO0_B7 - Header 1 Pin 11
  
  // Lower half RGB
  .p0_r2         = 68,  // GPIO2_A4 - Header 1 Pin 7
  .p0_g2         = 69,  // GPIO2_A5 - Header 1 Pin 12
  .p0_b2         = 74,  // GPIO2_B2 - Header 1 Pin 16
};
// ==========================================================

MAPPING_END
    tail -n +$LINE_NUM "$HWMAP_C"
} > "$HWMAP_C.tmp"
mv "$HWMAP_C.tmp" "$HWMAP_C"

# Register in array
if ! grep -q "&rockpis_mapping," "$HWMAP_C"; then
    sed -i "/static HardwareMapping \*hardware_mappings/,/NULL/s/  NULL/  \&rockpis_mapping,\n  NULL/" "$HWMAP_C"
    echo "  ✓ Mapping registered in array"
else
    echo "  ℹ  Mapping already in array"
fi
echo "  ✓ Hardware mapping applied"
echo ""

# ── 4. Compile library ──────────────────────────────────────────────────────
echo "[4/8] Compiling library (this takes 2-3 minutes)..."
cd ~/rpi-rgb-led-matrix
make clean
make -j$(nproc)
echo "  ✓ Library compiled"
echo ""

# ── 5. Install Python bindings ──────────────────────────────────────────────
echo "[5/8] Installing Python bindings..."
cd ~/rpi-rgb-led-matrix/bindings/python
sudo make install
echo "  ✓ Python bindings installed"
echo ""

# ── 6. Copy app files ───────────────────────────────────────────────────────
echo "[6/8] Copying application files to /opt/led-matrix..."
sudo mkdir -p /opt/led-matrix
sudo cp "$SCRIPT_DIR"/*.py /opt/led-matrix/
echo "  ✓ Files copied"
echo ""

# ── 7. Systemd service ──────────────────────────────────────────────────────
echo "[7/8] Installing systemd service..."
sudo mkdir -p /var/lib/led-matrix
sudo cp "$SCRIPT_DIR/led-matrix.service" /etc/systemd/system/led-matrix.service
sudo systemctl daemon-reload
sudo systemctl enable led-matrix.service

# Remove NO_DISPLAY override if present
if [ -f /etc/systemd/system/led-matrix.service.d/override.conf ]; then
    echo "  Removing NO_DISPLAY override..."
    sudo rm /etc/systemd/system/led-matrix.service.d/override.conf
    sudo rmdir /etc/systemd/system/led-matrix.service.d 2>/dev/null || true
    sudo systemctl daemon-reload
fi

echo "  ✓ Service installed and enabled"
echo ""

# ── 8. Disable UART0 serial console ────────────────────────────────────────
# GPIO2_A1 (Linux 65) and GPIO2_A0 (Linux 64) are used as HUB75 C and D
# address lines respectively. These same pins are UART0 TX/RX (pins 8/10)
# which Armbian uses as the default serial console. We must disable it before
# the GPIO pins can be used freely.
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
