#include <stdlib.h>
#include "screen_melt.h"

#define MAX_SCREEN_WIDTH 1920  // Support up to 1920 width

// Screen melt effect (Doom-style)
typedef struct {
	int active;                          // is melt currently playing
	int columnY[MAX_SCREEN_WIDTH];       // current Y position of each column (how far melted down)
	int columnSpeed[MAX_SCREEN_WIDTH];   // fall speed for each column
	int complete;                        // is melt complete
	int screenWidth;                     // current screen width
	int screenHeight;                    // current screen height
} ScreenMelt;

static ScreenMelt melt;

// Initialize screen melt effect
void initScreenMelt(void) {
	int x;
	melt.active = 0;
	melt.complete = 0;
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

// Draw the melt effect (draws black screen that melts down to reveal game)
void drawScreenMelt(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight) {
	if (!melt.active) return;
	
	// Update screen dimensions
	melt.screenWidth = screenWidth;
	melt.screenHeight = screenHeight;
	
	int x, y;
	
	// For each column, draw black pixels from the melt position UP to the top
	// This way the black screen "peels away" downward to reveal the game
	for (x = 0; x < screenWidth; x++) {
		int meltPos = melt.columnY[x];
		
		// Draw black from the melt position to the top of screen
		// Everything above meltPos is still black (not melted yet)
		for (y = meltPos; y < screenHeight; y++) {
			pixelFunc(x, screenHeight - 1 - y, 0, 0, 0); // Black pixels covering the game
		}
	}
}

// Check if melt is currently active
int isMeltActive(void) {
	return melt.active;
}

// Check if melt is complete
int isMeltComplete(void) {
	return melt.complete;
}
