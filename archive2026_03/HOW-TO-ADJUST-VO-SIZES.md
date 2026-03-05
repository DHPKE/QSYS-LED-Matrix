# How to Adjust VO Layout Segment Sizes

## File Location
`/opt/led-matrix/config.py` on the Pi (or `rpi/config.py` in the repo)

## Layout Definitions

### Layout 8 (VO-left) - Line ~276
```python
8: [(3,        3,         42,    26   ),     # segment 0: large left area
    (0,        0,         1,     1    ),     # segment 1 hidden
    (48,       19,        13,    10   ),     # segment 2: small BR corner
    (0,        0,         1,     1    )],    # segment 3 hidden
```

### Layout 9 (VO-right) - Line ~281
```python
9: [(0,        0,         1,     1    ),     # segment 0 hidden
    (32,       3,         29,    18   ),     # segment 1: large top-right ← ADJUST THIS
    (48,       24,        13,    5    ),     # segment 2: small BR corner
    (0,        0,         1,     1    )],    # segment 3 hidden
```

---

## Coordinate Format

Each line is: `(x, y, width, height)`

- **x**: Horizontal position (0 = left edge)
- **y**: Vertical position (0 = top edge)  
- **width**: How wide the segment is
- **height**: How tall the segment is

---

## Display Dimensions

- **Full display**: 64 pixels wide × 32 pixels tall
- **With curtain**: Usable area starts at (3, 3) and ends at (61, 29)
- **Usable size**: 58 × 26 pixels

---

## Layout 9 (VO-right) Segment 1 - How to Adjust

**Current values** (line ~281):
```python
(32,       3,         29,    18   ),     # segment 1: large top-right
```

### To Make It Wider:
1. **Decrease x** (move left): `32` → `30` or `28`
2. **Increase width** (same amount): `29` → `31` or `33`

Example - 4 pixels wider:
```python
(28,       3,         33,    18   ),     # segment 1: 4px wider
```

### To Make It Taller:
1. **Keep x and y** the same: `(32, 3, ...)`
2. **Increase height**: `18` → `20` or `22`
3. **Adjust segment 2 y position**: Move it down to avoid overlap

Example - 4 pixels taller:
```python
9: [(0,        0,         1,     1    ),     # segment 0 hidden
    (28,       3,         33,    22   ),     # segment 1: larger (33×22)
    (48,       27,        13,    2    ),     # segment 2: moved down (y=27)
    (0,        0,         1,     1    )],    # segment 3 hidden
```

---

## Important Rules

1. **Keep 3px margin from edges**: Minimum x=3, y=3, max x+width=61, y+height=29
2. **Segments must not overlap**: If you make seg 1 bigger, move seg 2 out of the way
3. **Hidden segments**: Use `(0, 0, 1, 1)` to hide a segment
4. **After editing**: 
   ```bash
   sudo systemctl restart led-matrix
   ```

---

## Quick Size Reference (Layout 9 Segment 1)

| Description | x | y | width | height | Area |
|------------|---|---|-------|--------|------|
| Current    | 32| 3 | 29    | 18     | 522  |
| Wider      | 28| 3 | 33    | 18     | 594  |
| Taller     | 32| 3 | 29    | 22     | 638  |
| Both       | 28| 3 | 33    | 22     | 726  |
| Maximum*   | 3 | 3 | 58    | 23     | 1334 |

*Maximum = full usable width, leaving 3px for bottom segment

---

## Current Layout 9 Values (v7.0.17)

**Segment 1 (top-right info):**
- Position: (32, 3)
- Size: 29 × 18 pixels
- Area: 522 square pixels

**Segment 2 (bottom-right indicator):**
- Position: (48, 24)
- Size: 13 × 5 pixels
- Area: 65 square pixels

---

## Example: Make Segment 1 Much Larger

```python
9: [(0,        0,         1,     1    ),     # segment 0 hidden
    (3,        3,         58,    20   ),     # segment 1: LARGE top area (58×20)
    (48,       26,        13,    3    ),     # segment 2: tiny indicator (13×3)
    (0,        0,         1,     1    )],    # segment 3 hidden
```

This gives segment 1 the full width (58 pixels) and most of the height (20 pixels).

---

## Testing Your Changes

1. Edit `/opt/led-matrix/config.py` on the Pi
2. Restart: `sudo systemctl restart led-matrix`
3. Apply Layout 9 in Q-SYS
4. Send text to Segment 2 (Q-SYS UI)
5. Check if size looks good
6. Adjust and repeat

---

## Safe Backup Command

Before editing:
```bash
sudo cp /opt/led-matrix/config.py /opt/led-matrix/config.py.backup
```

To restore:
```bash
sudo cp /opt/led-matrix/config.py.backup /opt/led-matrix/config.py
sudo systemctl restart led-matrix
```
