# MQTT Proxy - RPi Feature Parity Status

## Current Status: ⚠️ **PARTIAL** (60% complete)

The MQTT proxy currently supports BASIC commands but is **missing critical RPi features** needed for full compatibility.

### ✅ **IMPLEMENTED** (Working)

1. **segment** - Set text on segments (partial - missing bgcolor, font, size, intensity)
2. **layout** - Apply layout presets
3. **brightness** - Set display brightness (0-255)
4. **rotation** - Rotate display (0°, 90°, 180°, 270°)
5. **clear** - Clear specific segment
6. **test** - Test pattern

### ❌ **MISSING** (Required for RPi compatibility)

1. **curtain** - Edge bars (3px wide, per-group, colored)
2. **frame** - Segment borders (per-segment, colored, 1-3px width)
3. **group** - Assign device to group (1-8)
4. **display** - Enable/disable display (blank without disconnect)
5. **orientation** - Landscape/portrait mode
6. **config** - Manual segment positioning (x, y, width, height)
7. **Enhanced segment** - bgcolor, font, size, intensity fields

## Impact

**Without these features:**
- ❌ Cannot use curtain mode (edge indicators per group)
- ❌ Cannot draw borders around segments
- ❌ Cannot assign devices to groups dynamically
- ❌ Cannot blank display temporarily
- ❌ Cannot customize segment appearance (bg color, fonts, sizes)
- ❌ Cannot manually position segments

**This means:** The Q-SYS plugin **cannot work exactly like the RPi version** until these are added.

## Required Work

### 1. Update `render-engine.js`

Add support for:
- Background colors (bgcolor)
- Segment borders (frame)
- Curtain bars (3px left/right edges)
- Font selection (arial, digital, etc.)
- Size modes (auto, small, medium, large)
- Intensity/opacity

### 2. Update `server.js` Command Handlers

Add command handlers:
```javascript
case 'curtain':
  await this.handleCurtain(payload, devices);
  break;

case 'frame':
  await this.handleFrame(payload, devices);
  break;

case 'group':
  await this.handleGroup(payload, devices);
  break;

case 'display':
  await this.handleDisplay(payload, devices);
  break;

case 'orientation':
  await this.handleOrientation(payload, devices);
  break;

case 'config':
  await this.handleConfig(payload, devices);
  break;
```

### 3. Update Q-SYS Plugin

Add controls for:
- Curtain group, color, enable (per-group)
- Frame enable, color, width (per-segment)
- Background color (per-segment)
- Font selector (per-segment)
- Size mode (per-segment)
- Display enable toggle (global)
- Orientation selector (global)

### 4. Add State Tracking

Track curtain and frame state per device:
```javascript
this.curtainState = new Map(); // Track curtain config per group
this.frameState = new Map();   // Track frame config per segment
this.displayEnabled = new Map(); // Track display on/off per device
```

## Estimated Effort

- **render-engine.js updates**: 3-4 hours
- **server.js command handlers**: 2-3 hours
- **Q-SYS plugin updates**: 4-5 hours
- **Testing**: 2-3 hours
- **Total**: ~12-15 hours

## Priority Order

1. **HIGH**: `curtain` - Required for group indicators
2. **HIGH**: `frame` - Common UI element in Q-SYS designs
3. **HIGH**: Enhanced `segment` (bgcolor, font, size) - Visual polish
4. **MEDIUM**: `display` - Useful for "lights out" mode
5. **MEDIUM**: `group` - Dynamic group assignment
6. **LOW**: `orientation` - Less commonly used
7. **LOW**: `config` - Advanced custom positioning

## Recommendation

**Option A: Full Parity (12-15 hours)**
- Implement all 7 missing features
- Ensures 100% RPi compatibility
- Drop-in replacement for existing designs

**Option B: Essential Only (5-7 hours)**
- Implement: curtain, frame, enhanced segment
- Skip: display, group, orientation, config
- Covers 90% of common use cases

**Option C: Ship Current + Document Limitations**
- Ship what we have now (6 basic commands)
- Document: "Basic features only, full parity coming soon"
- Users understand limitations upfront

## My Recommendation: **Option B** (Essential Features)

Implement the visual features (curtain, frame, enhanced segment) that affect how displays look. Skip the operational features (display on/off, group reassignment) that are less critical.

This gets you to 85-90% parity with ~5-7 hours work, and those are the features users actually see and care about.

---

**Bottom Line:** The proxy works for basic text display, but needs additional work to match all RPi features. The Q-SYS plugin I created assumes these features exist, so it won't work fully until the proxy is updated.

Would you like me to implement Option B (essential visual features) now?
