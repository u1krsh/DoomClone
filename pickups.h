#ifndef PICKUPS_H
#define PICKUPS_H

// Pickup System Module
// Health packs, armor, ammo, and special powerups

#include <math.h>

// Pickup types
#define PICKUP_HEALTH_SMALL 0    // +10 health
#define PICKUP_HEALTH_LARGE 1    // +25 health
#define PICKUP_ARMOR_SMALL 2     // +10 armor
#define PICKUP_ARMOR_LARGE 3     // +50 armor
#define PICKUP_AMMO_CLIP 4       // +10 pistol ammo
#define PICKUP_AMMO_SHELLS 5     // +4 shotgun shells
#define PICKUP_AMMO_BULLETS 6    // +20 chaingun ammo
#define PICKUP_BERSERK 7         // 2x fist damage for 30 seconds
#define PICKUP_INVULN 8          // Invulnerability for 15 seconds
#define PICKUP_SPEED 9           // 1.5x speed for 20 seconds
#define NUM_PICKUP_TYPES 10

#define MAX_PICKUPS 64
#define PICKUP_RADIUS 20         // Collection radius
#define PICKUP_BOB_SPEED 4       // Floating animation speed
#define PICKUP_BOB_HEIGHT 5      // Floating animation height
#define PICKUP_RESPAWN_TIME 30000 // 30 seconds respawn

// Powerup durations (ms)
#define BERSERK_DURATION 30000
#define INVULN_DURATION 15000
#define SPEED_DURATION 20000

// Pickup structure
typedef struct {
    int active;
    int type;
    int x, y, z;              // World position
    int respawnTime;          // When to respawn (0 = never)
    int collected;            // Has been collected
    int collectedTime;        // When it was collected
    float bobPhase;           // Animation phase
} Pickup;

// Active powerups
typedef struct {
    int berserkEndTime;       // When berserk ends (0 = not active)
    int invulnEndTime;        // When invulnerability ends
    int speedEndTime;         // When speed boost ends
} ActivePowerups;

Pickup pickups[MAX_PICKUPS];
int numPickups = 0;
ActivePowerups powerups = {0, 0, 0};

// Get pickup color for rendering
void getPickupColor(int type, int* r, int* g, int* b) {
    switch (type) {
        case PICKUP_HEALTH_SMALL:
        case PICKUP_HEALTH_LARGE:
            *r = 255; *g = 50; *b = 50;   // Red
            break;
        case PICKUP_ARMOR_SMALL:
        case PICKUP_ARMOR_LARGE:
            *r = 50; *g = 100; *b = 255;  // Blue
            break;
        case PICKUP_AMMO_CLIP:
        case PICKUP_AMMO_SHELLS:
        case PICKUP_AMMO_BULLETS:
            *r = 255; *g = 200; *b = 50;  // Yellow/Gold
            break;
        case PICKUP_BERSERK:
            *r = 255; *g = 0; *b = 128;   // Magenta
            break;
        case PICKUP_INVULN:
            *r = 0; *g = 255; *b = 200;   // Cyan
            break;
        case PICKUP_SPEED:
            *r = 0; *g = 255; *b = 0;     // Green
            break;
        default:
            *r = 255; *g = 255; *b = 255;
    }
}

// Get pickup size for rendering
int getPickupSize(int type) {
    switch (type) {
        case PICKUP_HEALTH_LARGE:
        case PICKUP_ARMOR_LARGE:
        case PICKUP_BERSERK:
        case PICKUP_INVULN:
        case PICKUP_SPEED:
            return 12;
        default:
            return 8;
    }
}

// Initialize pickup system
void initPickups(void) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        pickups[i].active = 0;
    }
    numPickups = 0;
    
    powerups.berserkEndTime = 0;
    powerups.invulnEndTime = 0;
    powerups.speedEndTime = 0;
}

// Add a pickup to the world
void addPickup(int type, int x, int y, int z, int respawns) {
    if (numPickups >= MAX_PICKUPS) return;
    
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!pickups[i].active) {
            pickups[i].active = 1;
            pickups[i].type = type;
            pickups[i].x = x;
            pickups[i].y = y;
            pickups[i].z = z;
            pickups[i].respawnTime = respawns ? PICKUP_RESPAWN_TIME : 0;
            pickups[i].collected = 0;
            pickups[i].bobPhase = (float)(rand() % 360);
            numPickups++;
            break;
        }
    }
}

