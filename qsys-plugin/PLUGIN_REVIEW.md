# Q-SYS Plugin Review - led_matrix_controller.lua

**Version:** 1.0.0  
**Date:** 2026-02-17

---

## 🔍 ISSUES FOUND

### 🔴 CRITICAL

#### 1. Font Selector Shows Text Instead of ComboBox
**Line:** 247
```lua
layout[string.format("font_%d", i-1)] = {
    PrettyName = "Font",
    Style = "ComboBox",  -- ❌ Control defined as Text, not ComboBox
    Position = {520, segmentY + 25},
    Size = {60, 20}
}
```

**Problem:** Control created as Text (line 88) but layout says ComboBox
**Fix:** Either make it a proper ComboBox control or keep as Text with default

---

#### 2. Missing Error Handling for UDP Socket
**Lines:** 294-298
```lua
socket = UdpSocket.New()
socket:Open(ip_addr, udp_port)
Controls.connection_status.Value = 0 -- OK
```

**Problem:** No error handling if socket fails to open
**Fix:** Add pcall and proper error status

---

#### 3. No Effect or Alignment Selectors
**Problem:** Firmware supports effects (scroll, blink, fade, rainbow) and alignment (L, C, R) but plugin hardcodes them
**Current:** `align|C|effect|none` (line 324)
**Fix:** Add controls for effect and alignment selection

---

#### 4. No Font Size Control
**Problem:** Plugin sends `size|auto` always, but firmware supports explicit sizes 1-32
**Fix:** Add font size control (auto / 8 / 12 / 16 / 24 / 32)

---

### 🟡 MEDIUM

#### 5. No Visual Feedback on Send
**Problem:** User doesn't know if command was sent successfully
**Fix:** Add status indicator or momentary button color change

---

#### 6. Color Input Has No Validation
**Problem:** User can type invalid hex colors
**Fix:** Add validation or color picker

---

#### 7. UDP Port Can Be Set to Invalid Values
**Problem:** Port 0 or > 65535 accepted
**Fix:** Add validation in EventHandler

---

#### 8. Socket Never Closed on Cleanup
**Problem:** No cleanup function
**Fix:** Add cleanup to close socket properly

---

### 🟢 IMPROVEMENTS

#### 9. No Recall of Last Sent Values
**Problem:** Text disappears after sending if user edits control
**Fix:** Store last sent values

---

#### 10. Brightness Sends on Every Knob Change
**Problem:** Floods UDP with messages while dragging
**Fix:** Add debounce timer (send after 500ms of no changes)

---

#### 11. Missing Segment Active Indicators
**Problem:** Can't tell which segments are currently showing text
**Fix:** Add LED indicators

---

#### 12. No Command Queue
**Problem:** Rapid commands might be lost
**Fix:** Add command queue with rate limiting

---

#### 13. UI Colors Don't Match Firmware v1.2.0 Standards
**Problem:** Inconsistent with other plugins (grey backgrounds)
**Fix:** Use brighter greys like WAGO/Group-Matrix

---

## 📊 SUMMARY

| Category | Count | Priority |
|----------|-------|----------|
| Critical | 4 | Must fix |
| Medium | 4 | Should fix |
| Improvements | 5 | Nice to have |
| **Total** | **13** | |

---

## 🎯 RECOMMENDED FIXES

### Priority 1 (Must Fix):
1. Fix font control (ComboBox with proper choices)
2. Add UDP error handling
3. Add effect selector
4. Add alignment selector
5. Add font size selector

### Priority 2 (Should Fix):
6. Add command success feedback
7. Validate color input
8. Validate port numbers
9. Add cleanup function

### Priority 3 (Nice to Have):
10. Debounce brightness control
11. Add segment active indicators
12. Add command queue
13. Update UI colors to match v2.x plugins

---

## 📋 DETAILED FIX PLAN

### Fix 1: Proper Font ComboBox
```lua
-- In GetControls:
table.insert(controls, {
    Name = string.format("font_%d", i-1),
    ControlType = "Text",
    ControlUnit = "List",
    Choices = {"roboto8", "roboto12", "roboto16", "roboto24"},
    Count = 1,
    UserPin = true,
    PinStyle = "Input"
})
```

### Fix 2: UDP Error Handling
```lua
local success, err = pcall(function()
    socket = UdpSocket.New()
    socket:Open(ip_addr, udp_port)
end)

if success then
    Controls.connection_status.Value = 0 -- OK
    print("Connected to " .. ip_addr .. ":" .. udp_port)
else
    Controls.connection_status.Value = 2 -- Fault
    print("ERROR: " .. tostring(err))
end
```

### Fix 3: Add Effect Selector
```lua
-- Add control:
table.insert(controls, {
    Name = string.format("effect_%d", i-1),
    ControlType = "Text",
    ControlUnit = "List",
    Choices = {"none", "scroll", "blink"},  -- fade/rainbow not implemented yet
    Count = 1,
    UserPin = true,
    PinStyle = "Input"
})
```

### Fix 4: Add Alignment Selector
```lua
table.insert(controls, {
    Name = string.format("align_%d", i-1),
    ControlType = "Text",
    ControlUnit = "List",
    Choices = {"L", "C", "R"},
    Count = 1,
    UserPin = true,
    PinStyle = "Input"
})
```

### Fix 5: Add Font Size Selector
```lua
table.insert(controls, {
    Name = string.format("size_%d", i-1),
    ControlType = "Text",
    ControlUnit = "List",
    Choices = {"auto", "8", "12", "16", "24", "32"},
    Count = 1,
    UserPin = true,
    PinStyle = "Input"
})
```

---

**Status:** Ready for improvements
