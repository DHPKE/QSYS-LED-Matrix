#!/bin/bash
# test-layout-isolation.sh - Verify segments only render when in active layout

IP="${1:-10.1.1.22}"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Layout Isolation Test                                     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Target: $IP:21324"
echo ""

send() {
    echo "$1" | nc -u -w1 "$IP" 21324
    sleep 1.5
}

echo "Test 1: Layout 1 (fullscreen Seg 0 only)"
echo "─────────────────────────────────────────"
send '{"cmd":"layout","preset":1}'
echo "✓ Set layout 1"

echo ""
echo "Test 2: Set Seg 0 text (should appear)"
echo "─────────────────────────────────────────"
send '{"cmd":"text","seg":0,"text":"SEG 0","color":"00FF00"}'
echo "✓ Seg 0 = 'SEG 0' (green)"
echo "   Expected: Visible fullscreen"

echo ""
echo "Test 3: Set Seg 1 text (should NOT appear)"
echo "─────────────────────────────────────────"
send '{"cmd":"text","seg":1,"text":"SEG 1","color":"FF0000"}'
echo "✓ Seg 1 = 'SEG 1' (red)"
echo "   Expected: NOT visible (not in layout)"

sleep 2

echo ""
echo "Test 4: Switch to Layout 2 (top/bottom split)"
echo "─────────────────────────────────────────"
send '{"cmd":"layout","preset":2}'
echo "✓ Set layout 2"
echo "   Expected: Both Seg 0 (top) and Seg 1 (bottom) now visible"

sleep 3

echo ""
echo "Test 5: Set Seg 2 & 3 text (should NOT appear)"
echo "─────────────────────────────────────────"
send '{"cmd":"text","seg":2,"text":"SEG 2","color":"0000FF"}'
send '{"cmd":"text","seg":3,"text":"SEG 3","color":"FFFF00"}'
echo "✓ Seg 2 = 'SEG 2' (blue)"
echo "✓ Seg 3 = 'SEG 3' (yellow)"
echo "   Expected: Still NOT visible (layout 2 only has 2 segments)"

sleep 2

echo ""
echo "Test 6: Switch to Layout 7 (quad)"
echo "─────────────────────────────────────────"
send '{"cmd":"layout","preset":7}'
echo "✓ Set layout 7"
echo "   Expected: All 4 segments now visible"

sleep 3

echo ""
echo "Test 7: Back to Layout 1 (fullscreen Seg 0)"
echo "─────────────────────────────────────────"
send '{"cmd":"layout","preset":1}'
echo "✓ Set layout 1"
echo "   Expected: Only Seg 0 visible, others hidden (but text preserved)"

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Test Complete                                             ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "✅ If the fix works correctly:"
echo "   - Segments only appear when included in current layout"
echo "   - Editing segments doesn't make them visible"
echo "   - Switching layouts shows/hides segments properly"
echo "   - Text content is preserved when segments are hidden"
echo ""
