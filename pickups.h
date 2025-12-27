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
#define PICKUP_KEY_BLUE 10       // Blue Key
#define NUM_PICKUP_TYPES 11

#define MAX_PICKUPS 128           // Increased from 64 to support large imported maps
#define PICKUP_RADIUS 30         // Collection radius (Increased from 20)
#define PICKUP_BOB_SPEED 4       // Floating animation speed
#define PICKUP_BOB_HEIGHT 5      // Floating animation height
#define PICKUP_RESPAWN_TIME 30000 // 30 seconds respawn

// Powerup durations (ms)
#define BERSERK_DURATION 30000
#define INVULN_DURATION 15000
#define SPEED_DURATION 20000

// Pickup sprite definition
typedef struct {
    int animated;
    int frameCount;
    int frameTime; // ms per frame
    const unsigned char** frames;   // Array of pointers to frame data
    const int* widths;              // Array of widths
    const int* heights;             // Array of heights
    const unsigned char* staticTexture; // Fallback or static texture
    int staticWidth;
    int staticHeight;
} PickupDef;

PickupDef pickupDefs[NUM_PICKUP_TYPES];

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

// Initialize pickup definitions (map types to textures)
void initPickupDefs() {
    // defaults
    for(int i=0; i<NUM_PICKUP_TYPES; i++) {
        pickupDefs[i].animated = 0;
        pickupDefs[i].staticTexture = NULL;
    }

    // Health Large (Animated)
    pickupDefs[PICKUP_HEALTH_LARGE].animated = 1;
    pickupDefs[PICKUP_HEALTH_LARGE].frameCount = HEALTH_FRAME_COUNT;
    pickupDefs[PICKUP_HEALTH_LARGE].frameTime = HEALTH_FRAME_MS;
    pickupDefs[PICKUP_HEALTH_LARGE].frames = (const unsigned char**)HEALTH_frames;
    pickupDefs[PICKUP_HEALTH_LARGE].widths = HEALTH_frame_widths;
    pickupDefs[PICKUP_HEALTH_LARGE].heights = HEALTH_frame_heights;

    // Armor Large (Animated)
    pickupDefs[PICKUP_ARMOR_LARGE].animated = 1;
    pickupDefs[PICKUP_ARMOR_LARGE].frameCount = ARMOUR_FRAME_COUNT;
    pickupDefs[PICKUP_ARMOR_LARGE].frameTime = ARMOUR_FRAME_MS;
    pickupDefs[PICKUP_ARMOR_LARGE].frames = (const unsigned char**)ARMOUR_frames;
    pickupDefs[PICKUP_ARMOR_LARGE].widths = ARMOUR_frame_widths;
    pickupDefs[PICKUP_ARMOR_LARGE].heights = ARMOUR_frame_heights;

    // Blue Key (Animated)
    pickupDefs[PICKUP_KEY_BLUE].animated = 1;
    pickupDefs[PICKUP_KEY_BLUE].frameCount = BL_KEY_FRAME_COUNT;
    pickupDefs[PICKUP_KEY_BLUE].frameTime = BL_KEY_FRAME_MS;
    pickupDefs[PICKUP_KEY_BLUE].frames = (const unsigned char**)BL_KEY_frames;
    pickupDefs[PICKUP_KEY_BLUE].widths = BL_KEY_frame_widths;
    pickupDefs[PICKUP_KEY_BLUE].heights = BL_KEY_frame_heights;

    // For others, we might want fallbacks or reusable sprites
    // (In a full implementation, we'd map small health, ammo etc too)
    // For now, mapping Small Health to Large Health sprite as placeholder if needed, 
    // or just leave them blank (will be invisible? or need fallback rendering?)
    // Let's use Large Health sprite for Small Health for now but maybe scale it?
    // Sprite scaling happens in draw routine.
    pickupDefs[PICKUP_HEALTH_SMALL] = pickupDefs[PICKUP_HEALTH_LARGE];
    pickupDefs[PICKUP_ARMOR_SMALL] = pickupDefs[PICKUP_ARMOR_LARGE];
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

    initPickupDefs();
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
    extern int hasBlueKey; // Need to ensure this exists in DoomTest.c or similar
    
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
        case PICKUP_KEY_BLUE:
            return !hasBlueKey; // Only pick up if we don't have it
        default:
            return 0;
    }
}

