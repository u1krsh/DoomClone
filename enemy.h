#ifndef ENEMY_H
#define ENEMY_H

#include "textures/BOSSA1_walk.h"
#include "textures/BOSSA2_walk.h"
#include "textures/BOSSA3_walk.h"
#include "textures/BOSSA1_attack.h"
#include "textures/BOSSA2_attack.h"
#include "textures/BOSSA3_attack.h"
#include "textures/BOSSA1_DIE.h"
#include "textures/BOSSA2_DIE.h"
#include "textures/BOSSA3_DIE.h"
#include "textures/cace_stat.h"
#include "textures/cace_attack.h"
#include "textures/cace_die.h"
#include "textures/cace_fire.h"

// Forward declaration to avoid implicit declaration error
void spawnProjectile(float x, float y, float z, float angle, int type, int currentTime);



// Enemy constants
#define MAX_ENEMIES 32
#define ENEMY_DETECTION_RADIUS 500  // Distance at which enemy starts following player
#define ENEMY_ATTACK_RADIUS 250      // Distance at which enemy attacks
#define ENEMY_SPEED 2               // Movement speed of enemies
#define ENEMY_COLLISION_RADIUS 25   // Collision radius for enemies

// Animation constants
#define ENEMY_ANIM_SPEED 150        // Milliseconds per frame
#define ENEMY_ATTACK_COOLDOWN 1500  // Milliseconds between attacks
#define ENEMY_HURT_DURATION 200     // Milliseconds to show hurt state
#define ENEMY_DEATH_DURATION 500    // Milliseconds for death animation

// Enemy types
#define ENEMY_TYPE_BOSSA1 0
#define ENEMY_TYPE_BOSSA2 1
#define ENEMY_TYPE_BOSSA3 2
#define ENEMY_TYPE_CACE   3
#define NUM_ENEMY_TYPES 4

// Cacodemon specific constants
#define CACE_FLOAT_SPEED 0.005f
#define CACE_FLOAT_AMPLITUDE 15
#define CACE_BASE_Z 0

// Enemy states
#define ENEMY_STATE_IDLE 0
#define ENEMY_STATE_CHASING 1
#define ENEMY_STATE_ATTACKING 2
#define ENEMY_STATE_HURT 3
#define ENEMY_STATE_DYING 4
#define ENEMY_STATE_DEAD 5
#define ENEMY_STATE_FLANKING 6
#define ENEMY_STATE_RETREATING 7
#define ENEMY_STATE_STRAFING 8
#define ENEMY_STATE_ALERT 9

// Advanced AI Parameters
#define ENEMY_STRAFE_SPEED 1.5f       // Speed multiplier when strafing
#define ENEMY_RETREAT_HEALTH_PCT 30   // Health % to start retreating
#define ENEMY_STRAFE_DURATION 400     // ms to strafe in one direction
#define ENEMY_FLANK_CHANCE 25         // % chance to try flanking per second
#define ENEMY_PACK_ALERT_RADIUS 400   // Radius to alert other enemies
#define ENEMY_PREDICTION_FACTOR 0.4f  // How much to lead shots (0-1)
#define ENEMY_SIGHT_CHECK_INTERVAL 200 // ms between LOS checks
#define ENEMY_BEHAVIOR_UPDATE_INTERVAL 100 // ms between behavior changes

// Enemy health by type
#define BOSSA1_HEALTH 100
#define BOSSA2_HEALTH 150
#define BOSSA3_HEALTH 200
#define CACE_HEALTH   120

// Enemy damage by type
#define BOSSA1_DAMAGE 10
#define BOSSA2_DAMAGE 15
#define BOSSA3_DAMAGE 20
#define CACE_DAMAGE   12

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
	float trueZ;        // Precise Z position for smooth movement
	float driftPhase;   // Random phase for organic floating
	
	// Advanced AI fields
	int strafeDir;          // -1 = left, +1 = right, 0 = none
	int strafeEndTime;      // When to switch strafe direction
	int lastKnownPlayerX;   // Last seen player X position
	int lastKnownPlayerY;   // Last seen player Y position
	int lastSeenPlayerTime; // When player was last visible
	int canSeePlayer;       // Current visibility status
	int lastSightCheck;     // Last time LOS was checked
	int lastBehaviorTime;   // Last time behavior was updated
	int tacticalState;      // Sub-state for complex maneuvers
	float flankAngle;       // Offset angle for flanking movement
	int alerted;            // Has been alerted by another enemy
	int alertTime;          // When this enemy was alerted
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
		case ENEMY_TYPE_CACE:   return CACE_HEALTH;
		default: return BOSSA1_HEALTH;
	}
}

