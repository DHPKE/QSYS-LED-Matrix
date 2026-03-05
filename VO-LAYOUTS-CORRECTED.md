# VO Layout Corrections - v7.0.12

## Layout 15: VO-left

### Landscape (64×32)
```
┌──────────────────────────────────────────────────┬───────────┐
│                                                  │           │
│                 Segment 1                        │           │
│              5/6 width = 53px                    │           │
│              Full height = 32px                  │           │
│              Aligned LEFT                        │           │
│                                                  │           │
│                                                  │           │
│                                                  ├───────────┤
│                                                  │  Seg 3    │
│                                                  │  Quarter  │
│                                                  │  32×16    │
│                                                  │  BR       │
└──────────────────────────────────────────────────┴───────────┘
                   53 pixels                          32×16 at (32,16)
```

**Dimensions:**
- Segment 1: x=0, y=0, w=53, h=32 (5/6 width, full height, left aligned)
- Segment 3: x=32, y=16, w=32, h=16 (quarter bottom-right)

---

## Layout 16: VO-right

### Landscape (64×32)
```
┌────────────────────────────────┬───────────────────────────┐
│                                │    Segment 2              │
│                                │    1/2 width = 32px       │
│                                │    1/3 height = 11px      │
│                                │    Aligned TOP-RIGHT      │
│                                ├───────────────────────────┤
│                                │                           │
│                                │                           │
│                                │                           │
│                                ├───────────────────────────┤
│                                │        Seg 3              │
│                                │        Quarter            │
│                                │        32×16              │
│                                │        BR                 │
└────────────────────────────────┴───────────────────────────┘
           32 pixels                   32px wide
                                       11px (seg2) + 5px gap + 16px (seg3)
```

**Dimensions:**
- Segment 2: x=32, y=0, w=32, h=11 (1/2 width, 1/3 height, top-right)
- Segment 3: x=32, y=16, w=32, h=16 (quarter bottom-right)

---

## Key Changes from Previous Version

### Layout 15 (VO-left)
✅ **CORRECT** - No changes needed
- Segment 1: 53×32 (5/6 width, full height)
- Segment 3: 32×16 (quarter BR)

### Layout 16 (VO-right)
❌ **WAS:** Segment 2 = 21×32 (1/3 width, full height)  
✅ **NOW:** Segment 2 = 32×11 (1/2 width, 1/3 height)

**Reason:** User specified "1/2 width / 1/3 height" for segment 2

---

## Test Commands

### Layout 15 (VO-left)
```bash
# Apply layout
echo '{"cmd":"layout","preset":15}' | nc -u 10.1.1.15 21324

# Segment 1 (5/6 width left, 53×32)
echo '{"cmd":"text","seg":1,"text":"SPEAKER NAME","color":"FFFFFF","bgcolor":"0000FF","align":"C"}' | nc -u 10.1.1.15 21324

# Segment 3 (quarter BR, 32×16)
echo '{"cmd":"text","seg":3,"text":"LIVE","color":"FFFFFF","bgcolor":"FF0000","align":"C"}' | nc -u 10.1.1.15 21324
```

### Layout 16 (VO-right)
```bash
# Apply layout
echo '{"cmd":"layout","preset":16}' | nc -u 10.1.1.15 21324

# Segment 2 (1/2 width, 1/3 height top-right, 32×11)
echo '{"cmd":"text","seg":2,"text":"INFO","color":"FFFFFF","bgcolor":"00FF00","align":"C"}' | nc -u 10.1.1.15 21324

# Segment 3 (quarter BR, 32×16)
echo '{"cmd":"text","seg":3,"text":"REC","color":"FFFFFF","bgcolor":"FF0000","align":"C"}' | nc -u 10.1.1.15 21324
```

---

## Calculations

### Display: 64×32

#### Layout 15
- **5/6 width:** 64 × 5/6 = 53.33 → 53 pixels ✓
- **Full height:** 32 pixels ✓
- **Quarter:** 64/2 × 32/2 = 32×16 ✓
- **Quarter position:** (64/2, 32/2) = (32, 16) ✓

#### Layout 16
- **1/2 width:** 64 / 2 = 32 pixels ✓
- **1/3 height:** 32 / 3 = 10.67 → 11 pixels ✓
- **Top-right position:** (64/2, 0) = (32, 0) ✓
- **Quarter:** 32×16 at (32, 16) ✓

---

## Portrait Mode (32×64)

### Layout 15
- Segment 1: 32×53 (full width, 5/6 height)
- Segment 3: 16×32 (quarter BR)

### Layout 16
- Segment 2: 16×21 (1/2 width, 1/3 height)
- Segment 3: 16×32 (quarter BR)

---

## Auto-Scale with Curtain

When curtain is active (2px frame):
- **Margin:** 3px (2px curtain + 1px gap)
- **Layout 15 Seg 1:** 53×32 → text area 47×26
- **Layout 15 Seg 3:** 32×16 → text area 26×10
- **Layout 16 Seg 2:** 32×11 → text area 26×5
- **Layout 16 Seg 3:** 32×16 → text area 26×10

---

## Version

- **Firmware:** v7.0.12 (VO Layout Fix)
- **Q-SYS Plugin:** v7.0.13
- **Deployed:** 10.1.1.15 ✅
