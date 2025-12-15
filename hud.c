#define _CRT_SECURE_NO_WARNINGS
#include "hud.h"
#include "console_font.h"
#include "textures/Pl_cycle1.h"
#include "textures/Pl_cycle2.h"
#include "textures/Pl_cycle3.h"
#include "textures/Pl_cycle4.h"
#include "textures/Pl_cycle5.h"
#include <stdio.h>
#include <string.h>

// External declarations from enemy.h (instead of including it)
extern int playerHealth;
extern int playerMaxHealth;
extern int playerArmor;
extern int playerMaxArmor;
extern int playerDead;
extern int lastPlayerDamageTime;
extern int enemiesKilled;
extern int totalEnemiesSpawned;
extern int enemiesEnabled;

// HUD state
int hudEnabled = 1;

// Damage flash timing
#define DAMAGE_FLASH_DURATION 300  // milliseconds

// Initialize HUD system
void initHUD(void) {
    hudEnabled = 1;
}

// Draw a filled rectangle
static void drawRect(void (*pixelFunc)(int, int, int, int, int),
                     int x, int y, int width, int height,
                     int r, int g, int b) {
    for (int py = y; py < y + height; py++) {
        for (int px = x; px < x + width; px++) {
            pixelFunc(px, py, r, g, b);
        }
    }
}

// Draw a rectangle outline
static void drawRectOutline(void (*pixelFunc)(int, int, int, int, int),
                            int x, int y, int width, int height,
                            int r, int g, int b) {
    // Top and bottom
    for (int px = x; px < x + width; px++) {
        pixelFunc(px, y, r, g, b);
        pixelFunc(px, y + height - 1, r, g, b);
    }
    // Left and right
    for (int py = y; py < y + height; py++) {
        pixelFunc(x, py, r, g, b);
        pixelFunc(x + width - 1, py, r, g, b);
    }
}

// Draw player status bar
void drawStatusBar(void (*pixelFunc)(int, int, int, int, int),
                   int x, int y, int width, int height,
                   int current, int max,
                   int r, int g, int b,
                   int bgR, int bgG, int bgB) {
    // Draw background
    drawRect(pixelFunc, x, y, width, height, bgR, bgG, bgB);
    
    // Calculate fill width
    if (max <= 0) max = 1;
    float percent = (float)current / (float)max;
    if (percent < 0) percent = 0;
    if (percent > 1) percent = 1;
    int fillWidth = (int)(width * percent);
    
    // Draw filled portion
    if (fillWidth > 0) {
        drawRect(pixelFunc, x, y, fillWidth, height, r, g, b);
    }
    
    // Draw outline
    drawRectOutline(pixelFunc, x, y, width, height, 255, 255, 255);
}

