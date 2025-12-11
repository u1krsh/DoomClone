#ifndef ENEMY_H
#define ENEMY_H

#include "textures/BOSSA1_walk.h"
#include "textures/BOSSA2_walk.h"
#include "textures/BOSSA3_walk.h"

// Enemy constants
#define MAX_ENEMIES 32
#define ENEMY_DETECTION_RADIUS 200  // Distance at which enemy starts following player
#define ENEMY_ATTACK_RADIUS 40      // Distance at which enemy attacks
#define ENEMY_SPEED 2               // Movement speed of enemies
#define ENEMY_COLLISION_RADIUS 10   // Collision radius for enemies

// Animation constants
#define ENEMY_ANIM_SPEED 150        // Milliseconds per frame
#define ENEMY_ATTACK_COOLDOWN 1000  // Milliseconds between attacks
#define ENEMY_HURT_DURATION 200     // Milliseconds to show hurt state
#define ENEMY_DEATH_DURATION 500    // Milliseconds for death animation

// Enemy types
#define ENEMY_TYPE_BOSSA1 0
#define ENEMY_TYPE_BOSSA2 1
#define ENEMY_TYPE_BOSSA3 2
#define NUM_ENEMY_TYPES 3

// Enemy states
#define ENEMY_STATE_IDLE 0
#define ENEMY_STATE_CHASING 1
#define ENEMY_STATE_ATTACKING 2
#define ENEMY_STATE_HURT 3
#define ENEMY_STATE_DYING 4
#define ENEMY_STATE_DEAD 5

// Enemy health by type
#define BOSSA1_HEALTH 100
#define BOSSA2_HEALTH 150
#define BOSSA3_HEALTH 200

// Enemy damage by type
#define BOSSA1_DAMAGE 10
#define BOSSA2_DAMAGE 15
#define BOSSA3_DAMAGE 20

// Player health system
int playerHealth = 100;
int playerMaxHealth = 100;
int playerArmor = 0;
int playerMaxArmor = 100;
int playerDead = 0;
int lastPlayerDamageTime = 0;

// Enemy rendering toggle
int enemiesEnabled = 1;  // 1 = enemies active, 0 = enemies disabled

// Enemy structure
typedef struct {
	int x, y, z;        // Position in world
	int active;         // Is this enemy alive/active?
	int state;          // Enemy state (idle, chasing, attacking, etc.)
	float targetAngle;  // Angle to face player
	int animFrame;      // Current animation frame (0-3)
	int lastAnimTime;   // Last time animation frame changed
	int enemyType;      // Type of enemy (0=BOSSA1, 1=BOSSA2, 2=BOSSA3)
	int health;         // Current health
	int maxHealth;      // Maximum health
	int lastAttackTime; // Last time enemy attacked
	int stateStartTime; // When current state started
	int damage;         // Damage dealt by this enemy
} Enemy;

// Global enemy array
Enemy enemies[MAX_ENEMIES];
int numEnemies = 0;

// Statistics tracking
int enemiesKilled = 0;
int totalEnemiesSpawned = 0;

// Get enemy health by type
int getEnemyHealthByType(int enemyType) {
	switch (enemyType) {
		case ENEMY_TYPE_BOSSA1: return BOSSA1_HEALTH;
		case ENEMY_TYPE_BOSSA2: return BOSSA2_HEALTH;
		case ENEMY_TYPE_BOSSA3: return BOSSA3_HEALTH;
		default: return BOSSA1_HEALTH;
	}
}

// Get enemy damage by type
int getEnemyDamageByType(int enemyType) {
	switch (enemyType) {
		case ENEMY_TYPE_BOSSA1: return BOSSA1_DAMAGE;
		case ENEMY_TYPE_BOSSA2: return BOSSA2_DAMAGE;
		case ENEMY_TYPE_BOSSA3: return BOSSA3_DAMAGE;
		default: return BOSSA1_DAMAGE;
	}
}

