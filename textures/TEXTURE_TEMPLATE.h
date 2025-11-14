// Template for creating texture header files
// Replace the values below with your actual texture data

// For a 32x32 texture, define:
// #define TEXTURE_NAME_WIDTH 32
// #define TEXTURE_NAME_HEIGHT 32
// const unsigned char TEXTURE_NAME[] = {
//     // RGB data: width * height * 3 bytes
//     // Each pixel is 3 bytes: R, G, B
//     // Example for a 2x2 red texture:
//     // 255, 0, 0,  255, 0, 0,  // Row 1: red, red
//     // 255, 0, 0,  255, 0, 0   // Row 2: red, red
// };

// To convert an image to this format:
// 1. Use an image editor to resize to desired dimensions
// 2. Save as raw RGB format (no header, just pixel data)
// 3. Use a hex editor or script to convert to C array format

// Example: 16x16 checkerboard pattern
#define EXAMPLE_TEXTURE_WIDTH 16
#define EXAMPLE_TEXTURE_HEIGHT 16

// This would be 16 * 16 * 3 = 768 bytes
