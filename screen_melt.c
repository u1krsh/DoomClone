#include <stdlib.h>
#define _CRT_SECURE_NO_WARNINGS
#include "screen_melt.h"
#include "textures/main_screen.h"  // Include main screen texture

#define MAX_SCREEN_WIDTH 1920  // Support up to 1920 width
#define MELT_SPEED 8           // Pixels per frame

// Screen melt effect
typedef struct {
	int active;                          // is melt currently playing
	int columnY[MAX_SCREEN_WIDTH];       // current Y position of each column
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
	melt.started = 0;
	melt.screenWidth = 0;
	melt.screenHeight = 0;
	
	for (x = 0; x < MAX_SCREEN_WIDTH; x++) {
		melt.columnY[x] = 0;
	}
}

// Start the screen melt effect with random peaks
void startScreenMelt(void) {
	int x;
	melt.active = 1;
	melt.complete = 0;
	melt.started = 1;
	
	// Each column gets a completely random starting offset
	// Range: -100 to 0 (negative = delay before falling)
	// This creates big jagged random peaks
	for (x = 0; x < MAX_SCREEN_WIDTH; x++) {
		melt.columnY[x] = -(rand() % 100);
	}
}

// Update the screen melt effect
void updateScreenMelt(void) {
	if (!melt.active || melt.screenWidth == 0) return;
	
	int x;
	int allComplete = 1;
	
	// All columns advance at the same speed
	for (x = 0; x < melt.screenWidth; x++) {
		melt.columnY[x] += MELT_SPEED;
		
		if (melt.columnY[x] < melt.screenHeight) {
			allComplete = 0;
		}
	}
	
	if (allComplete) {
		melt.active = 0;
		melt.complete = 1;
	}
}

// Draw the main screen (before melt starts)
void drawMainScreen(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight) {
	int x, y;
	
	for (y = 0; y < screenHeight; y++) {
		for (x = 0; x < screenWidth; x++) {
			float u = (float)x / (float)screenWidth;
			float v = (float)(screenHeight - 1 - y) / (float)screenHeight;
			
			int tx = (int)(u * PFUB2_WIDTH);
			int ty = (int)(v * PFUB2_HEIGHT);
			
			if (tx < 0) tx = 0;
			if (tx >= PFUB2_WIDTH) tx = PFUB2_WIDTH - 1;
			if (ty < 0) ty = 0;
			if (ty >= PFUB2_HEIGHT) ty = PFUB2_HEIGHT - 1;
			
			int pixelIndex = (ty * PFUB2_WIDTH + tx) * 3;
			
			int r = PFUB2[pixelIndex + 0];
			int g = PFUB2[pixelIndex + 1];
			int b = PFUB2[pixelIndex + 2];
			
			pixelFunc(x, y, r, g, b);
		}
	}
}

// Draw the melt effect
void drawScreenMelt(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight) {
	if (!melt.active) return;
	
	melt.screenWidth = screenWidth;
	melt.screenHeight = screenHeight;
	
	int x, y;
	
	for (x = 0; x < screenWidth; x++) {
		int offset = melt.columnY[x];
		
		// Column hasn't started falling yet
		if (offset < 0) {
			offset = 0;
		}
		
		// Column has completely fallen off
		if (offset >= screenHeight) {
			continue;
		}
		
		// Draw the visible portion of this column
		for (y = 0; y < screenHeight - offset; y++) {
			int titleY = y + offset;
			
			float u = (float)x / (float)screenWidth;
			float v = (float)(screenHeight - 1 - titleY) / (float)screenHeight;
			
			int tx = (int)(u * PFUB2_WIDTH);
			int ty = (int)(v * PFUB2_HEIGHT);
			
			if (tx < 0) tx = 0;
			if (tx >= PFUB2_WIDTH) tx = PFUB2_WIDTH - 1;
			if (ty < 0) ty = 0;
			if (ty >= PFUB2_HEIGHT) ty = PFUB2_HEIGHT - 1;
			
			int pixelIndex = (ty * PFUB2_WIDTH + tx) * 3;
			
			int r = PFUB2[pixelIndex + 0];
			int g = PFUB2[pixelIndex + 1];
			int b = PFUB2[pixelIndex + 2];
			
			pixelFunc(x, y, r, g, b);
		}
	}
}

int shouldShowMainScreen(void) {
	return !melt.started;
}

int isMeltActive(void) {
	return melt.active;
}

int isMeltComplete(void) {
	return melt.complete;
}
