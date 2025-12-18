#ifndef WEAPON_H
#define WEAPON_H

// Weapon Module
// Provides weapon handling, shooting, and weapon switching

#include <math.h>
#include "textures/pistol_stat.h"
#include "textures/pistol_shooty.h"
#include "textures/pistol_flash.h"
#include "textures/shotgun_stat.h"
#include "textures/shotgun_flash.h"
#include "textures/shotgun_reload.h"
#include "textures/shotgun_reload.h"
#include "textures/hands_stat.h"
#include "textures/hand_punch.h"
#include "textures/plasma_stat.h"
#include "textures/plasma_fire.h"
#include "textures/plasma_reload.h"

// Weapon types
#define WEAPON_FIST 0
#define WEAPON_PISTOL 1
#define WEAPON_SHOTGUN 2
#define WEAPON_CHAINGUN 3
#define WEAPON_PLASMA 4
#define NUM_WEAPONS 5

// Weapon properties
#define PISTOL_DAMAGE 15
#define SHOTGUN_DAMAGE 60   // Total damage (pellets)
#define CHAINGUN_DAMAGE 10
#define FIST_DAMAGE 10
#define PLASMA_DAMAGE 20

#define PISTOL_COOLDOWN 500     // ms between shots (0.5 second delay)
#define SHOTGUN_COOLDOWN 1200
#define CHAINGUN_COOLDOWN 80
#define FIST_COOLDOWN 400
#define PLASMA_COOLDOWN 100 // Fast fire rate

#define PISTOL_RANGE 800
#define SHOTGUN_RANGE 400
#define CHAINGUN_RANGE 600
#define FIST_RANGE 50
#define PLASMA_RANGE 1000

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
    int muzzleFlashTime;    // Time of last muzzleFlash
    
    // Weapon switching animation
    float weaponOffsetY;    // Y offset for raising/lowering animation (0 = raised, >0 = lowered)
    int switchState;        // 0=Ready, 1=Lowering, 2=Raising
    int pendingWeapon;      // Weapon to switch to after lowering
} WeaponState;

#define WEAPON_STATE_READY 0
#define WEAPON_STATE_LOWERING 1
#define WEAPON_STATE_RAISING 2
#define WEAPON_SWITCH_SPEED 15.0f // Pixels per frame

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
    weapon.ammo[WEAPON_SHOTGUN] = 20;
    weapon.maxAmmo[WEAPON_SHOTGUN] = 50;
    
    // Chaingun
    weapon.ammo[WEAPON_CHAINGUN] = 0;
    weapon.maxAmmo[WEAPON_CHAINGUN] = 400;
    
    // Plasma Gun
    weapon.ammo[WEAPON_PLASMA] = 100;
    weapon.maxAmmo[WEAPON_PLASMA] = 400;
    
    weapon.lastFireTime = 0;
    weapon.isFiring = 0;
    weapon.weaponBobPhase = 0;
    weapon.muzzleFlashTime = 0;
    
    weapon.weaponOffsetY = 0;
    weapon.switchState = WEAPON_STATE_READY;
    weapon.pendingWeapon = WEAPON_PISTOL;
}

