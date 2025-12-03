#define _CRT_SECURE_NO_WARNINGS
#include "fps_counter.h"
#include "console_font.h"  // Use the same font as console
#include <stdio.h>
#include <stdlib.h>

// FPS counter state
typedef struct {
	int fps;           // Current FPS
	int frameCount;    // Frame counter
	int fpsTimer;      // Timer for FPS calculation
	int showFPS;       // Toggle FPS display (now also controls debug overlay)
} FPSCounter;

static FPSCounter fpsCounter;

// Initialize the FPS counter
void initFPSCounter(void) {
	fpsCounter.fps = 0;
	fpsCounter.frameCount = 0;
	fpsCounter.fpsTimer = 0;
	fpsCounter.showFPS = 0; // FPS counter starts hidden
}

// Update FPS counter (call once per frame)
void updateFPSCounter(int currentTime) {
	fpsCounter.frameCount++;
	
	if (currentTime - fpsCounter.fpsTimer >= 1000) { // Update FPS every second
		fpsCounter.fps = fpsCounter.frameCount;
		fpsCounter.frameCount = 0;
		fpsCounter.fpsTimer = currentTime;
	}
}

// Draw FPS display on screen using console font
void drawFPSCounter(void (*pixelFunc)(int, int, int, int, int), int screenHeight) {
	if (!fpsCounter.showFPS) return;
	
	// Format FPS as string
	char fpsText[16];
	snprintf(fpsText, sizeof(fpsText), "FPS: %d", fpsCounter.fps);
	
	// Draw using console font in top-left corner (white text)
	drawString(5, screenHeight - 10, fpsText, 255, 255, 255, pixelFunc);
}

// Toggle FPS display visibility
void toggleFPSDisplay(void) {
	fpsCounter.showFPS = !fpsCounter.showFPS;
}

// Get current FPS value
int getCurrentFPS(void) {
	return fpsCounter.fps;
}

// Check if FPS display is enabled
int isFPSDisplayEnabled(void) {
	return fpsCounter.showFPS;
}

// Optimized: Draw a line for debug visualization (bresenham algorithm - faster)
static void drawDebugLine(void (*pixelFunc)(int, int, int, int, int), 
                          int x1, int y1, int x2, int y2, 
                          int r, int g, int b) {
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int sx = x1 < x2 ? 1 : -1;
	int sy = y1 < y2 ? 1 : -1;
	int err = dx - dy;
	
	while (1) {
		pixelFunc(x1, y1, r, g, b);
		
		if (x1 == x2 && y1 == y2) break;
		
		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x1 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y1 += sy;
		}
	}
}

// Draw comprehensive debug overlay with crosshair, coordinates, hitboxes, and enemy activation radii
void drawDebugOverlay(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight, 
                      int playerX, int playerY, int playerZ, int playerAngle, int playerLook) {
	if (!fpsCounter.showFPS) return;
	
	// Draw center crosshair (XYZ indicator) - simplified for performance
	int centerX = screenWidth / 2;
	int centerY = screenHeight / 2;
	
	// Horizontal line (red for X axis) - reduced length
	drawDebugLine(pixelFunc, centerX - 8, centerY, centerX - 2, centerY, 255, 0, 0);
	drawDebugLine(pixelFunc, centerX + 2, centerY, centerX + 8, centerY, 255, 0, 0);
	
	// Vertical line (green for Y/Z axis) - reduced length
	drawDebugLine(pixelFunc, centerX, centerY - 8, centerX, centerY - 2, 0, 255, 0);
	drawDebugLine(pixelFunc, centerX, centerY + 2, centerX, centerY + 8, 0, 255, 0);
	
	// Center dot (white) - single pixel only
	pixelFunc(centerX, centerY, 255, 255, 255);
	
	// Draw player coordinates and info
	char debugText[128];
	int yPos = screenHeight - 10;
	
	// FPS (top left - white)
	snprintf(debugText, sizeof(debugText), "FPS: %d", fpsCounter.fps);
	drawString(5, yPos, debugText, 255, 255, 255, pixelFunc);
	yPos -= 12;
	
	// Player X position (red)
	snprintf(debugText, sizeof(debugText), "X: %d", playerX);
	drawString(5, yPos, debugText, 255, 100, 100, pixelFunc);
	yPos -= 12;
	
	// Player Y position (green)
	snprintf(debugText, sizeof(debugText), "Y: %d", playerY);
	drawString(5, yPos, debugText, 100, 255, 100, pixelFunc);
	yPos -= 12;
	
	// Player Z position (blue)
	snprintf(debugText, sizeof(debugText), "Z: %d", playerZ);
	drawString(5, yPos, debugText, 100, 100, 255, pixelFunc);
	yPos -= 12;
	
	// Player angle (yellow)
	snprintf(debugText, sizeof(debugText), "Angle: %d", playerAngle);
	drawString(5, yPos, debugText, 255, 255, 100, pixelFunc);
	yPos -= 12;
	
	// Player look (cyan)
	snprintf(debugText, sizeof(debugText), "Look: %d", playerLook);
	drawString(5, yPos, debugText, 100, 255, 255, pixelFunc);
}
