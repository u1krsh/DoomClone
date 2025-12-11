#ifndef WEAPON_H
#define WEAPON_H

// Weapon Module
// Provides weapon handling, shooting, and weapon switching

#include <math.h>
#include "textures/pistol_stat.h"
#include "textures/pistol_shooty.h"
#include "textures/pistol_flash.h"

// Weapon types
#define WEAPON_FIST 0
#define WEAPON_PISTOL 1
#define WEAPON_SHOTGUN 2
#define WEAPON_CHAINGUN 3
#define NUM_WEAPONS 4

// Weapon properties
#define PISTOL_DAMAGE 15
#define SHOTGUN_DAMAGE 60   // Total damage (pellets)
#define CHAINGUN_DAMAGE 10
#define FIST_DAMAGE 10

#define PISTOL_COOLDOWN 300     // ms between shots
#define SHOTGUN_COOLDOWN 700
#define CHAINGUN_COOLDOWN 80
#define FIST_COOLDOWN 400

#define PISTOL_RANGE 800
#define SHOTGUN_RANGE 400
#define CHAINGUN_RANGE 600
#define FIST_RANGE 50

// Shooting animation timing
#define PISTOL_SHOOT_ANIM_DURATION 200  // Total animation duration in ms
#define PISTOL_SHOOT_FRAME_DURATION 66  // Duration per frame in ms

// Weapon state
typedef struct {
    int currentWeapon;      // Currently selected weapon
    int ammo[NUM_WEAPONS];  // Ammo for each weapon
    int maxAmmo[NUM_WEAPONS]; // Max ammo capacity
    int lastFireTime;       // Last time weapon was fired
    int isFiring;           // Is the fire button held?
    int weaponBobPhase;     // For weapon bobbing animation
    int muzzleFlashTime;    // Time of last muzzle flash
} WeaponState;

// Global weapon state
WeaponState weapon;

// Expose current weapon for external access
int weapon_currentWeapon = WEAPON_PISTOL;

// Initialize weapon system
void initWeapons(void) {
    weapon.currentWeapon = WEAPON_PISTOL;
    weapon_currentWeapon = WEAPON_PISTOL;
    
    // Fist has unlimited ammo
    weapon.ammo[WEAPON_FIST] = -1;  // -1 = infinite
    weapon.maxAmmo[WEAPON_FIST] = -1;
    
    // Pistol
    weapon.ammo[WEAPON_PISTOL] = 50;
    weapon.maxAmmo[WEAPON_PISTOL] = 200;
    
    // Shotgun
    weapon.ammo[WEAPON_SHOTGUN] = 0;
    weapon.maxAmmo[WEAPON_SHOTGUN] = 50;
    
    // Chaingun
    weapon.ammo[WEAPON_CHAINGUN] = 0;
    weapon.maxAmmo[WEAPON_CHAINGUN] = 400;
    
    weapon.lastFireTime = 0;
    weapon.isFiring = 0;
    weapon.weaponBobPhase = 0;
    weapon.muzzleFlashTime = 0;
}

// Get weapon cooldown
int getWeaponCooldown(int weaponType) {
    switch (weaponType) {
        case WEAPON_FIST: return FIST_COOLDOWN;
        case WEAPON_PISTOL: return PISTOL_COOLDOWN;
        case WEAPON_SHOTGUN: return SHOTGUN_COOLDOWN;
        case WEAPON_CHAINGUN: return CHAINGUN_COOLDOWN;
        default: return PISTOL_COOLDOWN;
    }
}

// Get weapon damage
int getWeaponDamage(int weaponType) {
    switch (weaponType) {
        case WEAPON_FIST: return FIST_DAMAGE;
        case WEAPON_PISTOL: return PISTOL_DAMAGE;
        case WEAPON_SHOTGUN: return SHOTGUN_DAMAGE;
        case WEAPON_CHAINGUN: return CHAINGUN_DAMAGE;
        default: return PISTOL_DAMAGE;
    }
}

// Get weapon range
int getWeaponRange(int weaponType) {
    switch (weaponType) {
        case WEAPON_FIST: return FIST_RANGE;
        case WEAPON_PISTOL: return PISTOL_RANGE;
        case WEAPON_SHOTGUN: return SHOTGUN_RANGE;
        case WEAPON_CHAINGUN: return CHAINGUN_RANGE;
        default: return PISTOL_RANGE;
    }
}

