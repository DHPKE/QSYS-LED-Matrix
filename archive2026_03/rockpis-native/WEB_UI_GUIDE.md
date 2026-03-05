# LED Matrix Web UI - Quick Guide

## How to Use the Web Interface

### Step 1: Choose a Layout
Click one of the 4 layout buttons:
- **Split Vertical** - Activates segments 1 & 2 (left/right)
- **Split Horizontal** - Activates segments 1 & 2 (top/bottom)  
- **Quad** - Activates all 4 segments (quarters)
- **Full** - Activates only segment 1 (full screen)

⚠️ **Important**: When you change layouts:
- Only the active segments will display text
- Inactive segments are hidden (grayed out in UI)
- Text in inactive segments is preserved but not displayed

### Step 2: Enter Text
1. Find the segment card that is **not grayed out**
2. Type your text in the "Text" field
3. Choose text color and background color
4. Select alignment (Left/Center/Right)

### Step 3: Send to Display
Click the **"Send"** button below the text field
- Text appears immediately on the physical LED matrix
- Text persists until you clear it or change it

### Common Issues & Solutions

**Problem**: Display is flashing
- **Solution**: Make sure you've entered text and clicked "Send" in an active segment
- The display will flash if no content is present

**Problem**: Can't see my text after changing layout
- **Solution**: Text only shows in **active** segments (white card, not grayed)
- For "Full" layout, you must use Segment 1
- Re-enter text and click "Send" after changing layouts

**Problem**: Colors don't match what I selected
- **Solution**: Make sure you clicked "Send" after choosing colors
- The preview on screen may not match the physical display exactly

### Tips
- **Fullscreen**: Perfect for large text or single messages
- **Quad**: Great for 4 separate data fields
- **Brightness**: Use the slider at the bottom to adjust overall brightness
- **Clear All**: Removes text from all segments at once

### Segment Numbers
- The UI shows "Segment 1, 2, 3, 4"  
- Internally, the system uses 0-based indexing (0, 1, 2, 3)
- This is normal and doesn't affect operation

### Network Info
- **UDP Port**: 21324 (for Q-SYS plugin control)
- **Web Port**: 80 (HTTP)
- **Protocol**: JSON commands over UDP

## Example Workflow

1. Click **"Full"** layout
2. In **Segment 1** card:
   - Text: "Hello World"
   - Color: White
   - Background: Black
   - Alignment: Center
3. Click **"Send"**
4. Your text appears on the LED matrix!

To clear: Click **"Clear"** button in the segment, or **"Clear All Segments"** at the bottom.
