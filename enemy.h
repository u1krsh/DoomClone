#ifndef ENEMY_H
#define ENEMY_H

#include "textures/BOSSA1.h"

// Define BOSSA1 dimensions from the PNG structure
#define BOSSA1_WIDTH 41   // From PNG IHDR
#define BOSSA1_HEIGHT 73  // From PNG IHDR

// Enemy constants
#define MAX_ENEMIES 32
#define ENEMY_DETECTION_RADIUS 200  // Distance at which enemy starts following player
#define ENEMY_SPEED 2               // Movement speed of enemies
#define ENEMY_COLLISION_RADIUS 10   // Collision radius for enemies

// Enemy rendering toggle
int enemiesEnabled = 1;  // 1 = enemies active, 0 = enemies disabled

// Enemy structure
typedef struct {
	int x, y, z;        // Position in world
	int active;         // Is this enemy alive/active?
	int state;          // 0 = idle, 1 = chasing
	float targetAngle;  // Angle to face player
} Enemy;

// Global enemy array
Enemy enemies[MAX_ENEMIES];
int numEnemies = 0;

// Sprite data for BOSSA1 (extracted from PNG)
// Note: BOSSA1 is a PNG array, we need to extract the raw RGB data
// For now, we'll use a placeholder - you would need to decode the PNG to get raw RGB
const unsigned char* BOSSA1_sprite = BOSSA1;  // This is the PNG data
int BOSSA1_isRawRGB = 0;  // 0 = PNG format, 1 = raw RGB format

// Initialize enemy system
void initEnemies() {
	int i;
	for (i = 0; i < MAX_ENEMIES; i++) {
		enemies[i].active = 0;
		enemies[i].state = 0;
	}
	numEnemies = 0;
}

// Add an enemy at position (x, y, z)
void addEnemy(int x, int y, int z) {
	if (numEnemies >= MAX_ENEMIES) return;
	
	enemies[numEnemies].x = x;
	enemies[numEnemies].y = y;
	enemies[numEnemies].z = z;
	enemies[numEnemies].active = 1;
	enemies[numEnemies].state = 0;
	enemies[numEnemies].targetAngle = 0.0f;
	numEnemies++;
}

// Calculate distance between two points
int enemyDist(int x1, int y1, int x2, int y2) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	return (int)sqrt(dx * dx + dy * dy);
}

// Update all enemies
void updateEnemies(int playerX, int playerY, int playerZ) {
	if (!enemiesEnabled) return;  // Skip if enemies are disabled
	
	int i;
	for (i = 0; i < numEnemies; i++) {
		if (!enemies[i].active) continue;
		
		// Calculate distance to player
		int dist = enemyDist(enemies[i].x, enemies[i].y, playerX, playerY);
		
		if (dist < ENEMY_DETECTION_RADIUS) {
			// Chase player
			enemies[i].state = 1;
			
			// Calculate direction to player
			int dx = playerX - enemies[i].x;
			int dy = playerY - enemies[i].y;
			float angle = atan2(dx, dy);
			
			// Move towards player
			if (dist > ENEMY_COLLISION_RADIUS) {
				enemies[i].x += (int)(sin(angle) * ENEMY_SPEED);
				enemies[i].y += (int)(cos(angle) * ENEMY_SPEED);
			}
			
			// Keep enemy at same height as player (same plane)
			enemies[i].z = playerZ;
			
		} else {
			// Idle state
			enemies[i].state = 0;
		}
	}
}

// Debug drawing: Draw enemy hitboxes and activation radii in 3D view
void drawEnemyDebugOverlay(void (*pixelFunc)(int, int, int, int, int), 
                           int screenWidth, int screenHeight,
                           int playerX, int playerY, int playerZ, int playerAngle,
                           float* cosTable, float* sinTable) {
	int i;
	float CS = cosTable[playerAngle];
	float SN = sinTable[playerAngle];
	
	for (i = 0; i < numEnemies; i++) {
		if (!enemies[i].active) continue;
		
		// Transform enemy position to camera space
		int relX = enemies[i].x - playerX;
		int relY = enemies[i].y - playerY;
		
		float camX = relX * CS - relY * SN;
		float camY = relX * SN + relY * CS;
		
		// Skip if behind player
		if (camY < 1.0f) continue;
		
		// Project to screen
		int screenX = (int)(camX * 200.0f / camY + screenWidth / 2);
		int screenY = (int)((enemies[i].z - playerZ) * 200.0f / camY + screenHeight / 2);
		
		// Draw collision radius circle (red for active, yellow for idle)
		int radius = (int)(ENEMY_COLLISION_RADIUS * 200.0f / camY);
		if (radius > 0 && radius < 100) {
			// Draw circle outline
			for (int angle = 0; angle < 360; angle += 10) {
				int x = screenX + (int)(radius * cosTable[angle]);
				int y = screenY + (int)(radius * sinTable[angle]);
				if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight) {
					if (enemies[i].state == 1) {
						pixelFunc(x, y, 255, 0, 0);  // Red when chasing
					} else {
						pixelFunc(x, y, 255, 255, 0);  // Yellow when idle
					}
				}
			}
		}
		
		// Draw detection radius (green, semi-transparent by drawing fewer points)
		int detRadius = (int)(ENEMY_DETECTION_RADIUS * 200.0f / camY);
		if (detRadius > 0 && detRadius < 500) {
			for (int angle = 0; angle < 360; angle += 15) {
				int x = screenX + (int)(detRadius * cosTable[angle]);
				int y = screenY + (int)(detRadius * sinTable[angle]);
				if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight) {
					pixelFunc(x, y, 0, 255, 0);  // Green activation radius
				}
			}
		}
		
		// Draw center marker (white)
		if (screenX >= 0 && screenX < screenWidth && screenY >= 0 && screenY < screenHeight) {
			pixelFunc(screenX, screenY, 255, 255, 255);
			pixelFunc(screenX - 1, screenY, 255, 255, 255);
			pixelFunc(screenX + 1, screenY, 255, 255, 255);
			pixelFunc(screenX, screenY - 1, 255, 255, 255);
			pixelFunc(screenX, screenY + 1, 255, 255, 255);
		}
	}
}

// Simple PNG header check to determine if we have RGB data
// Returns 1 if PNG needs decoding, 0 if already raw RGB
int isPNG(const unsigned char* data) {
	// Check for PNG signature: 137 80 78 71 13 10 26 10
	return (data[0] == 137 && data[1] == 80 && data[2] == 78 && data[3] == 71);
}

#endif // ENEMY_H