// Get weapon name
const char* getWeaponName(int weaponType) {
    switch (weaponType) {
        case WEAPON_FIST: return "FIST";
        case WEAPON_PISTOL: return "PISTOL";
        case WEAPON_SHOTGUN: return "SHOTGUN";
        case WEAPON_CHAINGUN: return "CHAINGUN";
        default: return "UNKNOWN";
    }
}

// Check if weapon can fire
int canFire(int currentTime) {
    // Check cooldown
    int cooldown = getWeaponCooldown(weapon.currentWeapon);
    if (currentTime - weapon.lastFireTime < cooldown) {
        return 0;
    }
    
    // Check ammo
    if (weapon.ammo[weapon.currentWeapon] == 0) {
        return 0;
    }
    
    return 1;
}

// Fire weapon - returns 1 if fired, 0 if not
// enemyIndex is the enemy in crosshair (-1 if none)
int fireWeapon(int enemyIndex, int currentTime) {
    if (!canFire(currentTime)) return 0;
    
    // Use ammo
    if (weapon.ammo[weapon.currentWeapon] > 0) {
        weapon.ammo[weapon.currentWeapon]--;
    }
    
    weapon.lastFireTime = currentTime;
    weapon.muzzleFlashTime = currentTime;
    
    // Apply damage to enemy if one is targeted
    if (enemyIndex >= 0) {
        int damage = getWeaponDamage(weapon.currentWeapon);
        
        // Shotgun has spread - random damage reduction at distance
        if (weapon.currentWeapon == WEAPON_SHOTGUN) {
            damage = damage * (70 + (rand() % 30)) / 100;
        }
        
        // Call external damage function
        extern void damageEnemy(int enemyIndex, int damage, int currentTime);
        damageEnemy(enemyIndex, damage, currentTime);
        
        return 1;
    }
    
    return 1;  // Fired but missed
}

// Switch to next weapon
void nextWeapon(void) {
    int startWeapon = weapon.currentWeapon;
    do {
        weapon.currentWeapon = (weapon.currentWeapon + 1) % NUM_WEAPONS;
        // Skip weapons with no ammo (except fist)
        if (weapon.ammo[weapon.currentWeapon] != 0 || weapon.currentWeapon == WEAPON_FIST) {
            break;
        }
    } while (weapon.currentWeapon != startWeapon);
}

// Switch to previous weapon
void prevWeapon(void) {
    int startWeapon = weapon.currentWeapon;
    do {
        weapon.currentWeapon--;
        if (weapon.currentWeapon < 0) weapon.currentWeapon = NUM_WEAPONS - 1;
        // Skip weapons with no ammo (except fist)
        if (weapon.ammo[weapon.currentWeapon] != 0 || weapon.currentWeapon == WEAPON_FIST) {
            break;
        }
    } while (weapon.currentWeapon != startWeapon);
}

// Select specific weapon
void selectWeapon(int weaponType) {
    if (weaponType < 0 || weaponType >= NUM_WEAPONS) return;
    
    // Can always select fist, or any weapon with ammo
    if (weaponType == WEAPON_FIST || weapon.ammo[weaponType] != 0) {
        weapon.currentWeapon = weaponType;
        weapon_currentWeapon = weaponType;  // Update external variable
    }
}

// Add ammo
void addAmmo(int weaponType, int amount) {
    if (weaponType < 0 || weaponType >= NUM_WEAPONS) return;
    if (weapon.maxAmmo[weaponType] < 0) return;  // Infinite ammo weapon
    
    weapon.ammo[weaponType] += amount;
    if (weapon.ammo[weaponType] > weapon.maxAmmo[weaponType]) {
        weapon.ammo[weaponType] = weapon.maxAmmo[weaponType];
    }
}

// Give all weapons and ammo
void giveAllWeapons(void) {
    for (int i = 0; i < NUM_WEAPONS; i++) {
        if (weapon.maxAmmo[i] > 0) {
            weapon.ammo[i] = weapon.maxAmmo[i];
        }
    }
}