// Initialize enemy system
void initEnemies() {
	int i;
	for (i = 0; i < MAX_ENEMIES; i++) {
		enemies[i].active = 0;
		enemies[i].state = ENEMY_STATE_IDLE;
		enemies[i].animFrame = 0;
		enemies[i].lastAnimTime = 0;
		enemies[i].enemyType = ENEMY_TYPE_BOSSA1;
		enemies[i].health = 100;
		enemies[i].maxHealth = 100;
		enemies[i].lastAttackTime = 0;
		enemies[i].stateStartTime = 0;
		enemies[i].damage = BOSSA1_DAMAGE;
	}
	numEnemies = 0;
	enemiesKilled = 0;
	totalEnemiesSpawned = 0;
	
	// Reset player health
	playerHealth = 100;
	playerMaxHealth = 100;
	playerArmor = 0;
	playerDead = 0;
}

// Add an enemy at position (x, y, z) with specified type
void addEnemyType(int x, int y, int z, int enemyType) {
	if (numEnemies >= MAX_ENEMIES) return;
	if (enemyType < 0 || enemyType >= NUM_ENEMY_TYPES) enemyType = ENEMY_TYPE_BOSSA1;
	
	enemies[numEnemies].x = x;
	enemies[numEnemies].y = y;
	enemies[numEnemies].z = z;
	enemies[numEnemies].active = 1;
	enemies[numEnemies].state = ENEMY_STATE_IDLE;
	enemies[numEnemies].targetAngle = 0.0f;
	enemies[numEnemies].animFrame = 0;
	enemies[numEnemies].lastAnimTime = 0;
	enemies[numEnemies].enemyType = enemyType;
	enemies[numEnemies].health = getEnemyHealthByType(enemyType);
	enemies[numEnemies].maxHealth = getEnemyHealthByType(enemyType);
	enemies[numEnemies].lastAttackTime = 0;
	enemies[numEnemies].stateStartTime = 0;
	enemies[numEnemies].damage = getEnemyDamageByType(enemyType);
	numEnemies++;
	totalEnemiesSpawned++;
}

// Add an enemy at position (x, y, z) - defaults to BOSSA1 for backward compatibility
void addEnemy(int x, int y, int z) {
	addEnemyType(x, y, z, ENEMY_TYPE_BOSSA1);
}

// Calculate distance between two points
int enemyDist(int x1, int y1, int x2, int y2) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	return (int)sqrt(dx * dx + dy * dy);
}

// Get frame count for enemy type
int getEnemyFrameCount(int enemyType) {
	switch (enemyType) {
		case ENEMY_TYPE_BOSSA1: return BOSSA1_FRAME_COUNT;
		case ENEMY_TYPE_BOSSA2: return BOSSA2_FRAME_COUNT;
		case ENEMY_TYPE_BOSSA3: return BOSSA3_FRAME_COUNT;
		default: return BOSSA1_FRAME_COUNT;
	}
}

// Damage an enemy
void damageEnemy(int enemyIndex, int damage, int currentTime) {
	if (enemyIndex < 0 || enemyIndex >= numEnemies) return;
	if (!enemies[enemyIndex].active) return;
	if (enemies[enemyIndex].state == ENEMY_STATE_DEAD || 
	    enemies[enemyIndex].state == ENEMY_STATE_DYING) return;
	
	enemies[enemyIndex].health -= damage;
	
	if (enemies[enemyIndex].health <= 0) {
		enemies[enemyIndex].health = 0;
		enemies[enemyIndex].state = ENEMY_STATE_DYING;
		enemies[enemyIndex].stateStartTime = currentTime;
		enemies[enemyIndex].animFrame = 0;
	} else {
		enemies[enemyIndex].state = ENEMY_STATE_HURT;
		enemies[enemyIndex].stateStartTime = currentTime;
	}
}

// Damage player
void damagePlayer(int damage, int currentTime) {
	extern int godMode;
	if (godMode) return;
	if (playerDead) return;
	
	// Apply armor first (50% damage reduction)
	int actualDamage = damage;
	if (playerArmor > 0) {
		int armorAbsorb = damage / 2;
		if (armorAbsorb > playerArmor) armorAbsorb = playerArmor;
		playerArmor -= armorAbsorb;
		actualDamage = damage - armorAbsorb;
	}
	
	playerHealth -= actualDamage;
	lastPlayerDamageTime = currentTime;
	
	if (playerHealth <= 0) {
		playerHealth = 0;
		playerDead = 1;
	}
}

