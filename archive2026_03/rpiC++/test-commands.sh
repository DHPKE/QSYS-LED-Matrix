#!/bin/bash
# test-commands.sh - Test UDP commands

IP="${1:-10.1.1.22}"
PORT=21324

echo "=================================================="
echo "Testing LED Matrix Controller at $IP:$PORT"
echo "=================================================="
echo ""

send_cmd() {
    local cmd="$1"
    local desc="$2"
    echo "➤ $desc"
    echo "  Command: $cmd"
    echo "$cmd" | nc -u -w1 "$IP" "$PORT"
    sleep 0.5
    echo ""
}

# Test 1: Fullscreen text
send_cmd '{"cmd":"text","seg":0,"text":"C++ PORT","color":"FFFFFF","bgcolor":"000000","align":"C"}' \
         "Test 1: Fullscreen white text"

sleep 2

# Test 2: Red text
send_cmd '{"cmd":"text","seg":0,"text":"HELLO","color":"FF0000","bgcolor":"000000","align":"C"}' \
         "Test 2: Red text"

sleep 2

# Test 3: Layout split
send_cmd '{"cmd":"layout","preset":3}' \
         "Test 3: Split layout (left/right)"

sleep 1

send_cmd '{"cmd":"text","seg":0,"text":"LEFT","color":"00FF00","bgcolor":"000000","align":"C"}' \
         "Test 3a: Left panel (green)"

send_cmd '{"cmd":"text","seg":1,"text":"RIGHT","color":"0000FF","bgcolor":"000000","align":"C"}' \
         "Test 3b: Right panel (blue)"

sleep 3

# Test 4: Quad layout
send_cmd '{"cmd":"layout","preset":7}' \
         "Test 4: Quad layout"

sleep 1

send_cmd '{"cmd":"text","seg":0,"text":"1","color":"FFFFFF","bgcolor":"FF0000","align":"C"}' \
         "Test 4a: Top-left (white on red)"

send_cmd '{"cmd":"text","seg":1,"text":"2","color":"FFFFFF","bgcolor":"00FF00","align":"C"}' \
         "Test 4b: Top-right (white on green)"

send_cmd '{"cmd":"text","seg":2,"text":"3","color":"FFFFFF","bgcolor":"0000FF","align":"C"}' \
         "Test 4c: Bottom-left (white on blue)"

send_cmd '{"cmd":"text","seg":3,"text":"4","color":"000000","bgcolor":"FFFF00","align":"C"}' \
         "Test 4d: Bottom-right (black on yellow)"

sleep 3

# Test 5: Scroll effect
send_cmd '{"cmd":"layout","preset":1}' \
         "Test 5: Back to fullscreen"

send_cmd '{"cmd":"text","seg":0,"text":"SCROLLING TEXT DEMO","color":"00FFFF","bgcolor":"000000","align":"L","effect":"scroll"}' \
         "Test 5a: Scrolling cyan text"

sleep 5

# Test 6: Blink effect
send_cmd '{"cmd":"text","seg":0,"text":"BLINKING","color":"FF00FF","bgcolor":"000000","align":"C","effect":"blink"}' \
         "Test 6: Blinking magenta text"

sleep 5

# Test 7: Brightness
send_cmd '{"cmd":"brightness","value":50}' \
         "Test 7a: Dim (50%)"

sleep 2

send_cmd '{"cmd":"brightness","value":200}' \
         "Test 7b: Bright (78%)"

sleep 2

send_cmd '{"cmd":"brightness","value":128}' \
         "Test 7c: Normal (50%)"

sleep 1

# Test 8: Frame
send_cmd '{"cmd":"text","seg":0,"text":"FRAMED","color":"FFFFFF","bgcolor":"000000","align":"C","effect":"none"}' \
         "Test 8a: Static text"

send_cmd '{"cmd":"frame","seg":0,"enabled":true,"color":"FF0000","width":2}' \
         "Test 8b: Add red frame"

sleep 3

# Test 9: Portrait mode
send_cmd '{"cmd":"orientation","value":"portrait"}' \
         "Test 9a: Switch to portrait"

sleep 2

send_cmd '{"cmd":"text","seg":0,"text":"TALL","color":"FFFF00","bgcolor":"000000","align":"C"}' \
         "Test 9b: Text in portrait mode"

sleep 3

send_cmd '{"cmd":"orientation","value":"landscape"}' \
         "Test 9c: Back to landscape"

sleep 2

# Test 10: Group routing
send_cmd '{"cmd":"group","value":1}' \
         "Test 10a: Assign to Group 1 (white indicator)"

sleep 2

send_cmd '{"cmd":"text","seg":0,"text":"GROUP 1","color":"FFFFFF","bgcolor":"000000","align":"C","group":1}' \
         "Test 10b: Send to Group 1 (should show)"

sleep 2

send_cmd '{"cmd":"text","seg":0,"text":"GROUP 2","color":"FF0000","bgcolor":"000000","align":"C","group":2}' \
         "Test 10c: Send to Group 2 (should ignore)"

sleep 2

send_cmd '{"cmd":"text","seg":0,"text":"BROADCAST","color":"00FF00","bgcolor":"000000","align":"C","group":0}' \
         "Test 10d: Broadcast (should show)"

sleep 2

send_cmd '{"cmd":"group","value":0}' \
         "Test 10e: Reset to no group"

sleep 1

# Test 11: Clear
send_cmd '{"cmd":"clear_all"}' \
         "Test 11: Clear all segments"

sleep 2

# Final message
send_cmd '{"cmd":"text","seg":0,"text":"TESTS DONE","color":"00FF00","bgcolor":"000000","align":"C"}' \
         "Final: All tests complete"

echo "=================================================="
echo "All tests completed!"
echo "Check the display for visual verification"
echo "=================================================="