// Update weapon state (call each frame)
void updateWeapon(int isMoving, int currentTime) {
    // Update weapon bobbing when moving
    if (isMoving) {
        weapon.weaponBobPhase += 8;
        if (weapon.weaponBobPhase >= 360) weapon.weaponBobPhase -= 360;
    }
    
    // Auto-fire for chaingun
    if (weapon.isFiring && weapon.currentWeapon == WEAPON_CHAINGUN) {
        // Get enemy in crosshair
        extern int getEnemyInCrosshair(int, int, int, float*, float*);
        extern float M_cos[], M_sin[];
        extern int P_x, P_y, P_a;
        // Note: These would need to be passed in properly in a real implementation
    }
}

// Draw weapon HUD (ammo display)
void drawWeaponHUD(void (*pixelFunc)(int, int, int, int, int),
                   int screenWidth, int screenHeight) {
    // Import string drawing function
    extern void drawString(int, int, const char*, int, int, int, void (*)(int, int, int, int, int));
    
    char ammoText[32];
    int margin = 10;
    
    // Draw weapon name (bottom right)
    const char* weaponName = getWeaponName(weapon.currentWeapon);
    int nameWidth = 0;
    const char* p = weaponName;
    while (*p++) nameWidth += 8;
    
    int nameX = screenWidth - nameWidth - margin;
    int nameY = margin + 12;
    drawString(nameX, nameY, weaponName, 255, 200, 100, pixelFunc);
    
    // Draw ammo count
    if (weapon.ammo[weapon.currentWeapon] >= 0) {
        snprintf(ammoText, sizeof(ammoText), "%d", weapon.ammo[weapon.currentWeapon]);
    } else {
        snprintf(ammoText, sizeof(ammoText), "INF");
    }
    
    int ammoWidth = 0;
    p = ammoText;
    while (*p++) ammoWidth += 8;
    
    int ammoX = screenWidth - ammoWidth - margin;
    int ammoY = margin;
    
    // Color based on ammo level
    int ammoR = 255, ammoG = 255, ammoB = 255;
    if (weapon.ammo[weapon.currentWeapon] >= 0) {
        float ammoPercent = (float)weapon.ammo[weapon.currentWeapon] / 
                           (float)weapon.maxAmmo[weapon.currentWeapon];
        if (ammoPercent < 0.2f) {
            ammoR = 255; ammoG = 0; ammoB = 0;  // Red when low
        } else if (ammoPercent < 0.5f) {
            ammoR = 255; ammoG = 255; ammoB = 0;  // Yellow when medium
        }
    }
    
    drawString(ammoX, ammoY, ammoText, ammoR, ammoG, ammoB, pixelFunc);
}

// Draw muzzle flash effect - DISABLED (using shoot animation instead)
void drawMuzzleFlash(void (*pixelFunc)(int, int, int, int, int),
                     int screenWidth, int screenHeight,
                     int currentTime) {
    // Muzzle flash disabled - using pistol shoot animation frames instead
    (void)pixelFunc;
    (void)screenWidth;
    (void)screenHeight;
    (void)currentTime;
}

// Draw crosshair - DISABLED (we now show weapon sprite instead)
void drawCrosshair(void (*pixelFunc)(int, int, int, int, int),
                   int screenWidth, int screenHeight,
                   int targetingEnemy) {
    // Crosshair disabled - weapon sprite is shown instead
    (void)pixelFunc;
    (void)screenWidth;
    (void)screenHeight;
    (void)targetingEnemy;
}

