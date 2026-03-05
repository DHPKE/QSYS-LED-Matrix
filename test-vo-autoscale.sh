#!/bin/bash
# Test VO Layout Auto-Scale with Curtain Frame

PI_IP="10.1.1.15"
PORT="21324"

echo "=== Testing Layout 8 (VO-left) with Curtain Auto-Scale ==="
echo ""

# Step 1: Apply Layout 8
echo "1. Applying Layout 8 (VO-left)..."
echo '{"cmd":"layout","preset":15}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Step 2: Enable curtain for group 2 (yellow frame)
echo "2. Enabling yellow curtain frame for group 2..."
echo '{"cmd":"curtain","group":2,"enabled":true,"color":"FFFF00"}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Step 3: Assign panel to group 2
echo "3. Assigning panel to group 2..."
echo '{"cmd":"group","value":2}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Step 4: Send text to Segment 1 (should auto-scale with 3px margin)
echo "4. Sending text to Segment 1 (should maximize with curtain gap)..."
echo '{"cmd":"text","seg":1,"text":"SPEAKER","color":"FFFFFF","bgcolor":"0000FF","align":"C","font":"arial"}' | nc -u -w1 $PI_IP $PORT
sleep 2

# Step 5: Send text to Segment 3 (quarter BR)
echo "5. Sending text to Segment 3 (quarter BR)..."
echo '{"cmd":"text","seg":3,"text":"LIVE","color":"FFFFFF","bgcolor":"FF0000","align":"C","font":"arial"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo ""
echo "=== Now testing without curtain (should use 1px margin) ==="
echo ""

# Step 6: Disable curtain
echo "6. Disabling curtain..."
echo '{"cmd":"curtain","group":2,"enabled":false}' | nc -u -w1 $PI_IP $PORT
sleep 2

echo ""
echo "Text should now be larger (1px margin vs 3px margin)"
sleep 3

# Step 7: Re-enable curtain
echo "7. Re-enabling curtain..."
echo '{"cmd":"curtain","group":2,"enabled":true,"color":"FFFF00"}' | nc -u -w1 $PI_IP $PORT
sleep 2

echo ""
echo "Text should shrink back to respect curtain gap"
sleep 3

echo ""
echo "=== Testing Layout 9 (VO-right) ==="
echo ""

# Step 8: Apply Layout 9
echo "8. Applying Layout 9 (VO-right)..."
echo '{"cmd":"layout","preset":16}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Step 9: Send text to Segment 2 (1/3 right)
echo "9. Sending text to Segment 2 (1/3 width right)..."
echo '{"cmd":"text","seg":2,"text":"INFO","color":"FFFFFF","bgcolor":"00FF00","align":"C","font":"arial"}' | nc -u -w1 $PI_IP $PORT
sleep 2

# Step 10: Send text to Segment 3 (quarter BR)
echo "10. Sending text to Segment 3 (quarter BR)..."
echo '{"cmd":"text","seg":3,"text":"REC","color":"FFFFFF","bgcolor":"FF0000","align":"C","font":"arial"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo ""
echo "=== Test Complete ==="
echo ""
echo "Expected behavior:"
echo "- Layout 8 Seg 1: Text maximizes to 47×26 area (53×32 - 6px for margins)"
echo "- Layout 8 Seg 3: Text maximizes to 26×10 area (32×16 - 6px for margins)"
echo "- Layout 9 Seg 2: Text maximizes to 15×26 area (21×32 - 6px for margins)"
echo "- Layout 9 Seg 3: Text maximizes to 26×10 area (32×16 - 6px for margins)"
echo "- 1px gap visible between text and yellow curtain frame"
echo "- Text scales larger when curtain is disabled (uses 1px margin)"
echo ""
echo "To reset: echo '{\"cmd\":\"group\",\"value\":0}' | nc -u $PI_IP $PORT"
