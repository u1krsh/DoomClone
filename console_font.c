#include "console_font.h"

void drawChar(int x, int y, char c, int r, int g, int b, void (*pixelFunc)(int, int, int, int, int)) {
    if (c < 32 || c > 126) return; // Only printable ASCII
    
    int index = c - 32; // Offset to font array
    
    // Draw character with corrected orientation (flip vertically)
    for (int row = 0; row < 8; row++) {
        unsigned char byte = font8x8[index][7 - row]; // Reverse row order to fix inversion
        for (int col = 0; col < 8; col++) {
            // Fixed: Read bits from left to right instead of right to left
            if (byte & (1 << col)) {
                pixelFunc(x + col, y + row, r, g, b);
            }
        }
    }
}

void drawString(int x, int y, const char* str, int r, int g, int b, void (*pixelFunc)(int, int, int, int, int)) {
    int currentX = x;
    
    while (*str) {
        drawChar(currentX, y, *str, r, g, b, pixelFunc);
        currentX += 8; // Move to next character position (8 pixels wide)
        str++;
    }
}

// Small font version - draws at half size (4x4 pixels per character)
void drawCharSmall(int x, int y, char c, int r, int g, int b, void (*pixelFunc)(int, int, int, int, int)) {
    if (c < 32 || c > 126) return; // Only printable ASCII
    
    int index = c - 32; // Offset to font array
    
    // Draw character with corrected orientation and smaller size
    // Sample every other pixel to create 4x4 version
    for (int row = 0; row < 8; row += 2) {
        unsigned char byte = font8x8[index][7 - row]; // Reverse row order to fix inversion
        for (int col = 0; col < 8; col += 2) {
            // Fixed: Read bits from left to right instead of right to left
            if (byte & (1 << col)) {
                pixelFunc(x + col/2, y + row/2, r, g, b);
            }
        }
    }
}

void drawStringSmall(int x, int y, const char* str, int r, int g, int b, void (*pixelFunc)(int, int, int, int, int)) {
    int currentX = x;
    
    while (*str) {
        drawCharSmall(currentX, y, *str, r, g, b, pixelFunc);
        currentX += 4; // Move to next character position (4 pixels wide for small font)
        str++;
    }
}

// Scaled font version - draws character at specified scale
void drawCharScaled(int x, int y, char c, int r, int g, int b, int scale, void (*pixelFunc)(int, int, int, int, int)) {
    if (c < 32 || c > 126) return; // Only printable ASCII
    if (scale < 1) scale = 1; // Minimum scale of 1
    
    int index = c - 32; // Offset to font array
    
    // Draw character with corrected orientation and scaling
    for (int row = 0; row < 8; row++) {
        unsigned char byte = font8x8[index][7 - row]; // Reverse row order to fix inversion
        for (int col = 0; col < 8; col++) {
            // Fixed: Read bits from left to right instead of right to left
            if (byte & (1 << col)) {
                // Draw scaled pixel (scale x scale block)
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        pixelFunc(x + col * scale + sx, y + row * scale + sy, r, g, b);
                    }
                }
            }
        }
    }
}

void drawStringScaled(int x, int y, const char* str, int r, int g, int b, int scale, void (*pixelFunc)(int, int, int, int, int)) {
    if (scale < 1) scale = 1;
    int currentX = x;
    
    while (*str) {
        drawCharScaled(currentX, y, *str, r, g, b, scale, pixelFunc);
        currentX += 8 * scale; // Move to next character position (8 pixels * scale)
        str++;
    }
}
