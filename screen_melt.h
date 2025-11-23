#ifndef SCREEN_MELT_H
#define SCREEN_MELT_H

// Screen melt effect (Doom-style)
// Modular implementation for screen transitions

// Initialize the screen melt system
void initScreenMelt(void);

// Start/trigger a new screen melt effect
void startScreenMelt(void);

// Update the melt animation (call each frame)
void updateScreenMelt(void);

// Draw the melt effect (call each frame)
void drawScreenMelt(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight);

// Draw the main screen (before melt starts)
void drawMainScreen(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight);

// Check if we should show main screen (not started melting yet)
int shouldShowMainScreen(void);

// Check if melt is currently active
int isMeltActive(void);

// Check if melt is complete
int isMeltComplete(void);

#endif // SCREEN_MELT_H
