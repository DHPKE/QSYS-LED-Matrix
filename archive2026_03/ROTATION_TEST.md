# Rotation Segment Mapping Test

Testing to verify segments are properly configured for 90° rotation.

Expected for rotation 90° (portrait 32×64):
- Layout 1 (fullscreen): seg0 = (0, 0, 32, 64)
- Layout 2 (top/bottom): seg0 = (0, 0, 32, 32), seg1 = (0, 32, 32, 32)
- Layout 3 (left/right): seg0 = (0, 0, 16, 64), seg1 = (16, 0, 16, 64)

Let's check what the Pi is actually using...
