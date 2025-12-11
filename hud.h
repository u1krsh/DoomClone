#ifndef HUD_H
#define HUD_H

// HUD Module
// Provides heads-up display with health, armor, ammo, etc.

// HUD settings
#define HUD_BAR_WIDTH 100
#define HUD_BAR_HEIGHT 8
#define HUD_MARGIN 10
#define HUD_SPACING 5

// HUD visibility toggle
extern int hudEnabled;

// Initialize HUD system
void initHUD(void);

// Draw HUD elements
void drawHUD(void (*pixelFunc)(int, int, int, int, int), 
             int screenWidth, int screenHeight);

// Draw player status bar (health/armor)
void drawStatusBar(void (*pixelFunc)(int, int, int, int, int),
                   int x, int y, int width, int height,
                   int current, int max,
                   int r, int g, int b,
                   int bgR, int bgG, int bgB);

// Draw damage overlay (red flash when hit)
void drawDamageOverlay(void (*pixelFunc)(int, int, int, int, int),
                       int screenWidth, int screenHeight,
                       int currentTime);

// Draw death screen
void drawDeathScreen(void (*pixelFunc)(int, int, int, int, int),
                     int screenWidth, int screenHeight);

// Toggle HUD visibility
void toggleHUD(void);

// Check if HUD is enabled
int isHUDEnabled(void);

#endif // HUD_H