// Heal player
void healPlayer(int amount) {
	playerHealth += amount;
	if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;
	if (playerDead && playerHealth > 0) playerDead = 0;
}

// Add armor
void addArmor(int amount) {
	playerArmor += amount;
	if (playerArmor > playerMaxArmor) playerArmor = playerMaxArmor;
}

// Update all enemies
void updateEnemies(int playerX, int playerY, int playerZ, int currentTime) {
	if (!enemiesEnabled) return;
	
	int i;
	for (i = 0; i < numEnemies; i++) {
		if (!enemies[i].active) continue;
		
		// Handle death state
		if (enemies[i].state == ENEMY_STATE_DYING) {
			if (currentTime - enemies[i].stateStartTime >= ENEMY_DEATH_DURATION) {
				enemies[i].state = ENEMY_STATE_DEAD;
				enemies[i].active = 0;
				enemiesKilled++;
			}
			continue;
		}
		
		if (enemies[i].state == ENEMY_STATE_DEAD) continue;
		
		// Handle hurt state recovery
		if (enemies[i].state == ENEMY_STATE_HURT) {
			if (currentTime - enemies[i].stateStartTime >= ENEMY_HURT_DURATION) {
				enemies[i].state = ENEMY_STATE_CHASING;
			}
			continue;
		}
		
		// Calculate distance to player
		int dist = enemyDist(enemies[i].x, enemies[i].y, playerX, playerY);
		
		if (dist < ENEMY_ATTACK_RADIUS) {
			// Attack player
			enemies[i].state = ENEMY_STATE_ATTACKING;
			
			// Check attack cooldown
			if (currentTime - enemies[i].lastAttackTime >= ENEMY_ATTACK_COOLDOWN) {
				damagePlayer(enemies[i].damage, currentTime);
				enemies[i].lastAttackTime = currentTime;
			}
		}
		else if (dist < ENEMY_DETECTION_RADIUS) {
			// Chase player
			enemies[i].state = ENEMY_STATE_CHASING;
			
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
			int frameCount = getEnemyFrameCount(enemies[i].enemyType);
			if (frameCount > 1) {
				if (currentTime - enemies[i].lastAnimTime >= ENEMY_ANIM_SPEED) {
					enemies[i].animFrame = (enemies[i].animFrame + 1) % frameCount;
					enemies[i].lastAnimTime = currentTime;
				}
			}
		} else {
			// Idle state - use frame 0
			enemies[i].state = ENEMY_STATE_IDLE;
			enemies[i].animFrame = 0;
		}
	}
}