// Apply pickup effect
void applyPickup(int type, int currentTime) {
    extern int playerHealth, playerMaxHealth;
    extern int playerArmor, playerMaxArmor;
    extern int hasBlueKey;
    extern void healPlayer(int amount);
    extern void addArmor(int amount);
    extern void addAmmo(int weaponType, int amount);
    extern void triggerFlash(int r, int g, int b, int currentTime);
    extern void showMessage(const char* msg, int duration); // Assuming this might exist or we just use flash

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
        case PICKUP_KEY_BLUE:
            hasBlueKey = 1;
            triggerFlash(0, 0, 255, currentTime);
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
        int distSq = dx * dx + dy * dy;
        
        // Use cylindrical collision (2D radius + height check)
        // This fixes issue where player eye height makes floor items unpickable
        if (distSq < PICKUP_RADIUS * PICKUP_RADIUS && abs(dz) < 40) {
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

// Draw pickups as sprites
void drawPickups(void (*pixelFunc)(int, int, int, int, int),
                 int screenWidth, int screenHeight,
                 int playerX, int playerY, int playerZ, int playerAngle,
                 float* cosTable, float* sinTable,
                 float* depthBuffer, int currentTime) {
    float CS = cosTable[playerAngle];
    float SN = sinTable[playerAngle];
    
    // Simple sort (Painter's algorithm not strictly needed with depth buffer but good for transparency)
    int sortedIndices[MAX_PICKUPS];
    float distances[MAX_PICKUPS];
    int count = 0;

    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!pickups[i].active || pickups[i].collected) continue;
        
        float dx = (float)(pickups[i].x - playerX);
        float dy = (float)(pickups[i].y - playerY);
        distances[count] = dx * dx + dy * dy;
        sortedIndices[count] = i;
        count++;
    }

    // Sort furthest to closest
    for(int i=0; i<count-1; i++) {
        for(int j=0; j<count-i-1; j++) {
            if(distances[j] < distances[j+1]) {
                float tmpD = distances[j]; distances[j] = distances[j+1]; distances[j+1] = tmpD;
                int tmpI = sortedIndices[j]; sortedIndices[j] = sortedIndices[j+1]; sortedIndices[j+1] = tmpI;
            }
        }
    }

    for (int k = 0; k < count; k++) {
        int i = sortedIndices[k];
        Pickup* p = &pickups[i];
        PickupDef* def = &pickupDefs[p->type];

        // Determine sprite frame
        const unsigned char* spriteData = NULL;
        int sw = 8, sh = 8;

        if (def->animated && def->frames != NULL) {
            // Calculate frame
            int frame = (currentTime / def->frameTime) % def->frameCount;
            spriteData = def->frames[frame];
            sw = def->widths[frame];
            sh = def->heights[frame];
        } else if (def->staticTexture != NULL) {
            spriteData = def->staticTexture;
            sw = def->staticWidth;
            sh = def->staticHeight;
        } else {
             // Fallback to simple colored diamond if no sprite (or use a placeholder)
             // Skip for now or implement diamond fallback
             int r, g, b;
             // getPickupColor(p->type, &r, &g, &b); // Deprecated
             r=255; g=255; b=0; // Default yellow
             continue; // Skipping invisible pickups
        }

        if(!spriteData) continue;

        // Calculate bobbing
        int phaseInt = (int)p->bobPhase % 360;
        float bobOffset = sinTable[phaseInt] * PICKUP_BOB_HEIGHT;
        
        // Transform to camera space
        float relX = (float)(p->x - playerX);
        float relY = (float)(p->y - playerY);
        float relZ = (float)(p->z - playerZ) + bobOffset - (sh/2); // Center vertically? Or sit on floor?

        float camX = relX * CS - relY * SN;
        float camY = relX * SN + relY * CS;
        
        if (camY < 1.0f) continue;  // Behind player
        
        // Perspective projection
        int screenX = (int)(camX * 200.0f / camY + screenWidth / 2);
        int screenY = (int)((relZ - 5) * 200.0f / camY + screenHeight / 2); // Adjusted height
        
        // Calculate sprite dimensions on screen
        // FIXED: Double the size
        float scaleMultiplier = 2.0f;
        int spriteHeight = (int)(sh * scaleMultiplier * 200.0f / camY);
        int spriteWidth = (int)(sw * scaleMultiplier * 200.0f / camY);
        
         // Ensure minimum size
        if (spriteWidth < 1) spriteWidth = 1;
        if (spriteHeight < 1) spriteHeight = 1;

        int startX = screenX - spriteWidth / 2;
        int endX = screenX + spriteWidth / 2;
        int startY = screenY - spriteHeight / 2;
        int endY = screenY + spriteHeight / 2;

        // Check bounds
        if (startX >= screenWidth || endX < 0 || startY >= screenHeight || endY < 0) continue;

        // Draw loop
        for(int y = startY; y < endY; y++) {
             if(y < 0 || y >= screenHeight) continue;
             // Texture V
             float v = (float)(y - startY) / (float)spriteHeight;
             
             // FIXED: Flip texture vertically (upside down fix)
             // int texY = (int)(v * (sh)); // Old
             int texY = (sh - 1) - (int)(v * sh);
             
             if (texY < 0) texY = 0;
             if (texY >= sh) texY = sh - 1;

             for(int x = startX; x < endX; x++) {
                 if(x < 0 || x >= screenWidth) continue;
                 
                 // Depth test
                 if (camY > depthBuffer[x]) continue;

                 // Texture U
                 float u = (float)(x - startX) / (float)spriteWidth;
                 int texX = (int)(u * (sw));
                 if (texX >= sw) texX = sw - 1;

                 int pixelIdx = (texY * sw + texX) * 3;
                 int r = spriteData[pixelIdx];
                 int g = spriteData[pixelIdx+1];
                 int b = spriteData[pixelIdx+2];

                 // Chroma key
                 // FIXED: Treat (0,0,0) AND (1,0,0) as transparent
                 if ((r == 0 && g == 0 && b == 0) || (r == 1 && g == 0 && b == 0)) continue;

                 // Draw
                 pixelFunc(x, y, r, g, b);
             }
        }

    }
}

