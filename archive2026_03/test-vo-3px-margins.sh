#!/bin/bash
# Test VO Layouts with 3px margin fix

PI_IP="10.1.1.15"
PORT="21324"

echo "=== Testing Layout 15 (VO-left) - 3px margins ==="
echo ""

# Enable curtain first
echo "1. Enabling white curtain for group 1..."
echo '{"cmd":"curtain","group":1,"enabled":true,"color":"FFFFFF"}' | nc -u -w1 $PI_IP $PORT
sleep 1

echo "2. Assigning panel to group 1..."
echo '{"cmd":"group","value":1}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Apply Layout 15
echo "3. Applying Layout 15 (VO-left)..."
echo '{"cmd":"layout","preset":15}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Test Segment 1 (should show at position 3,3 with size 48×26)
echo "4. Sending text to Segment 1 (5/6 width left, should see BLUE)..."
echo '{"cmd":"text","seg":1,"text":"SEG 1","color":"FFFFFF","bgcolor":"0000FF","align":"C","font":"arial"}' | nc -u -w1 $PI_IP $PORT
sleep 3

# Test Segment 3 (should show at bottom-right with size 26×13)
echo "5. Sending text to Segment 3 (quarter BR, should see RED)..."
echo '{"cmd":"text","seg":3,"text":"LIVE","color":"FFFFFF","bgcolor":"FF0000","align":"C","font":"arial"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo ""
echo "=== Verify Layout 15: ==="
echo "- BLUE box (Seg 1) should fill ~5/6 of left side with 3px gap from curtain"
echo "- RED box (Seg 3) should be in bottom-right corner with 3px gap"
echo "- White curtain frame (2px) visible on all edges"
echo "- 1px gap between colored boxes and white frame"
echo ""
sleep 3

echo "=== Testing Layout 16 (VO-right) ==="
echo ""

# Apply Layout 16
echo "6. Applying Layout 16 (VO-right)..."
echo '{"cmd":"layout","preset":16}' | nc -u -w1 $PI_IP $PORT
sleep 1

# Test Segment 2 (should show at top-right with size 26×8)
echo "7. Sending text to Segment 2 (1/2 width, 1/3 height TR, should see GREEN)..."
echo '{"cmd":"text","seg":2,"text":"INFO","color":"FFFFFF","bgcolor":"00FF00","align":"C","font":"arial"}' | nc -u -w1 $PI_IP $PORT
sleep 3

# Test Segment 3 (should show at bottom-right, same as layout 15)
echo "8. Sending text to Segment 3 (quarter BR, should see RED)..."
echo '{"cmd":"text","seg":3,"text":"REC","color":"FFFFFF","bgcolor":"FF0000","align":"C","font":"arial"}' | nc -u -w1 $PI_IP $PORT
sleep 3

echo ""
echo "=== Verify Layout 16: ==="
echo "- GREEN box (Seg 2) should be in top-right ~1/3 height with 3px gap"
echo "- RED box (Seg 3) should be in bottom-right corner with 3px gap"
echo "- White curtain frame (2px) visible on all edges"
echo "- 1px gap between colored boxes and white frame"
echo ""
sleep 3

echo "=== Testing without curtain (should still work) ==="
echo ""

echo "9. Disabling curtain..."
echo '{"cmd":"curtain","group":1,"enabled":false}' | nc -u -w1 $PI_IP $PORT
sleep 2

echo ""
echo "Segments should still be at same positions (3px from edges)"
echo "but no white frame visible"
echo ""
sleep 3

echo "10. Re-enabling curtain..."
echo '{"cmd":"curtain","group":1,"enabled":true,"color":"FFFF00"}' | nc -u -w1 $PI_IP $PORT
sleep 2

echo ""
echo "Yellow curtain frame should appear around segments"
echo ""

echo "=== Test Complete ==="
echo ""
echo "Expected coordinates (64×32 display):"
echo "Layout 15:"
echo "  - Seg 1: (3, 3, 48×26) - Blue box left side"
echo "  - Seg 3: (35, 16, 26×13) - Red box bottom-right"
echo ""
echo "Layout 16:"
echo "  - Seg 2: (35, 3, 26×8) - Green box top-right"
echo "  - Seg 3: (35, 16, 26×13) - Red box bottom-right"
echo ""
echo "All segments have 3px clearance from display edges"
echo ""
echo "To reset: echo '{\"cmd\":\"group\",\"value\":0}' | nc -u $PI_IP $PORT"