// Get enemy damage by type
int getEnemyDamageByType(int enemyType) {
	switch (enemyType) {
		case ENEMY_TYPE_BOSSA1: return BOSSA1_DAMAGE;
		case ENEMY_TYPE_BOSSA2: return BOSSA2_DAMAGE;
		case ENEMY_TYPE_BOSSA3: return BOSSA3_DAMAGE;
		case ENEMY_TYPE_CACE:   return CACE_DAMAGE;
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
		// Initialize advanced AI fields
		enemies[i].strafeDir = 0;
		enemies[i].strafeEndTime = 0;
		enemies[i].lastKnownPlayerX = 0;
		enemies[i].lastKnownPlayerY = 0;
		enemies[i].lastSeenPlayerTime = 0;
		enemies[i].canSeePlayer = 0;
		enemies[i].lastSightCheck = 0;
		enemies[i].lastBehaviorTime = 0;
		enemies[i].tacticalState = 0;
		enemies[i].flankAngle = 0.0f;
		enemies[i].alerted = 0;
		enemies[i].alertTime = 0;
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
	enemies[numEnemies].trueZ = (float)z;
	enemies[numEnemies].driftPhase = (float)(rand() % 100) * 0.1f;
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
	// Initialize advanced AI fields
	enemies[numEnemies].strafeDir = (rand() % 2) * 2 - 1; // Random -1 or +1
	enemies[numEnemies].strafeEndTime = 0;
	enemies[numEnemies].lastKnownPlayerX = 0;
	enemies[numEnemies].lastKnownPlayerY = 0;
	enemies[numEnemies].lastSeenPlayerTime = 0;
	enemies[numEnemies].canSeePlayer = 0;
	enemies[numEnemies].lastSightCheck = 0;
	enemies[numEnemies].lastBehaviorTime = 0;
	enemies[numEnemies].tacticalState = 0;
	enemies[numEnemies].flankAngle = ((float)(rand() % 60) - 30.0f) * 0.0174533f; // -30 to +30 degrees in radians
	enemies[numEnemies].alerted = 0;
	enemies[numEnemies].alertTime = 0;
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
		case ENEMY_TYPE_CACE:   return 1; // Idle is single frame
		default: return BOSSA1_FRAME_COUNT;
	}
}