// Check if player is aiming at an enemy (for shooting)
// Returns enemy index or -1 if not aiming at enemy
int getEnemyInCrosshair(int playerX, int playerY, int playerAngle, float* cosTable, float* sinTable) {
	float CS = cosTable[playerAngle];
	float SN = sinTable[playerAngle];
	
	int closestEnemy = -1;
	float closestDist = 99999.0f;
	
	for (int i = 0; i < numEnemies; i++) {
		if (!enemies[i].active) continue;
		if (enemies[i].state == ENEMY_STATE_DEAD || enemies[i].state == ENEMY_STATE_DYING) continue;
		
		// Transform to camera space
		int relX = enemies[i].x - playerX;
		int relY = enemies[i].y - playerY;
		
		float camX = relX * CS - relY * SN;
		float camY = relX * SN + relY * CS;
		
		// Skip if behind player
		if (camY < 1.0f) continue;
		
		// Check if enemy is near center of screen (within crosshair tolerance)
		float screenX = camX * 200.0f / camY;
		
		// Tolerance based on distance (closer = smaller tolerance needed)
		float tolerance = 30.0f + (camY * 0.1f);
		
		if (screenX >= -tolerance && screenX <= tolerance) {
			if (camY < closestDist) {
				closestDist = camY;
				closestEnemy = i;
			}
		}
	}
	
	return closestEnemy;
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
		int attackRadius = (int)(ENEMY_ATTACK_RADIUS * 200.0f / camY);
		
		// Skip if too small or too large to avoid excessive drawing
		if (collisionRadius < 2 || collisionRadius > 200) continue;
		
		// Draw collision radius circle with depth buffer awareness
		if (collisionRadius > 0 && collisionRadius < 100) {
			for (int angle = 0; angle < 360; angle += 20) {
				int x = screenX + (int)(collisionRadius * cosTable[angle]);
				int y = screenY + (int)(collisionRadius * sinTable[angle]);
				
				// Bounds check
				if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) continue;
				
				// DEPTH TEST: Only draw if this point is in front of walls
				float wireframeDepth = camY + 0.5f;
				
				// Skip if occluded by geometry
				if (wireframeDepth > depthBuf[x]) continue;
				
				// Color based on enemy state
				switch (enemies[i].state) {
					case ENEMY_STATE_ATTACKING:
						pixelFunc(x, y, 255, 128, 0);  // Orange when attacking
						break;
					case ENEMY_STATE_CHASING:
						pixelFunc(x, y, 255, 0, 0);    // Red when chasing
						break;
					case ENEMY_STATE_HURT:
						pixelFunc(x, y, 255, 255, 255); // White when hurt
						break;
					case ENEMY_STATE_DYING:
						pixelFunc(x, y, 128, 128, 128); // Gray when dying
						break;
					default:
						// Different colors for different enemy types
						switch (enemies[i].enemyType) {
							case ENEMY_TYPE_BOSSA1: pixelFunc(x, y, 255, 255, 0); break;  // Yellow
							case ENEMY_TYPE_BOSSA2: pixelFunc(x, y, 0, 255, 255); break;  // Cyan
							case ENEMY_TYPE_BOSSA3: pixelFunc(x, y, 255, 0, 255); break;  // Magenta
							default: pixelFunc(x, y, 255, 255, 0); break;
						}
						break;
				}
			}
		}
		
		// Draw attack radius (orange circle)
		if (attackRadius > 0 && attackRadius < 200) {
			for (int angle = 0; angle < 360; angle += 30) {
				int x = screenX + (int)(attackRadius * cosTable[angle]);
				int y = screenY + (int)(attackRadius * sinTable[angle]);
				
				// Bounds check
				if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) continue;
				
				float wireframeDepth = camY + 0.5f;
				if (wireframeDepth > depthBuf[x]) continue;
				
				pixelFunc(x, y, 255, 128, 0);  // Orange attack radius
			}
		}
		
		// Draw detection radius with depth buffer awareness
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
		
		// Draw health bar above enemy (if not at full health)
		if (enemies[i].health < enemies[i].maxHealth) {
			int barWidth = 20;
			int barHeight = 2;
			int barY = screenY - collisionRadius - 5;
			int barStartX = screenX - barWidth / 2;
			
			// Calculate health percentage
			float healthPercent = (float)enemies[i].health / (float)enemies[i].maxHealth;
			int filledWidth = (int)(barWidth * healthPercent);
			
			// Draw background (red)
			for (int bx = 0; bx < barWidth; bx++) {
				int px = barStartX + bx;
				if (px >= 0 && px < screenWidth && barY >= 0 && barY < screenHeight) {
					if (camY <= depthBuf[px] + 1.0f) {
						if (bx < filledWidth) {
							pixelFunc(px, barY, 0, 255, 0);  // Green for health
						} else {
							pixelFunc(px, barY, 255, 0, 0);  // Red for missing health
						}
					}
				}
			}
		}
	}
}

// Kill all enemies (for console command)
void killAllEnemies(int currentTime) {
	for (int i = 0; i < numEnemies; i++) {
		if (enemies[i].active && enemies[i].state != ENEMY_STATE_DEAD) {
			enemies[i].health = 0;
			enemies[i].state = ENEMY_STATE_DEAD;
			enemies[i].active = 0;
			enemiesKilled++;
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