// Draw weapon sprite at bottom center of screen
void drawWeaponSprite(void (*pixelFunc)(int, int, int, int, int),
                      int screenWidth, int screenHeight,
                      int currentTime) {
    // Currently only pistol sprite is available
    if (weapon.currentWeapon != WEAPON_PISTOL) {
        // Draw placeholder for other weapons (just return for now)
        return;
    }
    
    // Determine which sprite to use (idle or shooting animation)
    const unsigned char* spriteData = PISTOL_STAT;
    int spriteWidth = PISTOL_STAT_WIDTH;
    int spriteHeight = PISTOL_STAT_HEIGHT;
    int showFlash = 0;  // Flag to show muzzle flash overlay
    
    // Check if we should show shooting animation
    int timeSinceShot = currentTime - weapon.muzzleFlashTime;
    if (timeSinceShot < PISTOL_SHOOT_ANIM_DURATION && timeSinceShot >= 0) {
        // Calculate which frame to show
        int frame = timeSinceShot / PISTOL_SHOOT_FRAME_DURATION;
        if (frame >= PISTOL_SHOOTY_FRAME_COUNT) frame = PISTOL_SHOOTY_FRAME_COUNT - 1;
        
        // Select the appropriate frame
        spriteWidth = PISTOL_SHOOTY_frame_widths[frame];
        spriteHeight = PISTOL_SHOOTY_frame_heights[frame];
        
        switch (frame) {
            case 0:
                spriteData = PISTOL_SHOOTY_frame_0;
                showFlash = 1;  // Show flash on first frame
                break;
            case 1:
                spriteData = PISTOL_SHOOTY_frame_1;
                break;
            case 2:
                spriteData = PISTOL_SHOOTY_frame_2;
                break;
            case 3:
            default:
                spriteData = PISTOL_SHOOTY_frame_3;
                break;
        }
    }
    
    // No scaling - draw at original size
    int scale = 1;
    int scaledWidth = spriteWidth * scale;
    int scaledHeight = spriteHeight * scale;
    
    // Position at bottom center - offset down so bottom part is cut off
    int startX = (screenWidth - scaledWidth) / 2;
    int startY = -20;  // Move down by 20 pixels so bottom is cut off
    
    // Apply weapon bob with more intensity (only when not shooting)
    float bobOffsetX = 0;
    float bobOffsetY = 0;
    if (weapon.weaponBobPhase > 0 && timeSinceShot >= PISTOL_SHOOT_ANIM_DURATION) {
        // Horizontal bob (side to side)
        bobOffsetX = sin(weapon.weaponBobPhase * 3.14159f / 180.0f) * 4.0f;
        // Vertical bob (up and down, double frequency)
        bobOffsetY = sin(weapon.weaponBobPhase * 2.0f * 3.14159f / 180.0f) * 3.0f;
    }
    startX += (int)bobOffsetX;
    startY += (int)bobOffsetY;
    
    // Draw the pistol sprite
    for (int y = 0; y < spriteHeight; y++) {
        for (int x = 0; x < spriteWidth; x++) {
            // Get pixel from texture (flipped vertically)
            int srcY = spriteHeight - 1 - y;
            int pixelIndex = (srcY * spriteWidth + x) * 3;
            
            int r = spriteData[pixelIndex + 0];
            int g = spriteData[pixelIndex + 1];
            int b = spriteData[pixelIndex + 2];
            
            // Skip transparent pixels (1, 0, 0 is transparent)
            if (r == 1 && g == 0 && b == 0) continue;
            
            // Draw pixel (no scaling)
            int drawX = startX + x;
            int drawY = startY + y;
            
            // Bounds check
            if (drawX >= 0 && drawX < screenWidth && 
                drawY >= 0 && drawY < screenHeight) {
                pixelFunc(drawX, drawY, r, g, b);
            }
        }
    }
    
    // Draw muzzle flash overlay on top of the gun (first frame only)
    if (showFlash) {
        // Position flash above the gun barrel (shifted right to align with barrel)
        int flashX = (screenWidth - PISTOL_FLASH_WIDTH) / 2 + 18;  // Shifted right
        int flashY = startY + spriteHeight - 19;  // Position above gun barrel
        
        for (int y = 0; y < PISTOL_FLASH_HEIGHT; y++) {
            for (int x = 0; x < PISTOL_FLASH_WIDTH; x++) {
                // Get pixel from flash texture (flipped vertically)
                int srcY = PISTOL_FLASH_HEIGHT - 1 - y;
                int pixelIndex = (srcY * PISTOL_FLASH_WIDTH + x) * 3;
                
                int r = PISTOL_FLASH[pixelIndex + 0];
                int g = PISTOL_FLASH[pixelIndex + 1];
                int b = PISTOL_FLASH[pixelIndex + 2];
                
                // Skip transparent pixels (1, 0, 0 is transparent)
                if (r == 1 && g == 0 && b == 0) continue;
                
                // Draw pixel
                int drawX = flashX + x;
                int drawY = flashY + y;
                
                // Bounds check
                if (drawX >= 0 && drawX < screenWidth && 
                    drawY >= 0 && drawY < screenHeight) {
                    pixelFunc(drawX, drawY, r, g, b);
                }
            }
        }
    }
}

#endif // WEAPON_H