// Draw HUD elements
void drawHUD(void (*pixelFunc)(int, int, int, int, int), 
             int screenWidth, int screenHeight, int currentTime) {
    if (!hudEnabled) return;
    
    // Bottom-left corner for health/armor bars
    int barWidth = HUD_BAR_WIDTH;
    int barHeight = HUD_BAR_HEIGHT;
    int margin = HUD_MARGIN;
    
    // Scale based on screen size
    if (screenHeight >= 480) {
        barWidth = 150;
        barHeight = 12;
        margin = 15;
    }
    
    // Health bar position (bottom-left)
    int healthX = margin;
    int healthY = margin;
    
    // Armor bar position (above health)
    int armorX = margin;
    int armorY = healthY + barHeight + HUD_SPACING;
    
    // Draw health bar (red to green based on health)
    int healthR = 255;
    int healthG = 0;
    float healthPercent = (float)playerHealth / (float)playerMaxHealth;
    if (healthPercent > 0.5f) {
        // Yellow to green
        healthR = (int)(255 * (1.0f - (healthPercent - 0.5f) * 2.0f));
        healthG = 255;
    } else {
        // Red to yellow
        healthR = 255;
        healthG = (int)(255 * healthPercent * 2.0f);
    }
    
    drawStatusBar(pixelFunc, healthX, healthY, barWidth, barHeight,
                  playerHealth, playerMaxHealth,
                  healthR, healthG, 0,  // Health color
                  64, 0, 0);            // Background color (dark red)
    
    // Draw armor bar (blue)
    if (playerArmor > 0) {
        drawStatusBar(pixelFunc, armorX, armorY, barWidth, barHeight,
                      playerArmor, playerMaxArmor,
                      0, 100, 255,      // Armor color (blue)
                      0, 0, 64);        // Background color (dark blue)
    }
    
    // Draw health/armor numbers
    char healthText[32];
    char armorText[32];
    
    snprintf(healthText, sizeof(healthText), "%d", playerHealth);
    snprintf(armorText, sizeof(armorText), "%d", playerArmor);
    
    // Health number (to the right of the bar)
    int textX = healthX + barWidth + 5;
    drawString(textX, healthY + 1, healthText, 255, 255, 255, pixelFunc);
    
    // Armor number
    if (playerArmor > 0) {
        drawString(textX, armorY + 1, armorText, 100, 150, 255, pixelFunc);
    }
    
    // Draw enemy kill counter (top-right)
    if (totalEnemiesSpawned > 0) {
        char killText[64];
        snprintf(killText, sizeof(killText), "KILLS: %d/%d", enemiesKilled, totalEnemiesSpawned);
        
        int killTextWidth = strlen(killText) * 8;
        int killX = screenWidth - killTextWidth - margin;
        int killY = screenHeight - margin - 8;
        
        drawString(killX, killY, killText, 255, 200, 100, pixelFunc);
    }
    
    // Draw status icons/text
    extern int godMode;
    extern int noclip;
    
    int statusY = screenHeight - margin - 8;
    int statusX = screenWidth / 2 - 40;
    
    // Show active cheats
    if (godMode) {
        drawString(statusX, statusY, "GOD", 255, 255, 0, pixelFunc);
        statusX += 35;
    }
    if (noclip) {
        drawString(statusX, statusY, "NOCLIP", 0, 255, 255, pixelFunc);
        statusX += 55;
    }
    if (!enemiesEnabled) {
        drawString(statusX, statusY, "NOTARGET", 0, 255, 0, pixelFunc);
    }
    
    // Draw player face (Doom-style status bar face)
    // Select which cycle sprite to use based on health percentage (reuse healthPercent from above)
    
    // Determine which cycle dataset to use
    const unsigned char** cycleFrames;
    const int* frameWidths;
    const int* frameHeights;
    const int* frameDurations;
    int frameCount;
    
    if (healthPercent > 0.8f) {
        // 100-80% health: Cycle 1 (healthy)
        cycleFrames = PL_CYCLE1_frames;
        frameWidths = PL_CYCLE1_frame_widths;
        frameHeights = PL_CYCLE1_frame_heights;
        frameDurations = PL_CYCLE1_frame_durations;
        frameCount = PL_CYCLE1_FRAME_COUNT;
    } else if (healthPercent > 0.6f) {
        // 80-60% health: Cycle 2 (slightly hurt)
        cycleFrames = PL_CYCLE2_frames;
        frameWidths = PL_CYCLE2_frame_widths;
        frameHeights = PL_CYCLE2_frame_heights;
        frameDurations = PL_CYCLE2_frame_durations;
        frameCount = PL_CYCLE2_FRAME_COUNT;
    } else if (healthPercent > 0.4f) {
        // 60-40% health: Cycle 3 (hurt)
        cycleFrames = PL_CYCLE3_frames;
        frameWidths = PL_CYCLE3_frame_widths;
        frameHeights = PL_CYCLE3_frame_heights;
        frameDurations = PL_CYCLE3_frame_durations;
        frameCount = PL_CYCLE3_FRAME_COUNT;
    } else if (healthPercent > 0.2f) {
        // 40-20% health: Cycle 4 (badly hurt)
        cycleFrames = PL_CYCLE4_frames;
        frameWidths = PL_CYCLE4_frame_widths;
        frameHeights = PL_CYCLE4_frame_heights;
        frameDurations = PL_CYCLE4_frame_durations;
        frameCount = PL_CYCLE4_FRAME_COUNT;
    } else {
        // Below 20% health: Cycle 5 (critical)
        cycleFrames = PL_CYCLE5_frames;
        frameWidths = PL_CYCLE5_frame_widths;
        frameHeights = PL_CYCLE5_frame_heights;
        frameDurations = PL_CYCLE5_frame_durations;
        frameCount = PL_CYCLE5_FRAME_COUNT;
    }
    
    // Calculate which frame to show based on time
    int totalCycleTime = 0;
    for (int i = 0; i < frameCount; i++) {
        totalCycleTime += frameDurations[i];
    }
    
    int cyclePos = currentTime % totalCycleTime;
    
    // Find which frame to display
    int currentFrame = 0;
    int accumulatedTime = 0;
    for (int i = 0; i < frameCount; i++) {
        if (cyclePos < accumulatedTime + frameDurations[i]) {
            currentFrame = i;
            break;
        }
        accumulatedTime += frameDurations[i];
    }
    
    // Draw the face (bottom center of screen)
    int faceWidth = frameWidths[currentFrame];
    int faceHeight = frameHeights[currentFrame];
    const unsigned char* faceData = cycleFrames[currentFrame];
    
    // Position: bottom center, just above the health bar
    int faceX = (screenWidth - faceWidth) / 2;
    int faceY = margin + 5; // Just above health bar
    
    // Draw the face sprite
    for (int y = 0; y < faceHeight; y++) {
        for (int x = 0; x < faceWidth; x++) {
            int pixelIndex = (y * faceWidth + x) * 3;
            int r = faceData[pixelIndex + 0];
            int g = faceData[pixelIndex + 1];
            int b = faceData[pixelIndex + 2];
            
            // Skip transparent pixels (1, 0, 0)
            if (r == 1 && g == 0 && b == 0) continue;
            
            // Flip vertically for correct display
            int drawY = faceY + (faceHeight - 1 - y);
            
            pixelFunc(faceX + x, drawY, r, g, b);
        }
    }
}

