#!/bin/bash
# Test Layout 15 & 16 on LED Matrix

PI_IP="10.1.1.15"
PORT="21324"

echo "Testing Layout 15 (VO-left)..."
echo '{"cmd":"layout","preset":15}' | nc -u -w1 $PI_IP $PORT
sleep 1

echo "Adding text to segment 1 (5/6 width left)..."
echo '{"cmd":"text","seg":1,"text":"SPEAKER NAME","color":"FFFFFF","bgcolor":"0000FF","align":"C"}' | nc -u -w1 $PI_IP $PORT
sleep 1

echo "Adding text to segment 3 (quarter bottom-right)..."
echo '{"cmd":"text","seg":3,"text":"LIVE","color":"FFFFFF","bgcolor":"FF0000","align":"C"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo ""
echo "Testing Layout 16 (VO-right)..."
echo '{"cmd":"layout","preset":16}' | nc -u -w1 $PI_IP $PORT
sleep 1

echo "Adding text to segment 2 (1/3 width right)..."
echo '{"cmd":"text","seg":2,"text":"INFO","color":"FFFFFF","bgcolor":"00FF00","align":"C"}' | nc -u -w1 $PI_IP $PORT
sleep 1

echo "Adding text to segment 3 (quarter bottom-right)..."
echo '{"cmd":"text","seg":3,"text":"REC","color":"FFFFFF","bgcolor":"FF0000","align":"C"}' | nc -u -w1 $PI_IP $PORT

echo ""
echo "Done! Check the display at $PI_IP"
