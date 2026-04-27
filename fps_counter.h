#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

// FPS Counter Module for Performance Monitoring
// Provides frame rate tracking and display functionality

// Initialize the FPS counter system and reset state timers
void initFPSCounter(void);

// Update FPS counter calculations (call once per frame)
void updateFPSCounter(int currentTime);

// Draw FPS display on screen
void drawFPSCounter(void (*pixelFunc)(int, int, int, int, int), int screenHeight);

// Toggle FPS display visibility
void toggleFPSDisplay(void);

// Get current frame-rate value as integer
int getCurrentFPS(void);

// Check if FPS display is enabled
int isFPSDisplayEnabled(void);

// Debug overlay functions (new)
void drawDebugOverlay(void (*pixelFunc)(int, int, int, int, int), int screenWidth, int screenHeight, 
                      int playerX, int playerY, int playerZ, int playerAngle, int playerLook);

#endif // FPS_COUNTER_H
