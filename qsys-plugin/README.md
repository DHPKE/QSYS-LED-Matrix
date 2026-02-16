# Q-SYS Plugin Files

## ✅ WHICH FILE TO USE

**Use this file:** `OlimexLEDMatrix_v2.qplug`

This is the **complete, production-ready plugin** (v2.0.0) ready to import into Q-SYS Designer.

**Features:**
- Effect selector (none, scroll, blink)
- Alignment selector (L, C, R)  
- Font size selector (auto, 8-32)
- Segment active indicators
- Error handling & validation
- Bright UI matching v2.x standards

---

## 📁 Files in this Directory

### ✅ OlimexLEDMatrix_v2.qplug
**Version:** 2.0.0  
**Status:** Production-ready  
**Format:** Q-SYS Designer plugin file (.qplug)  
**👉 Use this:** Import directly into Q-SYS Designer

### 📋 led_matrix_controller_v2.lua
**Version:** 2.0.0  
**Status:** Production-ready  
**Format:** Lua source code  
**Note:** Same as .qplug (for viewing/editing)

### 📋 led_matrix_controller.lua  
**Version:** 1.0.0  
**Status:** Legacy (kept for reference)  
**Note:** Missing controls and validation

### 📄 Documentation

- **PLUGIN_CHANGELOG.md** - Complete v1.0 → v2.0 changes
- **PLUGIN_REVIEW.md** - Issues found and fixed

---

## 🚀 Quick Start

1. **Download** `OlimexLEDMatrix_v2.qplug`
2. **Open Q-SYS Designer**
3. **Drag** the .qplug file into your design
4. **Configure:**
   - Set IP address (e.g., 192.168.1.100)
   - Set UDP port (default: 21324)
   - Set number of segments (1-4)
5. **Use:**
   - Enter text, color, font, effect, alignment
   - Click "Send" to display on LED matrix
   - Watch active LED indicators
   - Monitor "Last Command" for debugging

---

## 🎯 Installation

### Method 1: Drag & Drop (Recommended)
1. Download `OlimexLEDMatrix_v2.qplug`
2. Drag into Q-SYS Designer workspace
3. Done!

### Method 2: Manual Import
1. Q-SYS Designer → File → Open
2. Navigate to `OlimexLEDMatrix_v2.qplug`
3. Import plugin

---

**Always use v2.0.0 for new projects!**
