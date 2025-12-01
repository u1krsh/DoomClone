# BOSSA1 Animation Fix - Variable Frame Size Support

## Problem
The BOSSA1_walk animation has frames with different dimensions:
- Frame 0: 41x73
- Frame 1: 49x74
- Frame 2: 41x73
- Frame 3: 49x74

The old system used fixed `BOSSA1_FRAME_WIDTH` and `BOSSA1_FRAME_HEIGHT` which caused rendering issues when frames had different sizes.

## Solution Implemented

### 1. Updated Texture Editor (`tools/texture_editor_pro.py`)
Modified the `export_single_animated_header()` function to export per-frame dimension arrays:

```python
# Now exports:
static const int BOSSA1_frame_widths[4] = { 41, 49, 41, 49 };
static const int BOSSA1_frame_heights[4] = { 73, 74, 73, 74 };
```

Instead of the old fixed:
```c
#define BOSSA1_FRAME_WIDTH 41   // Wrong - assumes all frames are same size
#define BOSSA1_FRAME_HEIGHT 73  // Wrong - assumes all frames are same size
```

### 2. Updated Enemy Rendering (`DoomTest.c`)
Modified `drawEnemies()` function to use per-frame dimensions:

```c
// Get dimensions for this specific frame
int frameWidth = BOSSA1_frame_widths[currentFrame];
int frameHeight = BOSSA1_frame_heights[currentFrame];

// Use these frame-specific dimensions for:
// - Sprite size calculation
// - UV coordinate calculation
// - Bounds checking
```

### 3. Header File Structure (`textures/BOSSA1_walk_new.h`)
New structure with per-frame support:

```c
#define BOSSA1_FRAME_COUNT 4
#define BOSSA1_ANIM_AVAILABLE 1

// Per-frame dimensions
static const int BOSSA1_frame_widths[4] = { 41, 49, 41, 49 };
static const int BOSSA1_frame_heights[4] = { 73, 74, 73, 74 };

// Individual frames
static const unsigned char BOSSA1_frame_0[] = { /* 41x73 data */ };
static const unsigned char BOSSA1_frame_1[] = { /* 49x74 data */ };
static const unsigned char BOSSA1_frame_2[] = { /* 41x73 data */ };
static const unsigned char BOSSA1_frame_3[] = { /* 49x74 data */ };

// Frame array
static const unsigned char* BOSSA1_frames[] = {
    BOSSA1_frame_0, BOSSA1_frame_1, BOSSA1_frame_2, BOSSA1_frame_3
};
```

## How to Regenerate Your Animation

### Step 1: Run Updated Texture Editor
```bash
python tools/texture_editor_pro.py
```

### Step 2: Load Your Sprite Frames
1. Click "Open" button
2. Select all 4 frames of your BOSSA1 walk animation (they can be different sizes!)
3. The editor will load them as an animation

### Step 3: Export with New Format
1. Click "Export" button
2. Choose "No" when asked "Export as separate .h files?" (we want single animated file)
3. Save as `BOSSA1_walk.h`

The exported file will now have the correct per-frame dimension arrays!

### Step 4: Replace Old Header
```bash
# Backup old file (just in case)
cp textures/BOSSA1_walk.h textures/BOSSA1_walk_old.h

# Replace with new export
mv BOSSA1_walk.h textures/BOSSA1_walk.h
```

### Step 5: Update Include (if needed)
In `DoomTest.c`, ensure you're including the updated header:
```c
#include "textures/BOSSA1_walk.h"  // Now has per-frame dimensions
```

### Step 6: Rebuild and Test
```bash
# Build project
# Test in game - enemy animation should now work correctly with variable frame sizes
```

## Benefits of This Fix

1. **Flexibility**: Each frame can be any size - no need to pad/crop to uniform dimensions
2. **Memory Efficiency**: No wasted bytes from padding smaller frames
3. **Better Quality**: Preserve original sprite proportions without distortion
4. **Future-Proof**: Works for any animation with variable frame sizes

## Backward Compatibility Notes

- Old animations with uniform frame sizes still work (all arrays have same values)
- The `BOSSA1_ANIM_AVAILABLE` flag still controls animation enable/disable
- Frame count and timing work exactly as before

## Testing Checklist

- [ ] Enemy sprites render at correct sizes for each frame
- [ ] No visual "popping" or distortion during animation
- [ ] Sprites properly occlude/are occluded by walls
- [ ] Frame transitions are smooth
- [ ] Debug hitbox circles match sprite sizes
- [ ] No crashes or bounds errors

## Additional Notes

The texture editor update also works for any other animated textures (WALL57, WALL58, etc.) that might have variable frame sizes in the future.

---
Generated: 2024
Author: AI Assistant
