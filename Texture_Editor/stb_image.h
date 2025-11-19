// stb_image.h - v2.28 - public domain image loader
// 
// To use, place this in ONE C file:
// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
//
// Download the full library from:
// https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
//
// For now, this is a placeholder. You need to download the actual file.
// 
// Quick setup:
// 1. Go to: https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
// 2. Save as: Texture_Editor/stb_image.h
// 3. Replace this file with the downloaded content

#ifndef STB_IMAGE_H
#define STB_IMAGE_H

// Minimal declaration for compilation
unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
void stbi_image_free(void *retval_from_stbi_load);
const char *stbi_failure_reason(void);

#endif // STB_IMAGE_H