// Check if player can pick up item (not at max already)
int canPickup(int type) {
    extern int playerHealth, playerMaxHealth;
    extern int playerArmor, playerMaxArmor;
    extern WeaponState weapon;
    
    switch (type) {
        case PICKUP_HEALTH_SMALL:
        case PICKUP_HEALTH_LARGE:
            return playerHealth < playerMaxHealth;
        case PICKUP_ARMOR_SMALL:
        case PICKUP_ARMOR_LARGE:
            return playerArmor < playerMaxArmor;
        case PICKUP_AMMO_CLIP:
            return weapon.ammo[WEAPON_PISTOL] < weapon.maxAmmo[WEAPON_PISTOL];
        case PICKUP_AMMO_SHELLS:
            return weapon.ammo[WEAPON_SHOTGUN] < weapon.maxAmmo[WEAPON_SHOTGUN];
        case PICKUP_AMMO_BULLETS:
            return weapon.ammo[WEAPON_CHAINGUN] < weapon.maxAmmo[WEAPON_CHAINGUN];
        case PICKUP_BERSERK:
        case PICKUP_INVULN:
        case PICKUP_SPEED:
            return 1;  // Always can pick up powerups
        default:
            return 0;
    }
}

// Apply pickup effect
void applyPickup(int type, int currentTime) {
    extern int playerHealth, playerMaxHealth;
    extern int playerArmor, playerMaxArmor;
    extern void healPlayer(int amount);
    extern void addArmor(int amount);
    extern void addAmmo(int weaponType, int amount);
    extern void triggerFlash(int r, int g, int b, int currentTime);
    
    int r, g, b;
    getPickupColor(type, &r, &g, &b);
    
    switch (type) {
        case PICKUP_HEALTH_SMALL:
            healPlayer(10);
            triggerFlash(255, 100, 100, currentTime);
            break;
        case PICKUP_HEALTH_LARGE:
            healPlayer(25);
            triggerFlash(255, 50, 50, currentTime);
            break;
        case PICKUP_ARMOR_SMALL:
            addArmor(10);
            triggerFlash(100, 100, 255, currentTime);
            break;
        case PICKUP_ARMOR_LARGE:
            addArmor(50);
            triggerFlash(50, 50, 255, currentTime);
            break;
        case PICKUP_AMMO_CLIP:
            addAmmo(WEAPON_PISTOL, 10);
            triggerFlash(255, 200, 50, currentTime);
            break;
        case PICKUP_AMMO_SHELLS:
            addAmmo(WEAPON_SHOTGUN, 4);
            triggerFlash(255, 200, 50, currentTime);
            break;
        case PICKUP_AMMO_BULLETS:
            addAmmo(WEAPON_CHAINGUN, 20);
            triggerFlash(255, 200, 50, currentTime);
            break;
        case PICKUP_BERSERK:
            powerups.berserkEndTime = currentTime + BERSERK_DURATION;
            healPlayer(100);  // Berserk also heals
            triggerFlash(255, 0, 0, currentTime);
            break;
        case PICKUP_INVULN:
            powerups.invulnEndTime = currentTime + INVULN_DURATION;
            triggerFlash(0, 255, 255, currentTime);
            break;
        case PICKUP_SPEED:
            powerups.speedEndTime = currentTime + SPEED_DURATION;
            triggerFlash(0, 255, 0, currentTime);
            break;
    }
}

// Check if berserk is active
int isBerserkActive(int currentTime) {
    return currentTime < powerups.berserkEndTime;
}

// Check if invulnerable
int isInvulnerable(int currentTime) {
    return currentTime < powerups.invulnEndTime;
}

// Check if speed boosted
int isSpeedBoosted(int currentTime) {
    return currentTime < powerups.speedEndTime;
}

// Get speed multiplier
float getSpeedMultiplier(int currentTime) {
    return isSpeedBoosted(currentTime) ? 1.5f : 1.0f;
}

// Get damage multiplier (for berserk)
float getDamageMultiplier(int currentTime, int weaponType) {
    // Berserk only affects fist
    if (weaponType == WEAPON_FIST && isBerserkActive(currentTime)) {
        return 3.0f;  // Triple damage with fist
    }
    return 1.0f;
}

// Update pickups (handle respawns and collection)
void updatePickups(int playerX, int playerY, int playerZ, int currentTime) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!pickups[i].active) continue;
        
        // Update bob animation
        pickups[i].bobPhase += PICKUP_BOB_SPEED;
        if (pickups[i].bobPhase >= 360.0f) pickups[i].bobPhase -= 360.0f;
        
        // Handle respawn
        if (pickups[i].collected) {
            if (pickups[i].respawnTime > 0 && 
                currentTime - pickups[i].collectedTime >= pickups[i].respawnTime) {
                pickups[i].collected = 0;
            }
            continue;  // Skip collection check for collected items
        }
        
        // Check for collection
        int dx = playerX - pickups[i].x;
        int dy = playerY - pickups[i].y;
        int dz = playerZ - pickups[i].z;
        int distSq = dx * dx + dy * dy + dz * dz;
        
        if (distSq < PICKUP_RADIUS * PICKUP_RADIUS) {
            if (canPickup(pickups[i].type)) {
                applyPickup(pickups[i].type, currentTime);
                pickups[i].collected = 1;
                pickups[i].collectedTime = currentTime;
                
                // If no respawn, deactivate
                if (pickups[i].respawnTime == 0) {
                    pickups[i].active = 0;
                    numPickups--;
                }
            }
        }
    }
}

