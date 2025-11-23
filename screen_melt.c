#include <stdlib.h>
#include "screen_melt.h"
#include "textures/main_screen.h"  // Include main screen texture

#define MAX_SCREEN_WIDTH 1920  // Support up to 1920 width

// Screen melt effect (Doom-style)
typedef struct {
	int active;                          // is melt currently playing
	int columnY[MAX_SCREEN_WIDTH];       // current Y position of each column (how far melted down)
	int columnSpeed[MAX_SCREEN_WIDTH];   // fall speed for each column
	int complete;                        // is melt complete
	int screenWidth;                     // current screen width
	int screenHeight;                    // current screen height
	int started;                         // has melt been triggered at least once
} ScreenMelt;

static ScreenMelt melt;

// Initialize screen melt effect
void initScreenMelt(void) {
	int x;
	melt.active = 0;
	melt.complete = 0;
	melt.started = 0;  // Haven't started yet - show main screen
	melt.screenWidth = 0;
	melt.screenHeight = 0;
	
	// Initialize each column with random starting position and speed
	for (x = 0; x < MAX_SCREEN_WIDTH; x++) {
		melt.columnY[x] = 0;
		// Random speed between 2 and 10 pixels per frame (much faster and more random)
		melt.columnSpeed[x] = 2 + (rand() % 9);
	}
}

// Start the screen melt effect
void startScreenMelt(void) {
	int x;
	melt.active = 1;
	melt.complete = 0;
	melt.started = 1;  // Now we've started melting
	
	// Randomize column speeds and reset positions
	for (x = 0; x < MAX_SCREEN_WIDTH; x++) {
		melt.columnY[x] = 0;
		// Much more variation: speeds between 2 and 15 pixels per frame
		melt.columnSpeed[x] = 2 + (rand() % 14);
	}
}

// Update the screen melt effect
void updateScreenMelt(void) {
	if (!melt.active || melt.screenWidth == 0) return;
	
	int x;
	int allComplete = 1;
	
	// Update each column
	for (x = 0; x < melt.screenWidth; x++) {
		if (melt.columnY[x] < melt.screenHeight) {
			melt.columnY[x] += melt.columnSpeed[x];
			
			// Add random stuttering for more chaos
			if (rand() % 10 < 3) { // 30% chance to add extra speed
				melt.columnY[x] += (rand() % 5);
			}
			
			if (melt.columnY[x] < melt.screenHeight) {
				allComplete = 0;
			}
		}
	}
	
	// If all columns have finished, end the melt
	if (allComplete) {
		melt.active = 0;
		melt.complete = 1;
	}
}

// Draw the main screen (before melt starts) - FIXED: Correct vertical orientation
void drawMainScreen(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight) {
	int x, y;
	
	for (y = 0; y < screenHeight; y++) {
		for (x = 0; x < screenWidth; x++) {
			// Calculate texture coordinates (scale texture to fit screen)
			float u = (float)x / (float)screenWidth;
			float v = (float)(screenHeight - 1 - y) / (float)screenHeight;  // FIXED: Flip vertically
			
			// Map to texture space (PFUB2 is 320x200)
			int tx = (int)(u * PFUB2_WIDTH);
			int ty = (int)(v * PFUB2_HEIGHT);
			
			// Clamp to texture bounds
			if (tx < 0) tx = 0;
			if (tx >= PFUB2_WIDTH) tx = PFUB2_WIDTH - 1;
			if (ty < 0) ty = 0;
			if (ty >= PFUB2_HEIGHT) ty = PFUB2_HEIGHT - 1;
			
			// Get pixel from texture (RGB format, 3 bytes per pixel)
			int pixelIndex = (ty * PFUB2_WIDTH + tx) * 3;
			
			// Read RGB values
			int r = PFUB2[pixelIndex + 0];
			int g = PFUB2[pixelIndex + 1];
			int b = PFUB2[pixelIndex + 2];
			
			pixelFunc(x, y, r, g, b);
		}
	}
}

// Draw the melt effect (draws main_screen texture that melts down to reveal game) - FIXED: Melt from bottom to top
void drawScreenMelt(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight) {
	if (!melt.active) return;
	
	// Update screen dimensions
	melt.screenWidth = screenWidth;
	melt.screenHeight = screenHeight;
	
	int x, y;
	
	// For each column, draw texture pixels from the TOP down to the melt position
	// This way the texture "peels away" upward to reveal the game (melt goes bottom to top)
	for (x = 0; x < screenWidth; x++) {
		int meltPos = melt.columnY[x];
		
		// Draw main_screen texture from the top of screen down to the melt position
		// Everything below meltPos is melted away (revealing game)
		// Everything above meltPos still shows the texture
		for (y = 0; y < screenHeight - meltPos; y++) {
			// Calculate texture coordinates (scale texture to fit screen)
			float u = (float)x / (float)screenWidth;
			float v = (float)(screenHeight - 1 - y) / (float)screenHeight;  // Flip vertically
			
			// Map to texture space (PFUB2 is 320x200)
			int tx = (int)(u * PFUB2_WIDTH);
			int ty = (int)(v * PFUB2_HEIGHT);
			
			// Clamp to texture bounds
			if (tx < 0) tx = 0;
			if (tx >= PFUB2_WIDTH) tx = PFUB2_WIDTH - 1;
			if (ty < 0) ty = 0;
			if (ty >= PFUB2_HEIGHT) ty = PFUB2_HEIGHT - 1;
			
			// Get pixel from texture (RGB format, 3 bytes per pixel)
			int pixelIndex = (ty * PFUB2_WIDTH + tx) * 3;
			
			// Read RGB values
			int r = PFUB2[pixelIndex + 0];
			int g = PFUB2[pixelIndex + 1];
			int b = PFUB2[pixelIndex + 2];
			
			pixelFunc(x, y, r, g, b);
		}
	}
}

// Check if we should show main screen (not started melting yet)
int shouldShowMainScreen(void) {
	return !melt.started;
}

// Check if melt is currently active
int isMeltActive(void) {
	return melt.active;
}

// Check if melt is complete
int isMeltComplete(void) {
	return melt.complete;
}