// Damage an enemy
void damageEnemy(int enemyIndex, int damage, int currentTime) {
	if (enemyIndex < 0 || enemyIndex >= numEnemies) return;
	if (!enemies[enemyIndex].active) return;
	if (enemies[enemyIndex].state == ENEMY_STATE_DEAD || 
	    enemies[enemyIndex].state == ENEMY_STATE_DYING) return;
	
	// Apply damage multiplier from powerups (berserk)
	extern float getDamageMultiplier(int currentTime, int weaponType);
	extern int weapon_currentWeapon;  // Access current weapon
	float mult = getDamageMultiplier(currentTime, weapon_currentWeapon);
	damage = (int)(damage * mult);
	
	enemies[enemyIndex].health -= damage;
	
	// Spawn blood particles at enemy position
	extern void spawnBloodParticles(int, int, int, int);
	spawnBloodParticles(enemies[enemyIndex].x, enemies[enemyIndex].y, 
	                    enemies[enemyIndex].z, 5 + (damage / 10));
	
	// Add screen shake when hitting enemy
	extern void addScreenShake(float amount);
	addScreenShake(1.0f + (damage / 30.0f));
	
	if (enemies[enemyIndex].health <= 0) {
		enemies[enemyIndex].health = 0;
		enemies[enemyIndex].state = ENEMY_STATE_DYING;
		enemies[enemyIndex].stateStartTime = currentTime;
		enemies[enemyIndex].animFrame = 0;
		
		// Register kill for kill streak system
		extern void registerKill(int currentTime);
		registerKill(currentTime);
		
		// Spawn extra blood on death
		spawnBloodParticles(enemies[enemyIndex].x, enemies[enemyIndex].y,
		                    enemies[enemyIndex].z, 15);
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
	
	// Check for invulnerability powerup
	extern int isInvulnerable(int currentTime);
	if (isInvulnerable(currentTime)) return;
	
	// Reset kill streak when taking damage
	extern void resetKillStreak(void);
	resetKillStreak();
	
	// Add screen shake when taking damage
	extern void addScreenShake(float amount);
	addScreenShake(3.0f + (damage / 10.0f));
	
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
		// Start screen melt when player dies
		extern void startScreenMelt(void);
		startScreenMelt();
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

// ============================================
// ADVANCED AI HELPER FUNCTIONS
// ============================================

// Forward declarations for wall data access
extern walls W[];
extern int numWall;

// Line segment intersection test
// Returns 1 if line AB intersects line CD
int lineIntersectsLine(float ax, float ay, float bx, float by, 
                       float cx, float cy, float dx, float dy) {
	float denominator = ((bx - ax) * (dy - cy)) - ((by - ay) * (dx - cx));
	if (denominator == 0) return 0; // Parallel lines
	
	float t = (((cx - ax) * (dy - cy)) - ((cy - ay) * (dx - cx))) / denominator;
	float u = (((cx - ax) * (by - ay)) - ((cy - ay) * (bx - ax))) / denominator;
	
	return (t >= 0 && t <= 1 && u >= 0 && u <= 1);
}

// Check if enemy has line of sight to player
// Uses simple raycast against walls
int enemyCanSeePlayer(int enemyIndex, int playerX, int playerY) {
	if (enemyIndex < 0 || enemyIndex >= numEnemies) return 0;
	
	float ex = (float)enemies[enemyIndex].x;
	float ey = (float)enemies[enemyIndex].y;
	float px = (float)playerX;
	float py = (float)playerY;
	
	// Check intersection with each wall
	for (int w = 0; w < numWall; w++) {
		float wx1 = (float)W[w].x1;
		float wy1 = (float)W[w].y1;
		float wx2 = (float)W[w].x2;
		float wy2 = (float)W[w].y2;
		
		if (lineIntersectsLine(ex, ey, px, py, wx1, wy1, wx2, wy2)) {
			return 0; // Wall blocks view
		}
	}
	
	return 1; // No walls blocking
}

// Alert nearby enemies to player position (pack behavior)
void alertNearbyEnemies(int alerterIndex, int playerX, int playerY, int currentTime) {
	if (alerterIndex < 0 || alerterIndex >= numEnemies) return;
	
	int alerterX = enemies[alerterIndex].x;
	int alerterY = enemies[alerterIndex].y;
	
	for (int i = 0; i < numEnemies; i++) {
		if (i == alerterIndex) continue;
		if (!enemies[i].active) continue;
		if (enemies[i].state == ENEMY_STATE_DEAD || enemies[i].state == ENEMY_STATE_DYING) continue;
		
		// Check distance to alerter
		int dx = enemies[i].x - alerterX;
		int dy = enemies[i].y - alerterY;
		int distSq = dx * dx + dy * dy;
		
		if (distSq < ENEMY_PACK_ALERT_RADIUS * ENEMY_PACK_ALERT_RADIUS) {
			// Alert this enemy
			if (!enemies[i].alerted || enemies[i].state == ENEMY_STATE_IDLE) {
				enemies[i].alerted = 1;
				enemies[i].alertTime = currentTime;
				enemies[i].lastKnownPlayerX = playerX;
				enemies[i].lastKnownPlayerY = playerY;
				
				// If idle, switch to alert state
				if (enemies[i].state == ENEMY_STATE_IDLE) {
					enemies[i].state = ENEMY_STATE_ALERT;
					enemies[i].stateStartTime = currentTime;
				}
			}
		}
	}
}

// Predict where player will be based on movement (for leading shots)
void predictPlayerPosition(int enemyX, int enemyY, int playerX, int playerY, 
                          int lastPlayerX, int lastPlayerY, float predictionFactor,
                          int* targetX, int* targetY) {
	// Calculate player velocity
	int velX = playerX - lastPlayerX;
	int velY = playerY - lastPlayerY;
	
	// Calculate distance to enemy
	int dx = enemyX - playerX;
	int dy = enemyY - playerY;
	float dist = sqrt(dx * dx + dy * dy);
	
	// Predict based on distance and velocity
	// Further enemies need more prediction
	float predictionScale = predictionFactor * (dist / 200.0f);
	if (predictionScale > 1.5f) predictionScale = 1.5f;
	
	*targetX = playerX + (int)(velX * predictionScale * 10.0f);
	*targetY = playerY + (int)(velY * predictionScale * 10.0f);
}

// Calculate distance from point to line segment
float pointToSegmentDist(float px, float py, float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float lenSq = dx * dx + dy * dy;
	
	if (lenSq < 0.001f) {
		// Degenerate segment - just return distance to point
		float dpx = px - x1;
		float dpy = py - y1;
		return sqrt(dpx * dpx + dpy * dpy);
	}
	
	// Calculate projection parameter
	float t = ((px - x1) * dx + (py - y1) * dy) / lenSq;
	
	// Clamp to segment
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	
	// Closest point on segment
	float closestX = x1 + t * dx;
	float closestY = y1 + t * dy;
	
	// Distance from point to closest point
	float distX = px - closestX;
	float distY = py - closestY;
	
	return sqrt(distX * distX + distY * distY);
}

// Check if enemy at position collides with any wall (using collision radius)
// Returns 1 if position is too close to a wall, 0 if clear
int enemyPositionCollidesWithWall(int posX, int posY) {
	float px = (float)posX;
	float py = (float)posY;
	float collisionDist = (float)ENEMY_COLLISION_RADIUS;
	
	// Check distance to all walls
	for (int w = 0; w < numWall; w++) {
		// Skip degenerate walls (zero-length)
		if (W[w].x1 == W[w].x2 && W[w].y1 == W[w].y2) continue;
		
		float wx1 = (float)W[w].x1;
		float wy1 = (float)W[w].y1;
		float wx2 = (float)W[w].x2;
		float wy2 = (float)W[w].y2;
		
		// Check distance from enemy position to wall segment
		float dist = pointToSegmentDist(px, py, wx1, wy1, wx2, wy2);
		
		if (dist < collisionDist) {
			return 1; // Too close to wall
		}
	}
	
	return 0; // Position is clear
}

// Check if enemy movement collides with walls (path OR destination)
// Returns 1 if path is blocked, 0 if clear
// Uses multiple sample points along the path for more robust collision detection
int enemyCollidesWithWall(int fromX, int fromY, int toX, int toY) {
	float fx = (float)fromX;
	float fy = (float)fromY;
	float tx = (float)toX;
	float ty = (float)toY;
	
	// Calculate path length and number of samples
	float dx = tx - fx;
	float dy = ty - fy;
	float pathLen = sqrt(dx * dx + dy * dy);
	
	// Sample every 5 units along the path, minimum 2 samples (start and end)
	int numSamples = (int)(pathLen / 5.0f) + 2;
	if (numSamples > 10) numSamples = 10; // Cap samples for performance
	
	// Check each sample point along the path
	for (int s = 0; s <= numSamples; s++) {
		float t = (float)s / (float)numSamples;
		int sampleX = (int)(fx + dx * t);
		int sampleY = (int)(fy + dy * t);
		
		if (enemyPositionCollidesWithWall(sampleX, sampleY)) {
			return 1; // Too close to wall at this sample point
		}
	}
	
	// Also check for actual line intersection with walls
	for (int w = 0; w < numWall; w++) {
		if (W[w].x1 == W[w].x2 && W[w].y1 == W[w].y2) continue;
		
		float wx1 = (float)W[w].x1;
		float wy1 = (float)W[w].y1;
		float wx2 = (float)W[w].x2;
		float wy2 = (float)W[w].y2;
		
		if (lineIntersectsLine(fx, fy, tx, ty, wx1, wy1, wx2, wy2)) {
			return 1; // Path crosses wall
		}
	}
	
	return 0; // No collision
}

// Push enemy away from nearest wall if too close
// This prevents enemies from getting stuck inside walls
void pushEnemyAwayFromWalls(int enemyIndex) {
	if (enemyIndex < 0 || enemyIndex >= numEnemies) return;
	
	float px = (float)enemies[enemyIndex].x;
	float py = (float)enemies[enemyIndex].y;
	float pushDist = (float)ENEMY_COLLISION_RADIUS + 2.0f;
	
	for (int w = 0; w < numWall; w++) {
		// Skip degenerate walls
		if (W[w].x1 == W[w].x2 && W[w].y1 == W[w].y2) continue;
		
		float wx1 = (float)W[w].x1;
		float wy1 = (float)W[w].y1;
		float wx2 = (float)W[w].x2;
		float wy2 = (float)W[w].y2;
		
		float dist = pointToSegmentDist(px, py, wx1, wy1, wx2, wy2);
		
		if (dist < pushDist && dist > 0.01f) {
			// Calculate wall direction
			float wdx = wx2 - wx1;
			float wdy = wy2 - wy1;
			float wLen = sqrt(wdx * wdx + wdy * wdy);
			if (wLen < 0.01f) continue;
			
			// Calculate wall normal (perpendicular)
			float nx = -wdy / wLen;
			float ny = wdx / wLen;
			
			// Determine which side enemy is on by checking dot product
			float toEnemyX = px - wx1;
			float toEnemyY = py - wy1;
			float dot = toEnemyX * nx + toEnemyY * ny;
			
			// Push in the direction away from wall
			float pushAmount = (pushDist - dist) * 0.5f;
			if (dot < 0) {
				nx = -nx;
				ny = -ny;
			}
			
			enemies[enemyIndex].x += (int)(nx * pushAmount);
			enemies[enemyIndex].y += (int)(ny * pushAmount);
			
			// Update px, py for subsequent wall checks
			px = (float)enemies[enemyIndex].x;
			py = (float)enemies[enemyIndex].y;
		}
	}
}

// Move enemy with wall collision detection
// Returns 1 if movement was successful (at least partially), 0 if fully blocked
int moveEnemyWithCollision(int enemyIndex, int moveX, int moveY) {
	if (enemyIndex < 0 || enemyIndex >= numEnemies) return 0;
	
	int oldX = enemies[enemyIndex].x;
	int oldY = enemies[enemyIndex].y;
	int newX = oldX + moveX;
	int newY = oldY + moveY;
	
	// First, push enemy away from walls if currently too close (fixes stuck enemies)
	pushEnemyAwayFromWalls(enemyIndex);
	
	// Update positions after push
	oldX = enemies[enemyIndex].x;
	oldY = enemies[enemyIndex].y;
	newX = oldX + moveX;
	newY = oldY + moveY;
	
	// Try full movement first
	if (!enemyCollidesWithWall(oldX, oldY, newX, newY)) {
		enemies[enemyIndex].x = newX;
		enemies[enemyIndex].y = newY;
		return 1;
	}
	
	// Try X movement only (slide along wall)
	if (moveX != 0 && !enemyCollidesWithWall(oldX, oldY, newX, oldY)) {
		enemies[enemyIndex].x = newX;
		return 1;
	}
	
	// Try Y movement only (slide along wall)
	if (moveY != 0 && !enemyCollidesWithWall(oldX, oldY, oldX, newY)) {
		enemies[enemyIndex].y = newY;
		return 1;
	}
	
	// Try reduced movement (half step)
	int halfMoveX = moveX / 2;
	int halfMoveY = moveY / 2;
	if ((halfMoveX != 0 || halfMoveY != 0) && 
	    !enemyCollidesWithWall(oldX, oldY, oldX + halfMoveX, oldY + halfMoveY)) {
		enemies[enemyIndex].x = oldX + halfMoveX;
		enemies[enemyIndex].y = oldY + halfMoveY;
		return 1;
	}
	
	// Fully blocked
	return 0;
}

// Track player position for velocity calculation
static int prevPlayerX = 0;
static int prevPlayerY = 0;

// Update all enemies
void updateEnemies(int playerX, int playerY, int playerZ, int currentTime) {
	if (!enemiesEnabled) return;
	
	int i;
	for (i = 0; i < numEnemies; i++) {
		if (!enemies[i].active) continue;

		// Handle Cacodemon Organic Floating
		if (enemies[i].enemyType == ENEMY_TYPE_CACE && enemies[i].state != ENEMY_STATE_DYING && enemies[i].state != ENEMY_STATE_DEAD) {
			float targetZ = (float)playerZ;
			// Smoothly move towards targetZ
			float diff = targetZ - enemies[i].trueZ;
			enemies[i].trueZ += diff * 0.02f;
			
			// Add Organic Drift
			float timeVal = currentTime * 0.002f + enemies[i].driftPhase;
			float organicOffset = sin(timeVal) * 12.0f + sin(timeVal * 2.3f) * 6.0f;
			
			enemies[i].z = (int)(enemies[i].trueZ + organicOffset + 20); 
		}
		
		// Handle death state
		if (enemies[i].state == ENEMY_STATE_DYING) {
			// Animate death
			int dieFrames = BOSSA1_DIE_FRAME_COUNT; // Default to BOSSA1
			if (enemies[i].enemyType == ENEMY_TYPE_BOSSA1) dieFrames = BOSSA1_DIE_FRAME_COUNT;
			else if (enemies[i].enemyType == ENEMY_TYPE_BOSSA2) dieFrames = BOSSA2_DIE_FRAME_COUNT;
			else if (enemies[i].enemyType == ENEMY_TYPE_BOSSA3) dieFrames = BOSSA3_DIE_FRAME_COUNT;
			else if (enemies[i].enemyType == ENEMY_TYPE_CACE) dieFrames = CACE_DIE_FRAME_COUNT;
			
			// Update Z to keep on ground (user requested slightly lower)
			enemies[i].z = -3;

			if (currentTime - enemies[i].lastAnimTime >= 150) { // 150ms per frame
				enemies[i].animFrame++;
				enemies[i].lastAnimTime = currentTime;
				
				if (enemies[i].animFrame >= dieFrames) {
					enemies[i].state = ENEMY_STATE_DEAD;
					enemies[i].animFrame = dieFrames - 1; // Keep last frame
					enemiesKilled++;
					enemies[i].z = -3;
				}
			}
			continue;
		}
		
		if (enemies[i].state == ENEMY_STATE_DEAD) {
			enemies[i].z = -3;
			continue;
		}
		
		// Handle hurt state recovery (but don't skip movement!)
		if (enemies[i].state == ENEMY_STATE_HURT) {
			if (currentTime - enemies[i].stateStartTime >= ENEMY_HURT_DURATION) {
				enemies[i].state = ENEMY_STATE_CHASING;
			}
		}
		
		// ============================================
		// ADVANCED AI: Line of Sight Check
		// ============================================
		if (currentTime - enemies[i].lastSightCheck >= ENEMY_SIGHT_CHECK_INTERVAL) {
			enemies[i].canSeePlayer = enemyCanSeePlayer(i, playerX, playerY);
			enemies[i].lastSightCheck = currentTime;
			
			// If we can see the player, update last known position and alert others
			if (enemies[i].canSeePlayer) {
				enemies[i].lastKnownPlayerX = playerX;
				enemies[i].lastKnownPlayerY = playerY;
				enemies[i].lastSeenPlayerTime = currentTime;
				
				// Alert nearby enemies (pack behavior)
				if (enemies[i].state == ENEMY_STATE_IDLE || !enemies[i].alerted) {
					alertNearbyEnemies(i, playerX, playerY, currentTime);
				}
			}
		}
		
		// Calculate distance to player
		int dist = enemyDist(enemies[i].x, enemies[i].y, playerX, playerY);
		
		// ============================================
		// ADVANCED AI: Health-based Behavior
		// ============================================
		float healthPercent = (float)enemies[i].health / (float)enemies[i].maxHealth * 100.0f;
		int shouldRetreat = (healthPercent < ENEMY_RETREAT_HEALTH_PCT && dist < ENEMY_ATTACK_RADIUS * 2);
		
		// ============================================
		// ADVANCED AI: Tactical State Decisions
		// ============================================
		if (currentTime - enemies[i].lastBehaviorTime >= ENEMY_BEHAVIOR_UPDATE_INTERVAL) {
			enemies[i].lastBehaviorTime = currentTime;
			
			// Decide on tactical behavior
			if (shouldRetreat && enemies[i].state != ENEMY_STATE_HURT) {
				// Low health - retreat while shooting
				enemies[i].state = ENEMY_STATE_RETREATING;
				enemies[i].stateStartTime = currentTime;
			}
			else if (dist < ENEMY_ATTACK_RADIUS && enemies[i].canSeePlayer) {
				// In attack range with LOS - attack with strafing
				if (enemies[i].state != ENEMY_STATE_HURT) {
					// Randomly decide to strafe or attack directly
					if (rand() % 100 < 60) {
						enemies[i].state = ENEMY_STATE_STRAFING;
					} else {
						enemies[i].state = ENEMY_STATE_ATTACKING;
					}
				}
			}
			else if (dist < ENEMY_DETECTION_RADIUS && enemies[i].canSeePlayer) {
				// Can see player, in detection range
				if (enemies[i].state != ENEMY_STATE_HURT) {
					// Decide to flank or chase directly
					if (rand() % 100 < ENEMY_FLANK_CHANCE && dist > ENEMY_ATTACK_RADIUS) {
						enemies[i].state = ENEMY_STATE_FLANKING;
						enemies[i].flankAngle = ((rand() % 2) * 2 - 1) * (0.5f + (rand() % 50) / 100.0f);
					} else {
						enemies[i].state = ENEMY_STATE_CHASING;
					}
				}
			}
			else if (enemies[i].alerted && currentTime - enemies[i].alertTime < 5000) {
				// Was alerted by another enemy - move to last known position
				enemies[i].state = ENEMY_STATE_ALERT;
			}
			else if (dist >= ENEMY_DETECTION_RADIUS && !enemies[i].alerted) {
				// Out of range and not alerted
				if (enemies[i].state != ENEMY_STATE_HURT) {
					enemies[i].state = ENEMY_STATE_IDLE;
					enemies[i].animFrame = 0;
				}
			}
		}
		
		// ============================================
		// ADVANCED AI: Strafe Direction Updates
		// ============================================
		if (currentTime >= enemies[i].strafeEndTime) {
			// Time to change strafe direction
			enemies[i].strafeDir = (rand() % 2) * 2 - 1; // -1 or +1
			enemies[i].strafeEndTime = currentTime + ENEMY_STRAFE_DURATION + (rand() % 300);
		}
		
		// ============================================
		// STATE-SPECIFIC BEHAVIOR
		// ============================================
		int dx = playerX - enemies[i].x;
		int dy = playerY - enemies[i].y;
		float angleToPlayer = atan2((float)dx, (float)dy);
		
		switch (enemies[i].state) {
			case ENEMY_STATE_ATTACKING:
			case ENEMY_STATE_STRAFING: {
				// Update attack animation
				int frameCount = 0;
				if (enemies[i].enemyType == ENEMY_TYPE_BOSSA1) frameCount = BOSSA1_ATTACK_FRAME_COUNT;
				else if (enemies[i].enemyType == ENEMY_TYPE_BOSSA2) frameCount = BOSSA2_ATTACK_FRAME_COUNT;
				else if (enemies[i].enemyType == ENEMY_TYPE_BOSSA3) frameCount = BOSSA3_ATTACK_FRAME_COUNT;
				else if (enemies[i].enemyType == ENEMY_TYPE_CACE) frameCount = CACE_ATTACK_FRAME_COUNT;
				else frameCount = 3;
				
				if (currentTime - enemies[i].lastAnimTime >= ENEMY_ANIM_SPEED) {
					enemies[i].animFrame = (enemies[i].animFrame + 1) % frameCount;
					enemies[i].lastAnimTime = currentTime;
				}
				
				// STRAFING: Move sideways while attacking
				if (enemies[i].state == ENEMY_STATE_STRAFING) {
					float strafeAngle = angleToPlayer + (3.14159f / 2.0f) * enemies[i].strafeDir;
					int moveX = (int)(sin(strafeAngle) * ENEMY_SPEED * ENEMY_STRAFE_SPEED);
					int moveY = (int)(cos(strafeAngle) * ENEMY_SPEED * ENEMY_STRAFE_SPEED);
					moveEnemyWithCollision(i, moveX, moveY);
				}
				
				// Check attack cooldown - shoot with prediction
				if (currentTime - enemies[i].lastAttackTime >= ENEMY_ATTACK_COOLDOWN) {
					// Predict player position for leading shots
					int targetX, targetY;
					predictPlayerPosition(enemies[i].x, enemies[i].y, playerX, playerY,
					                      prevPlayerX, prevPlayerY, ENEMY_PREDICTION_FACTOR,
					                      &targetX, &targetY);
					
					float aimAngle = atan2((float)(targetX - enemies[i].x), (float)(targetY - enemies[i].y));
					
					int projType = PROJ_TYPE_BOSSA1; // BOSSA1 default
					if (enemies[i].enemyType == ENEMY_TYPE_BOSSA2) projType = PROJ_TYPE_BULLET;
					else if (enemies[i].enemyType == ENEMY_TYPE_BOSSA3) projType = PROJ_TYPE_SHELL;
					else if (enemies[i].enemyType == ENEMY_TYPE_CACE) projType = PROJ_TYPE_FIREBALL;
					
					spawnProjectile((float)enemies[i].x, (float)enemies[i].y, (float)enemies[i].z + 15, aimAngle, projType, currentTime);
					enemies[i].lastAttackTime = currentTime;
				}
				break;
			}
			
			case ENEMY_STATE_CHASING: {
				// Move directly towards player with wall collision
				if (dist > ENEMY_COLLISION_RADIUS) {
					int moveX = (int)(sin(angleToPlayer) * ENEMY_SPEED);
					int moveY = (int)(cos(angleToPlayer) * ENEMY_SPEED);
					moveEnemyWithCollision(i, moveX, moveY);
				}
				
				// Update walk animation
				int frameCount = getEnemyFrameCount(enemies[i].enemyType);
				if (frameCount > 1 && currentTime - enemies[i].lastAnimTime >= ENEMY_ANIM_SPEED) {
					enemies[i].animFrame = (enemies[i].animFrame + 1) % frameCount;
					enemies[i].lastAnimTime = currentTime;
				}
				break;
			}
			
			case ENEMY_STATE_FLANKING: {
				// Move at an angle to the player (flanking maneuver) with wall collision
				float flankMoveAngle = angleToPlayer + enemies[i].flankAngle;
				if (dist > ENEMY_COLLISION_RADIUS) {
					int moveX = (int)(sin(flankMoveAngle) * ENEMY_SPEED * 1.2f);
					int moveY = (int)(cos(flankMoveAngle) * ENEMY_SPEED * 1.2f);
					moveEnemyWithCollision(i, moveX, moveY);
				}
				
				// If close enough, switch to attacking
				if (dist < ENEMY_ATTACK_RADIUS) {
					enemies[i].state = ENEMY_STATE_STRAFING;
				}
				
				// Update walk animation
				int frameCount = getEnemyFrameCount(enemies[i].enemyType);
				if (frameCount > 1 && currentTime - enemies[i].lastAnimTime >= ENEMY_ANIM_SPEED) {
					enemies[i].animFrame = (enemies[i].animFrame + 1) % frameCount;
					enemies[i].lastAnimTime = currentTime;
				}
				break;
			}
			
			case ENEMY_STATE_RETREATING: {
				// Move AWAY from player while still facing them (with wall collision)
				float retreatAngle = angleToPlayer + 3.14159f; // Opposite direction
				int moveX = (int)(sin(retreatAngle) * ENEMY_SPEED * 0.8f);
				int moveY = (int)(cos(retreatAngle) * ENEMY_SPEED * 0.8f);
				moveEnemyWithCollision(i, moveX, moveY);
				
				// Still shoot while retreating (faster cooldown)
				if (currentTime - enemies[i].lastAttackTime >= ENEMY_ATTACK_COOLDOWN * 0.7f) {
					float aimAngle = atan2((float)(playerX - enemies[i].x), (float)(playerY - enemies[i].y));
					
					int projType = PROJ_TYPE_BOSSA1; // BOSSA1 default
					if (enemies[i].enemyType == ENEMY_TYPE_BOSSA2) projType = PROJ_TYPE_BULLET;
					else if (enemies[i].enemyType == ENEMY_TYPE_BOSSA3) projType = PROJ_TYPE_SHELL;
					else if (enemies[i].enemyType == ENEMY_TYPE_CACE) projType = PROJ_TYPE_FIREBALL;
					
					spawnProjectile((float)enemies[i].x, (float)enemies[i].y, (float)enemies[i].z + 15, aimAngle, projType, currentTime);
					enemies[i].lastAttackTime = currentTime;
				}
				
				// Update attack animation during retreat
				int frameCount = 0;
				if (enemies[i].enemyType == ENEMY_TYPE_BOSSA1) frameCount = BOSSA1_ATTACK_FRAME_COUNT;
				else if (enemies[i].enemyType == ENEMY_TYPE_BOSSA2) frameCount = BOSSA2_ATTACK_FRAME_COUNT;
				else if (enemies[i].enemyType == ENEMY_TYPE_BOSSA3) frameCount = BOSSA3_ATTACK_FRAME_COUNT;
				else frameCount = 3;
				
				if (currentTime - enemies[i].lastAnimTime >= ENEMY_ANIM_SPEED) {
					enemies[i].animFrame = (enemies[i].animFrame + 1) % frameCount;
					enemies[i].lastAnimTime = currentTime;
				}
				break;
			}
			
			case ENEMY_STATE_ALERT: {
				// Move towards last known player position
				int alertDx = enemies[i].lastKnownPlayerX - enemies[i].x;
				int alertDy = enemies[i].lastKnownPlayerY - enemies[i].y;
				int alertDist = (int)sqrt(alertDx * alertDx + alertDy * alertDy);
				
				if (alertDist > 20) {
					float alertAngle = atan2((float)alertDx, (float)alertDy);
					int moveX = (int)(sin(alertAngle) * ENEMY_SPEED);
					int moveY = (int)(cos(alertAngle) * ENEMY_SPEED);
					moveEnemyWithCollision(i, moveX, moveY);
				} else {
					// Reached last known position, go back to idle
					enemies[i].state = ENEMY_STATE_IDLE;
					enemies[i].alerted = 0;
				}
				
				// Update walk animation
				int frameCount = getEnemyFrameCount(enemies[i].enemyType);
				if (frameCount > 1 && currentTime - enemies[i].lastAnimTime >= ENEMY_ANIM_SPEED) {
					enemies[i].animFrame = (enemies[i].animFrame + 1) % frameCount;
					enemies[i].lastAnimTime = currentTime;
				}
				break;
			}
			
			case ENEMY_STATE_IDLE:
			default:
				enemies[i].animFrame = 0;
				break;
		}
		
		// Keep enemy at same height as player (same plane) - EXCEPT Cacodemon
		if (enemies[i].enemyType != ENEMY_TYPE_CACE && 
		    enemies[i].state != ENEMY_STATE_DYING && 
		    enemies[i].state != ENEMY_STATE_DEAD) {
			enemies[i].z = playerZ;
		}
	}
	
	// Update previous player position for velocity tracking
	prevPlayerX = playerX;
	prevPlayerY = playerY;
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