// Draw pickups as simple 3D sprites
void drawPickups(void (*pixelFunc)(int, int, int, int, int),
                 int screenWidth, int screenHeight,
                 int playerX, int playerY, int playerZ, int playerAngle,
                 float* cosTable, float* sinTable,
                 float* depthBuffer, int currentTime) {
    float CS = cosTable[playerAngle];
    float SN = sinTable[playerAngle];
    
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!pickups[i].active || pickups[i].collected) continue;
        
        // Calculate bobbing offset
        int phaseInt = (int)pickups[i].bobPhase % 360;
        float bobOffset = sinTable[phaseInt] * PICKUP_BOB_HEIGHT;
        
        // Transform to camera space
        float relX = (float)(pickups[i].x - playerX);
        float relY = (float)(pickups[i].y - playerY);
        float relZ = (float)(pickups[i].z - playerZ) + bobOffset;
        
        float camX = relX * CS - relY * SN;
        float camY = relX * SN + relY * CS;
        
        if (camY < 1.0f) continue;  // Behind player
        
        // Depth test
        int screenX = (int)(camX * 200.0f / camY + screenWidth / 2);
        if (screenX < 0 || screenX >= screenWidth) continue;
        if (camY > depthBuffer[screenX]) continue;
        
        int screenY = (int)(relZ * 200.0f / camY + screenHeight / 2);
        
        // Get pickup properties
        int r, g, b;
        getPickupColor(pickups[i].type, &r, &g, &b);
        int size = (int)(getPickupSize(pickups[i].type) * 200.0f / camY);
        if (size < 2) size = 2;
        if (size > 50) size = 50;
        
        // Pulsing glow effect
        float pulse = (sinTable[(int)(pickups[i].bobPhase * 2) % 360] + 1.0f) / 2.0f;
        pulse = 0.7f + pulse * 0.3f;
        r = (int)(r * pulse);
        g = (int)(g * pulse);
        b = (int)(b * pulse);
        
        // Draw pickup as a diamond shape
        for (int dy = -size; dy <= size; dy++) {
            int width = size - abs(dy);
            for (int dx = -width; dx <= width; dx++) {
                int px = screenX + dx;
                int py = screenY + dy;
                if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight) {
                    // Only draw if in front of walls
                    if (camY < depthBuffer[px]) {
                        pixelFunc(px, py, r, g, b);
                    }
                }
            }
        }
    }
}

// Draw powerup status indicators
void drawPowerupStatus(void (*pixelFunc)(int, int, int, int, int),
                       int screenWidth, int screenHeight,
                       int currentTime) {
    extern void drawString(int, int, const char*, int, int, int, void (*)(int, int, int, int, int));
    
    int y = screenHeight - 50;
    int x = 10;
    char text[32];
    
    // Berserk indicator
    if (isBerserkActive(currentTime)) {
        int remaining = (powerups.berserkEndTime - currentTime) / 1000;
        snprintf(text, sizeof(text), "BERSERK %ds", remaining);
        // Pulsing red
        int pulse = (currentTime / 100) % 2 ? 255 : 200;
        drawString(x, y, text, pulse, 0, 100, pixelFunc);
        y -= 12;
    }
    
    // Invulnerability indicator
    if (isInvulnerable(currentTime)) {
        int remaining = (powerups.invulnEndTime - currentTime) / 1000;
        snprintf(text, sizeof(text), "INVULN %ds", remaining);
        // Pulsing cyan
        int pulse = (currentTime / 100) % 2 ? 255 : 200;
        drawString(x, y, text, 0, pulse, pulse, pixelFunc);
        y -= 12;
    }
    
    // Speed indicator
    if (isSpeedBoosted(currentTime)) {
        int remaining = (powerups.speedEndTime - currentTime) / 1000;
        snprintf(text, sizeof(text), "SPEED %ds", remaining);
        // Pulsing green
        int pulse = (currentTime / 100) % 2 ? 255 : 200;
        drawString(x, y, text, 0, pulse, 0, pixelFunc);
    }
}

#endif // PICKUPS_H
