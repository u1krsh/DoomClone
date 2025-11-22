#include "fps_counter.h"
#include "console_font.h"  // Use the same font as console
#include <stdio.h>

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
	sprintf_s(fpsText, sizeof(fpsText), "FPS: %d", fpsCounter.fps);
	
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

// Draw a line for debug visualization
static void drawDebugLine(void (*pixelFunc)(int, int, int, int, int), 
                          int x1, int y1, int x2, int y2, 
                          int r, int g, int b) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	int steps = (dx > dy ? (dx > -dx ? dx : -dx) : (dy > -dy ? dy : -dy));
	if (steps == 0) steps = 1;
	
	float xInc = (float)dx / (float)steps;
	float yInc = (float)dy / (float)steps;
	float x = (float)x1;
	float y = (float)y1;
	
	for (int i = 0; i <= steps; i++) {
		int px = (int)x;
		int py = (int)y;
		pixelFunc(px, py, r, g, b);
		x += xInc;
		y += yInc;
	}
}

// Draw a circle for debug visualization
static void drawDebugCircle(void (*pixelFunc)(int, int, int, int, int), 
                            int cx, int cy, int radius, 
                            int r, int g, int b) {
	int x = radius;
	int y = 0;
	int err = 0;
	
	while (x >= y) {
		pixelFunc(cx + x, cy + y, r, g, b);
		pixelFunc(cx + y, cy + x, r, g, b);
		pixelFunc(cx - y, cy + x, r, g, b);
		pixelFunc(cx - x, cy + y, r, g, b);
		pixelFunc(cx - x, cy - y, r, g, b);
		pixelFunc(cx - y, cy - x, r, g, b);
		pixelFunc(cx + y, cy - x, r, g, b);
		pixelFunc(cx + x, cy - y, r, g, b);
		
		if (err <= 0) {
			y += 1;
			err += 2 * y + 1;
		}
		if (err > 0) {
			x -= 1;
			err -= 2 * x + 1;
		}
	}
}

// Draw comprehensive debug overlay with crosshair, coordinates, hitboxes, and enemy activation radii
void drawDebugOverlay(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight, 
                      int playerX, int playerY, int playerZ, int playerAngle, int playerLook) {
	if (!fpsCounter.showFPS) return;
	
	// Draw center crosshair (XYZ indicator)
	int centerX = screenWidth / 2;
	int centerY = screenHeight / 2;
	
	// Horizontal line (red for X axis)
	drawDebugLine(pixelFunc, centerX - 10, centerY, centerX - 2, centerY, 255, 0, 0);
	drawDebugLine(pixelFunc, centerX + 2, centerY, centerX + 10, centerY, 255, 0, 0);
	
	// Vertical line (green for Y/Z axis)
	drawDebugLine(pixelFunc, centerX, centerY - 10, centerX, centerY - 2, 0, 255, 0);
	drawDebugLine(pixelFunc, centerX, centerY + 2, centerX, centerY + 10, 0, 255, 0);
	
	// Center dot (white)
	pixelFunc(centerX, centerY, 255, 255, 255);
	pixelFunc(centerX - 1, centerY, 255, 255, 255);
	pixelFunc(centerX + 1, centerY, 255, 255, 255);
	pixelFunc(centerX, centerY - 1, 255, 255, 255);
	pixelFunc(centerX, centerY + 1, 255, 255, 255);
	
	// Draw player coordinates and info
	char debugText[128];
	int yPos = screenHeight - 10;
	
	// FPS (top left - white)
	sprintf_s(debugText, sizeof(debugText), "FPS: %d", fpsCounter.fps);
	drawString(5, yPos, debugText, 255, 255, 255, pixelFunc);
	yPos -= 12;
	
	// Player X position (red)
	sprintf_s(debugText, sizeof(debugText), "X: %d", playerX);
	drawString(5, yPos, debugText, 255, 100, 100, pixelFunc);
	yPos -= 12;
	
	// Player Y position (green)
	sprintf_s(debugText, sizeof(debugText), "Y: %d", playerY);
	drawString(5, yPos, debugText, 100, 255, 100, pixelFunc);
	yPos -= 12;
	
	// Player Z position (blue)
	sprintf_s(debugText, sizeof(debugText), "Z: %d", playerZ);
	drawString(5, yPos, debugText, 100, 100, 255, pixelFunc);
	yPos -= 12;
	
	// Player angle (yellow)
	sprintf_s(debugText, sizeof(debugText), "Angle: %d", playerAngle);
	drawString(5, yPos, debugText, 255, 255, 100, pixelFunc);
	yPos -= 12;
	
	// Player look (cyan)
	sprintf_s(debugText, sizeof(debugText), "Look: %d", playerLook);
	drawString(5, yPos, debugText, 100, 255, 255, pixelFunc);
}
