#ifndef ENEMY_H
#define ENEMY_H

#include "textures/BOSSA1_walk.h"

// Enemy constants
#define MAX_ENEMIES 32
#define ENEMY_DETECTION_RADIUS 200  // Distance at which enemy starts following player
#define ENEMY_SPEED 2               // Movement speed of enemies
#define ENEMY_COLLISION_RADIUS 10   // Collision radius for enemies

// Animation constants
#define ENEMY_ANIM_SPEED 150        // Milliseconds per frame

// Enemy rendering toggle
int enemiesEnabled = 1;  // 1 = enemies active, 0 = enemies disabled

// Enemy structure
typedef struct {
	int x, y, z;        // Position in world
	int active;         // Is this enemy alive/active?
	int state;          // 0 = idle, 1 = chasing
	float targetAngle;  // Angle to face player
	int animFrame;      // Current animation frame (0-3)
	int lastAnimTime;   // Last time animation frame changed
} Enemy;

// Global enemy array
Enemy enemies[MAX_ENEMIES];
int numEnemies = 0;

// Initialize enemy system
void initEnemies() {
	int i;
	for (i = 0; i < MAX_ENEMIES; i++) {
		enemies[i].active = 0;
		enemies[i].state = 0;
		enemies[i].animFrame = 0;
		enemies[i].lastAnimTime = 0;
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
	enemies[numEnemies].animFrame = 0;
	enemies[numEnemies].lastAnimTime = 0;
	numEnemies++;
}

// Calculate distance between two points
int enemyDist(int x1, int y1, int x2, int y2) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	return (int)sqrt(dx * dx + dy * dy);
}

// Update all enemies
void updateEnemies(int playerX, int playerY, int playerZ, int currentTime) {
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
			
			// Update animation frame when moving (only if we have more than 1 frame)
			#if BOSSA1_FRAME_COUNT > 1
			if (currentTime - enemies[i].lastAnimTime >= ENEMY_ANIM_SPEED) {
				enemies[i].animFrame = (enemies[i].animFrame + 1) % BOSSA1_FRAME_COUNT;
				enemies[i].lastAnimTime = currentTime;
			}
			#endif
		} else {
			// Idle state - use frame 0
			enemies[i].state = 0;
			enemies[i].animFrame = 0;
		}
	}
}

// Debug drawing: Draw enemy hitboxes and activation radii in 3D view
void drawEnemyDebugOverlay(void (*pixelFunc)(int, int, int, int, int), 
                           int screenWidth, int screenHeight,
                           int playerX, int playerY, int playerZ, int playerAngle,
                           float* cosTable, float* sinTable,
                           float* depthBuf) {
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
		
		// Calculate radii based on distance
		int collisionRadius = (int)(ENEMY_COLLISION_RADIUS * 200.0f / camY);
		int detectionRadius = (int)(ENEMY_DETECTION_RADIUS * 200.0f / camY);
		
		// Skip if too small or too large to avoid excessive drawing
		if (collisionRadius < 2 || collisionRadius > 200) continue;
		
		// FIXED: Draw collision radius circle with depth buffer awareness
		// Only draw circle points that are NOT occluded by walls/enemies
		if (collisionRadius > 0 && collisionRadius < 100) {
			for (int angle = 0; angle < 360; angle += 20) {
				int x = screenX + (int)(collisionRadius * cosTable[angle]);
				int y = screenY + (int)(collisionRadius * sinTable[angle]);
				
				// Bounds check
				if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) continue;
				
				// DEPTH TEST: Only draw if this point is in front of walls
				// Add small offset to prevent z-fighting with enemy sprite
				float wireframeDepth = camY + 0.5f;
				
				// Skip if occluded by geometry
				if (wireframeDepth > depthBuf[x]) continue;
				
				if (enemies[i].state == 1) {
					pixelFunc(x, y, 255, 0, 0);  // Red when chasing
				} else {
					pixelFunc(x, y, 255, 255, 0);  // Yellow when idle
				}
			}
		}
		
		// FIXED: Draw detection radius with depth buffer awareness
		if (detectionRadius > 0 && detectionRadius < 300) {
			for (int angle = 0; angle < 360; angle += 30) {
				int x = screenX + (int)(detectionRadius * cosTable[angle]);
				int y = screenY + (int)(detectionRadius * sinTable[angle]);
				
				// Bounds check
				if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) continue;
				
				// DEPTH TEST: Only draw if visible
				float wireframeDepth = camY + 0.5f;
				
				// Skip if occluded
				if (wireframeDepth > depthBuf[x]) continue;
				
				pixelFunc(x, y, 0, 255, 0);  // Green activation radius
			}
		}
		
		// Draw center marker (white) - always visible since it's at enemy center
		if (screenX >= 0 && screenX < screenWidth && screenY >= 0 && screenY < screenHeight) {
			// Check depth for center marker too
			if (camY <= depthBuf[screenX] + 1.0f) {
				pixelFunc(screenX, screenY, 255, 255, 255);
			}
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
