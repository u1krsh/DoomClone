#ifndef WEAPON_H
#define WEAPON_H

// Weapon Module
// Provides weapon handling, shooting, and weapon switching

#include <math.h>

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

// Initialize weapon system
void initWeapons(void) {
    weapon.currentWeapon = WEAPON_PISTOL;
    
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

// Draw muzzle flash effect
void drawMuzzleFlash(void (*pixelFunc)(int, int, int, int, int),
                     int screenWidth, int screenHeight,
                     int currentTime) {
    // Check if we should show muzzle flash
    int timeSinceFlash = currentTime - weapon.muzzleFlashTime;
    if (timeSinceFlash > 50) return;  // Flash lasts 50ms
    
    // Draw simple muzzle flash in center-bottom of screen
    int centerX = screenWidth / 2;
    int flashY = screenHeight / 4;  // Upper part of lower half
    
    // Flash size based on weapon
    int flashSize = 20;
    if (weapon.currentWeapon == WEAPON_SHOTGUN) flashSize = 40;
    if (weapon.currentWeapon == WEAPON_CHAINGUN) flashSize = 15;
    if (weapon.currentWeapon == WEAPON_FIST) return;  // No flash for fist
    
    // Draw flash (yellow/white)
    for (int y = flashY - flashSize; y <= flashY + flashSize; y++) {
        for (int x = centerX - flashSize; x <= centerX + flashSize; x++) {
            if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) continue;
            
            int dx = x - centerX;
            int dy = y - flashY;
            int dist = dx * dx + dy * dy;
            
            if (dist < flashSize * flashSize / 4) {
                pixelFunc(x, y, 255, 255, 255);  // White core
            } else if (dist < flashSize * flashSize) {
                if ((x + y) % 2 == 0) {
                    pixelFunc(x, y, 255, 200, 50);  // Yellow outer
                }
            }
        }
    }
}

// Draw crosshair
void drawCrosshair(void (*pixelFunc)(int, int, int, int, int),
                   int screenWidth, int screenHeight,
                   int targetingEnemy) {
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    
    // Crosshair color (red if targeting enemy, white otherwise)
    int r = targetingEnemy ? 255 : 200;
    int g = targetingEnemy ? 0 : 200;
    int b = targetingEnemy ? 0 : 200;
    
    // Crosshair size
    int size = 4;
    int gap = 2;
    
    // Draw four lines
    // Top
    for (int y = centerY - size - gap; y < centerY - gap; y++) {
        pixelFunc(centerX, y, r, g, b);
    }
    // Bottom
    for (int y = centerY + gap + 1; y <= centerY + size + gap; y++) {
        pixelFunc(centerX, y, r, g, b);
    }
    // Left
    for (int x = centerX - size - gap; x < centerX - gap; x++) {
        pixelFunc(x, centerY, r, g, b);
    }
    // Right
    for (int x = centerX + gap + 1; x <= centerX + size + gap; x++) {
        pixelFunc(x, centerY, r, g, b);
    }
    
    // Center dot
    pixelFunc(centerX, centerY, r, g, b);
}

#endif // WEAPON_H
