#!/bin/bash
# Test script for group functionality

PI_IP="10.1.1.25"
UDP_PORT="21324"

echo "Testing group functionality on Pi at $PI_IP:$UDP_PORT"
echo ""

# Test 1: Set group to 1 (should show white indicator)
echo "Test 1: Setting Pi to Group 1 (White indicator)..."
echo '{"cmd":"group","value":1}' | nc -u $PI_IP $UDP_PORT
sleep 1

# Test 2: Send text to group 0 (broadcast - should display)
echo "Test 2: Sending text to group 0 (broadcast)..."
echo '{"cmd":"text","seg":0,"text":"GRP1-ALL","color":"FFFFFF","bgcolor":"000000","font":"arial","size":"auto","align":"C","effect":"none","intensity":255,"group":0}' | nc -u $PI_IP $UDP_PORT
sleep 2

# Test 3: Send text to group 1 (should display)
echo "Test 3: Sending text to group 1 (should work)..."
echo '{"cmd":"text","seg":0,"text":"GRP1-OK","color":"00FF00","bgcolor":"000000","font":"arial","size":"auto","align":"C","effect":"none","intensity":255,"group":1}' | nc -u $PI_IP $UDP_PORT
sleep 2

# Test 4: Send text to group 2 (should be ignored)
echo "Test 4: Sending text to group 2 (should ignore)..."
echo '{"cmd":"text","seg":0,"text":"GRP2-NO","color":"FF0000","bgcolor":"000000","font":"arial","size":"auto","align":"C","effect":"none","intensity":255,"group":2}' | nc -u $PI_IP $UDP_PORT
sleep 2

# Test 5: Set group to 3 (should show orange indicator)
echo "Test 5: Setting Pi to Group 3 (Orange indicator)..."
echo '{"cmd":"group","value":3}' | nc -u $PI_IP $UDP_PORT
sleep 1

# Test 6: Send text to group 3 (should display)
echo "Test 6: Sending text to group 3 (should work)..."
echo '{"cmd":"text","seg":0,"text":"GRP3-OK","color":"FFA500","bgcolor":"000000","font":"arial","size":"auto","align":"C","effect":"none","intensity":255,"group":3}' | nc -u $PI_IP $UDP_PORT
sleep 2

# Test 7: Set back to group 0 (no indicator)
echo "Test 7: Setting Pi to Group 0 (no indicator, accept all)..."
echo '{"cmd":"group","value":0}' | nc -u $PI_IP $UDP_PORT
sleep 1

echo ""
echo "Tests complete. Check the LED display for:"
echo "  - White square indicator when in Group 1"
echo "  - Orange square indicator when in Group 3"
echo "  - No indicator when in Group 0"
echo "  - Group 2 message should have been ignored"