// Debug drawing: Draw pickup hitboxes in 3D view
void drawPickupDebugOverlay(void (*pixelFunc)(int, int, int, int, int), 
                           int screenWidth, int screenHeight,
                           int playerX, int playerY, int playerZ, int playerAngle,
                           float* cosTable, float* sinTable,
                           float* depthBuf) {
    float CS = cosTable[playerAngle];
    float SN = sinTable[playerAngle];
    
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!pickups[i].active || pickups[i].collected) continue;
        
        // Transform pickup position to camera space
        int relX = pickups[i].x - playerX;
        int relY = pickups[i].y - playerY;
        
        float camX = relX * CS - relY * SN;
        float camY = relX * SN + relY * CS;
        
        // Calculate collision radius in screen space
        int screenRadius = (int)(PICKUP_RADIUS * 200.0f / camY);
        if (screenRadius < 1) screenRadius = 1;
        
        if (camY < 1.0f) continue; // Behind player
        
        int screenX = (int)(camX * 200.0f / camY + screenWidth / 2);
        
        // Check if visible
        if (screenX + screenRadius < 0 || screenX - screenRadius >= screenWidth) continue;
        
        // Draw circle at floor level (approximate)
        int floorZ = pickups[i].z - playerZ;
        int screenY = (int)(floorZ * 200.0f / camY + screenHeight / 2);
        
        // Draw collision circle (simple 8 points or more)
        for (int a = 0; a < 360; a += 30) {
            float rad = a * 3.14159f / 180.0f;
            int px = screenX + (int)(cos(rad) * screenRadius);
            int py = screenY + (int)(sin(rad) * screenRadius * 0.3f); // Flattened circle for perspective
            
            if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight) {
                pixelFunc(px, py, 0, 255, 0); // Green outline
            }
        }
        
        // Draw vertical centerline
        int topZ = floorZ - 40; // Height of hitbox approx
        int screenYTop = (int)(topZ * 200.0f / camY + screenHeight / 2);
        
        for (int y = screenYTop; y <= screenY; y++) {
             if (screenX >= 0 && screenX < screenWidth && y >= 0 && y < screenHeight) {
                 pixelFunc(screenX, y, 0, 255, 0);
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
