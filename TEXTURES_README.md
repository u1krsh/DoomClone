# Texture System

## Overview

The texture rendering code in `Oracular_map_editor.c` supports textures of **any size**. The rendering dynamically scales based on the texture's width and height.

## Texture Format

Textures must be in **raw RGB format**:
- 3 bytes per pixel (Red, Green, Blue)
- No compression, no headers
- Row-major order (left-to-right, top-to-bottom)

## How Textures Work

### Rendering Code
The texture rendering in `draw2D()` already supports any size:

```c
float tx_stp = Textures[G.wt].w / 15.0;  // Dynamically calculates step size
float ty_stp = Textures[G.wt].h / 15.0;
```

This means:
- 16x16 textures work ?
- 32x32 textures work ?  
- 64x64 textures work ?
- Any size works ?

### Current Issue with T_00.h

**WARNING:** The current `T_00.h` file contains PNG image data, NOT raw RGB data. This will not render correctly.

The PNG header shows the image is actually 128x128 pixels, but it needs to be decoded first.

## Creating New Textures

### Method 1: Using the Conversion Script (Recommended)

```bash
# Install PIL/Pillow if needed
pip install pillow

# Convert an image
python tools/convert_texture.py yourimage.png T_02

# This creates textures/T_02.h
```

### Method 2: Manual Creation

1. Create or open an image in an image editor (GIMP, Photoshop, etc.)
2. Resize to desired dimensions (e.g., 32x32, 64x64)
3. Export as "Raw RGB" or "Raw image data"
4. Convert the binary data to a C array format

## Adding Textures to Your Project

1. Create the texture header file (e.g., `textures/T_02.h`)
2. Include it in `Oracular_map_editor.c`:
   ```c
   #include "textures/T_02.h"
   ```
3. Update `numText`:
   ```c
   int numText = 3;  // Increment for each texture
   ```
4. Initialize in `init()` function:
   ```c
   Textures[2].w = T_02_WIDTH;
   Textures[2].h = T_02_HEIGHT;
   Textures[2].name = T_02;
   ```

## Example Textures

### T_00 (16x16)
- **Status:** Contains PNG data (needs conversion)
- **Format:** PNG (not compatible)
- **Actual Size:** 128x128 according to PNG header

### T_01 (32x32)  
- **Status:** Working ?
- **Format:** Raw RGB
- **Pattern:** Checkerboard

## Texture Header Format

```c
#define TEXTURE_NAME_WIDTH 32
#define TEXTURE_NAME_HEIGHT 32

const unsigned char TEXTURE_NAME[] = {
    255, 0, 0,    // Pixel 0,0: Red
    0, 255, 0,    // Pixel 1,0: Green
    0, 0, 255,    // Pixel 2,0: Blue
    // ... (width * height * 3 bytes total)
};
```

## Switching Between Textures

Use the UI buttons in the editor to cycle through textures. The system automatically handles different sizes.

## Performance Notes

- Larger textures use more memory but render at the same speed
- The editor preview is always 15x15 pixels regardless of texture size
- In-game rendering scales appropriately based on texture dimensions