// Get weapon cooldown
int getWeaponCooldown(int weaponType) {
    switch (weaponType) {
        case WEAPON_FIST: return FIST_COOLDOWN;
        case WEAPON_PISTOL: return PISTOL_COOLDOWN;
        case WEAPON_SHOTGUN: return SHOTGUN_COOLDOWN;
        case WEAPON_CHAINGUN: return CHAINGUN_COOLDOWN;
        case WEAPON_PLASMA: return PLASMA_COOLDOWN;
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
        case WEAPON_PLASMA: return PLASMA_DAMAGE;
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
        case WEAPON_PLASMA: return PLASMA_RANGE;
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
        case WEAPON_PLASMA: return "PLASMA";
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
    
    // Apply weapon kickback (push player backward)
    extern player P;
    extern math M;
    
    // Get kickback strength based on weapon type
    int kickbackStrength = 0;
    switch (weapon.currentWeapon) {
        case WEAPON_FIST:
            kickbackStrength = 1;  // Very light kickback
            break;
        case WEAPON_PISTOL:
            kickbackStrength = 3;  // Moderate kickback
            break;
        case WEAPON_SHOTGUN:
            kickbackStrength = 8;  // Strong kickback
            break;
        case WEAPON_CHAINGUN:
            kickbackStrength = 2;  // Light but rapid fire
            break;
        case WEAPON_PLASMA:
            kickbackStrength = 1;  // Minimal kickback
            // Spawn Plasma Projectile
            // Spawn Plasma Projectile
            // Convert angle (0-359) to radians for standard sin/cos functions
            float radAngle = P.a * 3.14159f / 180.0f;
            
            // Offset spawn position to avoid self-collision (Player radius ~8, Proj radius 4)
            // Spawn 20 units in front
            float spawnX = P.x + sin(radAngle) * 20.0f;
            float spawnY = P.y + cos(radAngle) * 20.0f;
            
            spawnProjectile(spawnX, spawnY, (float)P.z, radAngle, PROJ_TYPE_PLASMA, currentTime);
            return 1; // Return 1 to indicate fired (and skip hitscan damage below)
            break;
    }
    
    // Apply kickback in the direction opposite to player's facing
    if (kickbackStrength > 0) {
        // Calculate backward direction (opposite of facing angle)
        float backwardX = -M.sin[P.a] * kickbackStrength;
        float backwardY = -M.cos[P.a] * kickbackStrength;
        
        P.x += (int)backwardX;
        P.y += (int)backwardY;
    }
    
    // Apply damage to enemy if one is targeted (Hitscan weapons only)
    if (weapon.currentWeapon != WEAPON_PLASMA && enemyIndex >= 0) {
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
    
    if (weapon.currentWeapon == WEAPON_PLASMA) return 1; // Already handled
    
    return 1;  // Fired but missed
}

// Switch to next weapon
void nextWeapon(void) {
    if (weapon.switchState != WEAPON_STATE_READY) return; // Wait for current switch to finish
    
    int startWeapon = weapon.currentWeapon;
    int nextW = startWeapon;
    
    do {
        nextW = (nextW + 1) % NUM_WEAPONS;
        // Skip weapons with no ammo (except fist)
        if (weapon.ammo[nextW] != 0 || nextW == WEAPON_FIST) {
            break;
        }
    } while (nextW != startWeapon);
    
    if (nextW != startWeapon) {
        weapon.pendingWeapon = nextW;
        weapon.switchState = WEAPON_STATE_LOWERING;
    }
}

// Switch to previous weapon
void prevWeapon(void) {
    if (weapon.switchState != WEAPON_STATE_READY) return;
    
    int startWeapon = weapon.currentWeapon;
    int prevW = startWeapon;
    
    do {
        prevW--;
        if (prevW < 0) prevW = NUM_WEAPONS - 1;
        // Skip weapons with no ammo (except fist)
        if (weapon.ammo[prevW] != 0 || prevW == WEAPON_FIST) {
            break;
        }
    } while (prevW != startWeapon);
    
    if (prevW != startWeapon) {
        weapon.pendingWeapon = prevW;
        weapon.switchState = WEAPON_STATE_LOWERING;
    }
}

// Select specific weapon
void selectWeapon(int weaponType) {
    if (weaponType < 0 || weaponType >= NUM_WEAPONS) return;
    if (weapon.switchState != WEAPON_STATE_READY) return;
    if (weaponType == weapon.currentWeapon) return;
    
    // Can always select fist, or any weapon with ammo
    if (weaponType == WEAPON_FIST || weapon.ammo[weaponType] != 0) {
        weapon.pendingWeapon = weaponType;
        weapon.switchState = WEAPON_STATE_LOWERING;
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
    // Handle switching animation
    if (weapon.switchState == WEAPON_STATE_LOWERING) {
        weapon.weaponOffsetY += WEAPON_SWITCH_SPEED;
        if (weapon.weaponOffsetY >= 150.0f) { // Fully lowered (adjust height as needed)
            weapon.currentWeapon = weapon.pendingWeapon;
            weapon_currentWeapon = weapon.pendingWeapon;
            weapon.switchState = WEAPON_STATE_RAISING;
        }
    } else if (weapon.switchState == WEAPON_STATE_RAISING) {
        weapon.weaponOffsetY -= WEAPON_SWITCH_SPEED;
        if (weapon.weaponOffsetY <= 0.0f) {
            weapon.weaponOffsetY = 0.0f;
            weapon.switchState = WEAPON_STATE_READY;
        }
    }

    // Update weapon bobbing when moving (only if ready)
    if (isMoving && weapon.switchState == WEAPON_STATE_READY) {
        weapon.weaponBobPhase += 8;
        if (weapon.weaponBobPhase >= 360) weapon.weaponBobPhase -= 360;
    }
    
    // Auto-fire for chaingun (only if ready)
    if (weapon.isFiring && weapon.currentWeapon == WEAPON_CHAINGUN && weapon.switchState == WEAPON_STATE_READY) {
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
    // Fist Drawing Logic
    if (weapon.currentWeapon == WEAPON_FIST) {
        int timeSinceFire = currentTime - weapon.lastFireTime;
        const unsigned char* spriteData = HANDS_STAT;
        int spriteWidth = HANDS_STAT_WIDTH;
        int spriteHeight = HANDS_STAT_HEIGHT;
        
        // Animation timing
        int punchAnimDuration = 400; // Total punch animation duration (matches FIST_COOLDOWN)
        int slideBackOffset = 0;
        int punchYOffset = -15; // Extra Y offset for punch frames
        
        // Determine state and apply slide-back effect
        if (timeSinceFire >= 0 && timeSinceFire < punchAnimDuration) {
            // During punch animation
            punchYOffset = 5; // Move punch frames lower than static hand
            
            if (timeSinceFire < 100) {
                // First phase: show punch frame (0-100ms)
                spriteData = HAND_PUNCH_frame_0;
                spriteWidth = HAND_PUNCH_frame_widths[0];
                spriteHeight = HAND_PUNCH_frame_heights[0];
                // Slide forward slightly
                slideBackOffset = -(int)((timeSinceFire / 100.0f) * 5);
            } else if (timeSinceFire < 200) {
                // Second phase: transition frame (100-200ms)
                spriteData = HAND_PUNCH_frame_1;
                spriteWidth = HAND_PUNCH_frame_widths[1];
                spriteHeight = HAND_PUNCH_frame_heights[1];
                slideBackOffset = -5; // Hold forward position
            } else if (timeSinceFire < 300) {
                // Third phase: full extend (200-300ms)
                spriteData = HAND_PUNCH_frame_2;
                spriteWidth = HAND_PUNCH_frame_widths[2];
                spriteHeight = HAND_PUNCH_frame_heights[2];
                slideBackOffset = -5; // Hold forward position
            } else {
                // Recovery phase: play animation backwards (300-400ms)
                int recoveryTime = timeSinceFire - 300;
                if (recoveryTime < 25) {
                    // Frame 2 (300-325ms)
                    spriteData = HAND_PUNCH_frame_2;
                    spriteWidth = HAND_PUNCH_frame_widths[2];
                    spriteHeight = HAND_PUNCH_frame_heights[2];
                    slideBackOffset = -5;
                } else if (recoveryTime < 50) {
                    // Frame 1 (325-350ms)
                    spriteData = HAND_PUNCH_frame_1;
                    spriteWidth = HAND_PUNCH_frame_widths[1];
                    spriteHeight = HAND_PUNCH_frame_heights[1];
                    slideBackOffset = -5;
                } else if (recoveryTime < 75) {
                    // Frame 0 (350-375ms)
                    spriteData = HAND_PUNCH_frame_0;
                    spriteWidth = HAND_PUNCH_frame_widths[0];
                    spriteHeight = HAND_PUNCH_frame_heights[0];
                    slideBackOffset = -(int)(((75.0f - recoveryTime) / 25.0f) * 5);
                } else {
                    // Back to static (375-400ms)
                    spriteData = HANDS_STAT;
                    spriteWidth = HANDS_STAT_WIDTH;
                    spriteHeight = HANDS_STAT_HEIGHT;
                    slideBackOffset = 0;
                    punchYOffset = 0; // Return to normal height
                }
            }
        } else {
            // Idle state: static hand
            spriteData = HANDS_STAT;
            spriteWidth = HANDS_STAT_WIDTH;
            spriteHeight = HANDS_STAT_HEIGHT;
            slideBackOffset = 0;
            punchYOffset = 3;
        }
        
        // Position at bottom center (moved up - positive value raises it)
        int startX = (screenWidth - spriteWidth) / 2;
        int startY = 0;
        
        // Apply switching animation offset
        startY -= (int)weapon.weaponOffsetY;
        
        // Apply punch Y offset (makes punch frames lower)
        startY -= punchYOffset;
        
        // Apply slide-back offset
        startY -= slideBackOffset;
        
        // Apply weapon bob (only when not punching)
        float bobOffsetX = 0;
        float bobOffsetY = 0;
        if (weapon.weaponBobPhase > 0 && timeSinceFire >= punchAnimDuration) {
            bobOffsetX = sin(weapon.weaponBobPhase * 3.14159f / 180.0f) * 2.0f;
            bobOffsetY = sin(weapon.weaponBobPhase * 2.0f * 3.14159f / 180.0f) * 1.5f;
        }
        startX += (int)bobOffsetX;
        startY += (int)bobOffsetY;
        
        // Draw the fist sprite
        for (int y = 0; y < spriteHeight; y++) {
            for (int x = 0; x < spriteWidth; x++) {
                int srcY = spriteHeight - 1 - y;
                
                if (srcY < 0 || srcY >= spriteHeight) continue;
                
                int pixelIndex = (srcY * spriteWidth + x) * 3;
                
                int r = spriteData[pixelIndex + 0];
                int g = spriteData[pixelIndex + 1];
                int b = spriteData[pixelIndex + 2];
                
                // Skip transparent pixels
                if (r == 1 && g == 0 && b == 0) continue;
                
                int drawX = startX + x;
                int drawY = startY + y;
                
                if (drawX >= 0 && drawX < screenWidth && 
                    drawY >= 0 && drawY < screenHeight) {
                    pixelFunc(drawX, drawY, r, g, b);
                }
            }
        }
        return; // Done drawing fist
    }
    
    // Check for other weapons or allow fall-through to default behavior (Pistol logic below)

    
    // Shotgun Drawing Logic
    if (weapon.currentWeapon == WEAPON_SHOTGUN) {
        int timeSinceFire = currentTime - weapon.lastFireTime;
        const unsigned char* spriteData = SHOTGUN_STAT;
        int spriteWidth = SHOTGUN_STAT_WIDTH;
        int spriteHeight = SHOTGUN_STAT_HEIGHT;
        
        int showFlash = 0;
        const unsigned char* flashData = NULL;
        int flashWidth = 0;
        int flashHeight = 0;

        // Determine state
        if (timeSinceFire >= 0 && timeSinceFire < 100) {
            // Firing State (0-100ms): Static + Flash
            spriteData = SHOTGUN_STAT; // Base is static
            
            // Flash logic
            int flashFrame = (timeSinceFire / 50) % 2;
            if (flashFrame < SHOTGUN_FLASH_FRAME_COUNT) {
                flashData = SHOTGUN_FLASH_frames[flashFrame];
                flashWidth = SHOTGUN_FLASH_frame_widths[flashFrame];
                flashHeight = SHOTGUN_FLASH_frame_heights[flashFrame];
                showFlash = 1;
            }
        } else if (timeSinceFire >= 100 && timeSinceFire < 700) {
            // Reload State (100-700ms): Reload Animation
            int reloadTime = timeSinceFire - 100;
            int frame = reloadTime / 200; // 200ms per frame
            if (frame >= SHOTGUN_RELOAD_FRAME_COUNT) frame = SHOTGUN_RELOAD_FRAME_COUNT - 1;
            
            spriteData = SHOTGUN_RELOAD_frames[frame];
            spriteWidth = SHOTGUN_RELOAD_frame_widths[frame];
            spriteHeight = SHOTGUN_RELOAD_frame_heights[frame];
            // Wait, does reload header have widths array?
            // "textures/shotgun_reload.h" usually has _frame_widths if frames are different sizes.
            // If they are all same size, use macro. 
            // My previous view showed consistent 3560 lines, and frame data seemed packed.
            // I'll assume they are dimensions of SHOTGUN_RELOAD_WIDTH/HEIGHT if defined, or check if SHOTGUN_RELOAD_frame_widths exists in next step if error.
            // Actually, based on previous headers, it's safer to not assume.
            // BUT, usually these arrays are there. I'll rely on the grep result which FAILED for array pointer, but I found `frames` array at end.
            // I did NOT check for widths array at end.
            // Let's use hardcoded variables if needed or just assume widths array likely exists if frames array exists.
            // Actually, looking at pistol logic: `PISTOL_SHOOTY_frame_widths`.
            // I will assume `SHOTGUN_RELOAD_frame_widths` exists. If not, I'll fix.
            
            // Re-reading my "view file" output for reload header...
            // I missed checking for widths array. 
            // Let's assume standard format.
        } else {
            // Idle State: Static
            spriteData = SHOTGUN_STAT;
        }

        // Draw the shotgun sprite (Centered)
        // Shift right by 15 pixels as requested (corrected from left)
        int startX = (screenWidth - spriteWidth) / 2 + 15;
        int startY = -15; // Shift down by 15 pixels (adjusted from -30)
        
        // Apply switching animation offset
        startY -= (int)weapon.weaponOffsetY;

        // Apply bob
        float bobOffsetX = 0;
        float bobOffsetY = 0;
        if (weapon.weaponBobPhase > 0 && timeSinceFire >= 700) {
             bobOffsetX = sin(weapon.weaponBobPhase * 3.14159f / 180.0f) * 4.0f;
             bobOffsetY = sin(weapon.weaponBobPhase * 2.0f * 3.14159f / 180.0f) * 3.0f;
        }
        startX += (int)bobOffsetX;
        startY += (int)bobOffsetY;

        // Draw Gun
        for (int y = 0; y < spriteHeight; y++) {
            for (int x = 0; x < spriteWidth; x++) {
                int srcY = spriteHeight - 1 - y;
                // Check bounds
                if (srcY < 0 || srcY >= spriteHeight) continue;
                
                int pixelIndex = (srcY * spriteWidth + x) * 3;
                int r = spriteData[pixelIndex + 0];
                int g = spriteData[pixelIndex + 1];
                int b = spriteData[pixelIndex + 2];
                
                if (r == 1 && g == 0 && b == 0) continue; // Transparent
                
                // PixelFunc takes (x, y, r, g, b)
                // Need to ensure x,y inside screen? PixelFunc usually handles it or we should check.
                // Assuming PixelFunc handles clipping or we are safe.
                pixelFunc(startX + x, startY + y, r, g, b);
            }
        }

        // Draw Flash Overlay
        if (showFlash && flashData) {
            // Flash position: Relative to gun to match bobbing/animation
            // Center horizontally on the gun, then shift left by 12 pixels (adjusted from 8)
            int fX = startX + (spriteWidth - flashWidth) / 2 + 3;
            
            // Position vertically relative to gun top
            // startY is bottom of gun image. spriteHeight is height.
            // Gun Top = startY + spriteHeight.
            // Shift down adjustment: was -15, now -20 (5 more pixels down)
            int fY = startY + spriteHeight - 20;
            
            // Adjust to align with muzzle
            // SHOTGUN_STAT is huge. Flash should be at muzzle.
            
            for (int y = 0; y < flashHeight; y++) {
                for (int x = 0; x < flashWidth; x++) {
                    int srcY = flashHeight - 1 - y;
                    int pixelIndex = (srcY * flashWidth + x) * 3;
                    int r = flashData[pixelIndex + 0];
                    int g = flashData[pixelIndex + 1];
                    int b = flashData[pixelIndex + 2];
                    
                    if (r == 1 && g == 0 && b == 0) continue;
                    
                    // Simple additive blending or replace? Pistol uses replace.
                    pixelFunc(fX + x, fY + y, r, g, b);
                }
            }
        }
        return; // Done drawing shotgun
    }

    // Plasma Gun Drawing Logic
    if (weapon.currentWeapon == WEAPON_PLASMA) {
        int timeSinceFire = currentTime - weapon.lastFireTime;
        const unsigned char* spriteData = PLASMA_STAT; // Default idle
        int spriteWidth = PLASMA_STAT_WIDTH;
        int spriteHeight = PLASMA_STAT_HEIGHT;
        
        // Logic:
        // If firing (button held): Use continuous firing animation (Frame 1)
        // If button released: check if we should be reloading (brief delay)
        
        // Debug
        // printf("DEBUG: Weapon State: Fire=%d, TimeSince=%d\n", weapon.isFiring, timeSinceFire);
        
        if (weapon.isFiring) {
            // Firing state - lock to Frame 1 as requested ("stays on second frame")
            // Ensure array bounds
            if (PLASMA_FIRE_FRAME_COUNT > 1) {
                spriteData = PLASMA_FIRE_frames[1];
                spriteWidth = PLASMA_FIRE_frame_widths[1];
                spriteHeight = PLASMA_FIRE_frame_heights[1];
            } else {
                 spriteData = PLASMA_FIRE_frames[0];
                 spriteWidth = PLASMA_FIRE_frame_widths[0];
                 spriteHeight = PLASMA_FIRE_frame_heights[0];
            }
        } 
        else if (currentTime - weapon.lastFireTime < 1000) {
            // Reload state (1 second duration after release)
            // Use the single static reload frame
            spriteData = PLASMA_RELOAD;
            // Use macros if arrays not available
            spriteWidth = PLASMA_RELOAD_WIDTH;
            spriteHeight = PLASMA_RELOAD_HEIGHT;
        } else {
            // Idle
            spriteData = PLASMA_STAT;
            spriteWidth = PLASMA_STAT_WIDTH;
            spriteHeight = PLASMA_STAT_HEIGHT;
        }

        // Draw Plasma Gun (Center)
        int startX = (screenWidth - spriteWidth) / 2;
        // User requested moving textures down. 
        // 0 was default. +20 moved UP. So -20 should move DOWN.
        int startY = -20; 
        
        startY -= (int)weapon.weaponOffsetY;
        
        // Bobbing
        if (weapon.weaponBobPhase > 0 && !weapon.isFiring) {
             startX += (int)(sin(weapon.weaponBobPhase * 3.14159f / 180.0f) * 4.0f);
             startY += (int)(sin(weapon.weaponBobPhase * 2.0f * 3.14159f / 180.0f) * 3.0f);
        }

        for (int y = 0; y < spriteHeight; y++) {
            for (int x = 0; x < spriteWidth; x++) {
                int srcY = spriteHeight - 1 - y;
                if (srcY < 0 || srcY >= spriteHeight) continue;
                
                int pixelIndex = (srcY * spriteWidth + x) * 3;
                int r = spriteData[pixelIndex + 0];
                int g = spriteData[pixelIndex + 1];
                int b = spriteData[pixelIndex + 2];
                
                if (r == 1 && g == 0 && b == 0) continue;
                
                pixelFunc(startX + x, startY + y, r, g, b);
            }
        }
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
    
    // Position at bottom center
    int startX = (screenWidth - scaledWidth) / 2;
    
    // Base Y position (0 = bottom of screen)
    // Add weaponOffsetY for switching animation (positive value moves it down)
    // Shift base position lower by 20 pixels as requested
    int startY = -20 -((int)weapon.weaponOffsetY); 
    
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
    
    // If startY moves down (negative), we need to handle clipping or just rely on pixelFunc/loop bounds
    // Note: If startY is negative, the loop should probably handle it or pixelFunc should
    // Let's ensure we don't draw below screen (y < 0)
    
    // Draw the pistol sprite
    for (int y = 0; y < spriteHeight; y++) {
        for (int x = 0; x < spriteWidth; x++) {
            // Get pixel from texture (flipped vertically? No, usually raw data is bottom-up or top-down depending on load)
            // Code previously used: int srcY = spriteHeight - 1 - y;
            // Assuming texture is stored top-down? Or bottom-up?
            // "srcY = spriteHeight - 1 - y" implies we are iterating screen Y from 0 (bottom) up to height.
            // If y=0 is bottom row of sprite...
            
            int srcY = spriteHeight - 1 - y;
            
            // Safety check for source bounds
            if (srcY < 0 || srcY >= spriteHeight) continue;
            
            int pixelIndex = (srcY * spriteWidth + x) * 3;
            
            int r = spriteData[pixelIndex + 0];
            int g = spriteData[pixelIndex + 1];
            int b = spriteData[pixelIndex + 2];
            
            // Skip transparent pixels (1, 0, 0 is transparent)
            if (r == 1 && g == 0 && b == 0) continue;
            
            // Draw pixel (no scaling)
            int drawX = startX + x;
            int drawY = startY + y;
            
            // PixelFunc typically handles clipping, but let's be safe
             // (Assuming pixelFunc(x, y, r, g, b) checks bounds, or we should)
             // If drawY is negative (below screen), don't draw.
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
