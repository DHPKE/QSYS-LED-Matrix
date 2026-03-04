#!/bin/bash
# Direct test to verify segment mapping

PI_IP="10.1.1.15"
PORT="21324"

echo "=== Testing Segment Mapping for Layout 15 (VO-left) ==="
echo ""

# Enable curtain
echo "1. Enabling yellow curtain..."
echo '{"cmd":"curtain","group":1,"enabled":true,"color":"FFFF00"}' | nc -u -w1 $PI_IP $PORT
sleep 1

echo '{"cmd":"group","value":1}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Apply Layout 15
echo "2. Applying Layout 15 (preset 15)..."
echo '{"cmd":"layout","preset":15}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Test each segment individually
echo "3. Testing SEGMENT 0 (should be HIDDEN)..."
echo '{"cmd":"text","seg":0,"text":"SEG 0","color":"FFFFFF","bgcolor":"FF0000"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo "4. Testing SEGMENT 1 (should show in LARGE LEFT AREA)..."
echo '{"cmd":"text","seg":1,"text":"SEG 1","color":"000000","bgcolor":"FFFF00"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo "5. Testing SEGMENT 2 (should be HIDDEN)..."
echo '{"cmd":"text","seg":2,"text":"SEG 2","color":"FFFFFF","bgcolor":"00FF00"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo "6. Testing SEGMENT 3 (should show in SMALL BOTTOM-RIGHT)..."
echo '{"cmd":"text","seg":3,"text":"SEG 3","color":"FFFFFF","bgcolor":"0000FF"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo ""
echo "=== Results ==="
echo "Expected behavior:"
echo "- Segment 0: HIDDEN (1×1 size)"
echo "- Segment 1: Yellow 'SEG 1' in large left area"
echo "- Segment 2: HIDDEN (1×1 size)"
echo "- Segment 3: Blue 'SEG 3' in small bottom-right corner"
echo ""
echo "If you see different segments showing, that indicates the mapping is wrong."