// Draw damage overlay (red flash)
void drawDamageOverlay(void (*pixelFunc)(int, int, int, int, int),
                       int screenWidth, int screenHeight,
                       int currentTime) {
    // Check if we recently took damage
    int timeSinceDamage = currentTime - lastPlayerDamageTime;
    
    if (timeSinceDamage < DAMAGE_FLASH_DURATION && timeSinceDamage >= 0) {
        // Calculate flash intensity (fades out)
        float intensity = 1.0f - ((float)timeSinceDamage / (float)DAMAGE_FLASH_DURATION);
        
        // Draw red overlay with dithering for transparency effect
        int skipFactor = (int)(4 + (1.0f - intensity) * 8);
        if (skipFactor < 2) skipFactor = 2;
        
        int redIntensity = (int)(180 * intensity);
        
        for (int y = 0; y < screenHeight; y += skipFactor) {
            for (int x = (y % 2); x < screenWidth; x += skipFactor) {
                pixelFunc(x, y, redIntensity, 0, 0);
            }
        }
        
        // Draw red border
        int borderSize = (int)(10 * intensity);
        for (int i = 0; i < borderSize; i++) {
            // Top border
            for (int x = 0; x < screenWidth; x++) {
                pixelFunc(x, screenHeight - 1 - i, redIntensity, 0, 0);
            }
            // Bottom border
            for (int x = 0; x < screenWidth; x++) {
                pixelFunc(x, i, redIntensity, 0, 0);
            }
            // Left border
            for (int y = 0; y < screenHeight; y++) {
                pixelFunc(i, y, redIntensity, 0, 0);
            }
            // Right border
            for (int y = 0; y < screenHeight; y++) {
                pixelFunc(screenWidth - 1 - i, y, redIntensity, 0, 0);
            }
        }
    }
}

// Draw death screen
void drawDeathScreen(void (*pixelFunc)(int, int, int, int, int),
                     int screenWidth, int screenHeight) {
    if (!playerDead) return;
    
    // Red tint overlay
    for (int y = 0; y < screenHeight; y++) {
        for (int x = 0; x < screenWidth; x++) {
            if ((x + y) % 3 == 0) {
                pixelFunc(x, y, 128, 0, 0);
            }
        }
    }
    
    // Calculate font scale
    int fontScale = 2;
    if (screenHeight >= 480) fontScale = 3;
    if (screenHeight >= 720) fontScale = 4;
    
    // "YOU DIED" text
    const char* deathText = "YOU DIED";
    int textLength = strlen(deathText);
    int charWidth = 8 * fontScale;
    int textWidth = textLength * charWidth;
    int textX = (screenWidth - textWidth) / 2;
    int textY = screenHeight / 2;
    
    drawStringScaled(textX, textY, deathText, 255, 0, 0, fontScale, pixelFunc);
    
    // "Press Enter to respawn" text
    int helpScale = fontScale > 1 ? fontScale - 1 : 1;
    const char* helpText = "Press ENTER to restart";
    int helpLength = strlen(helpText);
    int helpWidth = helpLength * 8 * helpScale;
    int helpX = (screenWidth - helpWidth) / 2;
    int helpY = textY - (20 * fontScale);
    
    drawStringScaled(helpX, helpY, helpText, 200, 100, 100, helpScale, pixelFunc);
    
    // Show final stats
    char statsText[64];
    snprintf(statsText, sizeof(statsText), "Enemies killed: %d", enemiesKilled);
    int statsLength = strlen(statsText);
    int statsWidth = statsLength * 8 * helpScale;
    int statsX = (screenWidth - statsWidth) / 2;
    int statsY = helpY - (15 * helpScale);
    
    drawStringScaled(statsX, statsY, statsText, 200, 200, 200, helpScale, pixelFunc);
}

// Toggle HUD visibility
void toggleHUD(void) {
    hudEnabled = !hudEnabled;
}

// Check if HUD is enabled
int isHUDEnabled(void) {
    return hudEnabled;
}
