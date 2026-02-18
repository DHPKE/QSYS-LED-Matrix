#!/bin/bash
# check-fix.sh — Verify if the Rock Pi S bus error fix has been applied
#
# Run this FROM your dev machine to check the remote Rock Pi S:
#   bash check-fix.sh <rockpi-ip-address>
#
# Or run it ON the Rock Pi S itself:
#   sudo bash check-fix.sh

ROCKPI_IP="$1"
IS_REMOTE=false

if [ -n "$ROCKPI_IP" ]; then
  IS_REMOTE=true
  SSH_CMD="ssh root@$ROCKPI_IP"
  echo "=== Checking Rock Pi S at $ROCKPI_IP ==="
else
  SSH_CMD=""
  echo "=== Checking local Rock Pi S configuration ==="
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. Checking if fix is present in /opt/led-matrix/main.py"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if $SSH_CMD grep -q "drop_privileges = False" /opt/led-matrix/main.py 2>/dev/null; then
  echo "✓ PASS: drop_privileges fix is present"
else
  echo "✗ FAIL: drop_privileges fix NOT found"
  echo "  → The file needs to be updated!"
  exit 1
fi

if $SSH_CMD grep -q "options.gpio_r1.*GPIO_R1" /opt/led-matrix/main.py 2>/dev/null; then
  echo "✓ PASS: Explicit GPIO pin assignments present"
else
  echo "✗ FAIL: GPIO pin assignments NOT found"
  echo "  → The file needs to be updated!"
  exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2. Checking file timestamp"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

FILE_INFO=$($SSH_CMD ls -lh /opt/led-matrix/main.py 2>/dev/null)
echo "$FILE_INFO"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3. Checking service status"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

SERVICE_STATUS=$($SSH_CMD systemctl is-active led-matrix.service 2>/dev/null)
echo "Service state: $SERVICE_STATUS"

if [ "$SERVICE_STATUS" = "active" ]; then
  echo "✓ Service is running"
elif [ "$SERVICE_STATUS" = "failed" ]; then
  echo "✗ Service has FAILED (likely bus error)"
else
  echo "⚠ Service is in state: $SERVICE_STATUS"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4. Checking recent logs for bus error"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

BUS_ERROR=$($SSH_CMD journalctl -u led-matrix -n 20 --no-pager 2>/dev/null | grep "status=7/BUS" | tail -1)
if [ -n "$BUS_ERROR" ]; then
  echo "✗ FOUND BUS ERROR in recent logs:"
  echo "  $BUS_ERROR"
  echo ""
  echo "  → Service needs to be restarted with the updated code!"
else
  echo "✓ No bus errors found in recent logs"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5. Checking for successful initialization"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

SUCCESS=$($SSH_CMD journalctl -u led-matrix -n 30 --no-pager 2>/dev/null | grep "LED matrix initialised")
DROP_PRIV=$($SSH_CMD journalctl -u led-matrix -n 30 --no-pager 2>/dev/null | grep "Privilege drop disabled")

if [ -n "$SUCCESS" ]; then
  echo "✓ FOUND successful initialization:"
  echo "  $SUCCESS"
  if [ -n "$DROP_PRIV" ]; then
    echo "  $DROP_PRIV"
    echo ""
    echo "✓✓✓ FIX IS WORKING! Matrix is initialized correctly."
  fi
else
  echo "✗ No successful initialization found in recent logs"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Summary"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ -n "$SUCCESS" ] && [ -n "$DROP_PRIV" ] && [ -z "$BUS_ERROR" ]; then
  echo "✓✓✓ ALL CHECKS PASSED - Fix is working correctly!"
  echo ""
  echo "The Rock Pi S LED Matrix is functioning properly."
  echo "Web UI should be available at: http://$ROCKPI_IP/"
elif [ -n "$BUS_ERROR" ]; then
  echo "✗✗✗ FIX NOT APPLIED YET"
  echo ""
  echo "The code has been updated but the service is still running the old version."
  echo "ACTION REQUIRED: Restart the service with the new code:"
  echo ""
  if $IS_REMOTE; then
    echo "  ./push-to-rockpi.sh $ROCKPI_IP"
  else
    echo "  sudo systemctl restart led-matrix"
  fi
else
  echo "⚠ PARTIAL - Some checks passed, some failed"
  echo "Review the output above for details."
fi

echo ""
