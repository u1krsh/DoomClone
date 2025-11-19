# Texture System

## Overview
All textures are now consolidated into a single header file (`all_textures.h`) that both the main game and map editor include.

## File Structure
```
textures/
??? all_textures.h       # Master texture header (include this)
??? number.h             # Number font texture
??? oracular_texture.h   # Oracular UI texture
??? T_00.h               # Texture 0 (128x128)
??? T_01.h               # Texture 1 (32x32)
??? T_02.h               # Texture 2 (56x56)
??? T_03.h               # Texture 3
??? T_04.h               # Texture 4
??? T_05.h               # Texture 5
??? T_06.h               # Texture 6
```

## Usage

### In Your Code
Simply include the master header:

```c
#include "textures/all_textures.h"  // For DoomTest.c
// OR
#include "../textures/all_textures.h"  // For OracularMap/Oracular_map_editor.c
```

### Texture Count
Use the `NUM_TEXTURES` macro for the total count:

```c
int numText = NUM_TEXTURES - 1;  // Max texture index (0-based)
```

## Adding New Textures

To add a new texture:

1. Create your texture header file (e.g., `T_07.h`) with:
   - `T_07_WIDTH` constant
   - `T_07_HEIGHT` constant
   - `T_07` array data

2. Add the include to `all_textures.h`:
   ```c
   #include "T_07.h"
   ```

3. Update `NUM_TEXTURES` in `all_textures.h`:
   ```c
   #define NUM_TEXTURES 8  // Increment this number
   ```

4. Initialize the texture in your `init()` function:
   ```c
   Textures[7].w = T_07_WIDTH;
   Textures[7].h = T_07_HEIGHT;
   Textures[7].name = T_07;
   ```

## Benefits
- **Single point of maintenance**: All texture includes in one place
- **Consistency**: Both DoomTest and Oracular map editor use the same textures
- **Easy to extend**: Just add to `all_textures.h` and update `NUM_TEXTURES`
- **Less clutter**: Main source files are cleaner
