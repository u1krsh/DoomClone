#ifndef EFFECTS_H
#define EFFECTS_H

// Visual Effects Module
// Provides screen shake, particles, head bobbing, and other visual effects

#include <stdlib.h>
#include <math.h>

// ============== SCREEN SHAKE ==============
#define MAX_SCREEN_SHAKE 15
#define SHAKE_DECAY_RATE 0.85f

typedef struct {
    float intensity;          // Current shake intensity
    float offsetX;            // Current X offset
    float offsetY;            // Current Y offset
    int lastUpdateTime;       // Last time shake was updated
} ScreenShake;

ScreenShake screenShake = {0, 0, 0, 0};

// Add screen shake
void addScreenShake(float amount) {
    screenShake.intensity += amount;
    if (screenShake.intensity > MAX_SCREEN_SHAKE) {
        screenShake.intensity = MAX_SCREEN_SHAKE;
    }
}

// Update screen shake (call each frame)
void updateScreenShake(int currentTime) {
    if (screenShake.intensity > 0.1f) {
        // Generate random offset based on intensity
        screenShake.offsetX = ((float)(rand() % 100) / 50.0f - 1.0f) * screenShake.intensity;
        screenShake.offsetY = ((float)(rand() % 100) / 50.0f - 1.0f) * screenShake.intensity;
        
        // Decay shake
        screenShake.intensity *= SHAKE_DECAY_RATE;
    } else {
        screenShake.intensity = 0;
        screenShake.offsetX = 0;
        screenShake.offsetY = 0;
    }
    screenShake.lastUpdateTime = currentTime;
}

// Get current shake offset
int getShakeOffsetX(void) { return (int)screenShake.offsetX; }
int getShakeOffsetY(void) { return (int)screenShake.offsetY; }

// ============== HEAD BOB ==============
typedef struct {
    float phase;              // Current bob phase (0-360)
    float intensity;          // Bob intensity
    int lastMoveTime;         // Last time player moved
} HeadBob;

HeadBob headBob = {0, 0, 0};

#define HEAD_BOB_SPEED 12.0f
#define HEAD_BOB_INTENSITY 3.0f

// Update head bobbing
void updateHeadBob(int isMoving, int currentTime) {
    if (isMoving) {
        headBob.phase += HEAD_BOB_SPEED;
        if (headBob.phase >= 360.0f) headBob.phase -= 360.0f;
        headBob.intensity = HEAD_BOB_INTENSITY;
        headBob.lastMoveTime = currentTime;
    } else {
        // Gradually reduce bob when stopped
        headBob.intensity *= 0.9f;
    }
}

// Get current head bob offset
int getHeadBobOffset(float* sinTable) {
    if (headBob.intensity < 0.1f) return 0;
    int phaseInt = (int)headBob.phase % 360;
    return (int)(sinTable[phaseInt] * headBob.intensity);
}

// ============== BLOOD PARTICLES ==============
#define MAX_PARTICLES 64
#define PARTICLE_LIFETIME 500  // ms

typedef struct {
    int active;
    float x, y, z;            // World position
    float vx, vy, vz;         // Velocity
    int startTime;            // When particle was created
    int r, g, b;              // Color
    int size;                 // Particle size
} Particle;

Particle particles[MAX_PARTICLES];
int numActiveParticles = 0;

void initParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = 0;
    }
    numActiveParticles = 0;
}

// Spawn blood particles at position
void spawnBloodParticles(int worldX, int worldY, int worldZ, int count) {
    for (int c = 0; c < count && numActiveParticles < MAX_PARTICLES; c++) {
        // Find inactive particle
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                particles[i].active = 1;
                particles[i].x = (float)worldX;
                particles[i].y = (float)worldY;
                particles[i].z = (float)worldZ;
                
                // Random velocity
                particles[i].vx = ((float)(rand() % 100) / 25.0f - 2.0f);
                particles[i].vy = ((float)(rand() % 100) / 25.0f - 2.0f);
                particles[i].vz = ((float)(rand() % 50) / 25.0f);  // Upward bias
                
                particles[i].startTime = 0;  // Will be set on first update
                
                // Blood color variations
                particles[i].r = 150 + (rand() % 105);
                particles[i].g = rand() % 30;
                particles[i].b = rand() % 30;
                particles[i].size = 1 + (rand() % 2);
                
                numActiveParticles++;
                break;
            }
        }
    }
}

// Update all particles
void updateParticles(int currentTime) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        // Initialize start time if needed
        if (particles[i].startTime == 0) {
            particles[i].startTime = currentTime;
        }
        
        // Check lifetime
        if (currentTime - particles[i].startTime > PARTICLE_LIFETIME) {
            particles[i].active = 0;
            numActiveParticles--;
            continue;
        }
        
        // Update position
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].z += particles[i].vz;
        
        // Gravity
        particles[i].vz -= 0.2f;
    }
}

// Draw particles (call from 3D renderer with player info)
void drawParticles(void (*pixelFunc)(int, int, int, int, int),
                   int screenWidth, int screenHeight,
                   int playerX, int playerY, int playerZ, int playerAngle,
                   float* cosTable, float* sinTable,
                   int currentTime) {
    float CS = cosTable[playerAngle];
    float SN = sinTable[playerAngle];
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        // Transform to camera space
        float relX = particles[i].x - (float)playerX;
        float relY = particles[i].y - (float)playerY;
        float relZ = particles[i].z - (float)playerZ;
        
        float camX = relX * CS - relY * SN;
        float camY = relX * SN + relY * CS;
        
        if (camY < 1.0f) continue;  // Behind player
        
        // Project to screen
        int screenX = (int)(camX * 200.0f / camY + screenWidth / 2);
        int screenY = (int)(relZ * 200.0f / camY + screenHeight / 2);
        
        // Fade based on lifetime
        float life = 1.0f - (float)(currentTime - particles[i].startTime) / (float)PARTICLE_LIFETIME;
        int r = (int)(particles[i].r * life);
        int g = (int)(particles[i].g * life);
        int b = (int)(particles[i].b * life);
        
        // Draw particle
        int size = particles[i].size;
        for (int py = -size; py <= size; py++) {
            for (int px = -size; px <= size; px++) {
                int dx = screenX + px;
                int dy = screenY + py;
                if (dx >= 0 && dx < screenWidth && dy >= 0 && dy < screenHeight) {
                    pixelFunc(dx, dy, r, g, b);
                }
            }
        }
    }
}

// ============== LOW HEALTH EFFECTS ==============
#define CRITICAL_HEALTH 25
#define HEARTBEAT_SPEED 800  // ms per beat

// Draw low health overlay (pulsing red vignette)
void drawLowHealthOverlay(void (*pixelFunc)(int, int, int, int, int),
                          int screenWidth, int screenHeight,
                          int playerHealth, int currentTime) {
    if (playerHealth >= CRITICAL_HEALTH || playerHealth <= 0) return;
    
    // Calculate pulse intensity (sine wave)
    float pulse = (float)sin((currentTime % HEARTBEAT_SPEED) * 3.14159f * 2.0f / HEARTBEAT_SPEED);
    pulse = (pulse + 1.0f) / 2.0f;  // Normalize to 0-1
    
    // Stronger effect at lower health
    float healthFactor = 1.0f - ((float)playerHealth / (float)CRITICAL_HEALTH);
    float intensity = pulse * healthFactor * 0.5f;
    
    // Draw red vignette from edges
    int maxDist = screenWidth / 3;
    
    for (int y = 0; y < screenHeight; y++) {
        for (int x = 0; x < screenWidth; x++) {
            // Calculate distance from edge
            int distLeft = x;
            int distRight = screenWidth - 1 - x;
            int distTop = screenHeight - 1 - y;
            int distBottom = y;
            
            int minDist = distLeft;
            if (distRight < minDist) minDist = distRight;
            if (distTop < minDist) minDist = distTop;
            if (distBottom < minDist) minDist = distBottom;
            
            if (minDist < maxDist) {
                float edgeFactor = 1.0f - ((float)minDist / (float)maxDist);
                edgeFactor *= edgeFactor;  // Square for smoother falloff
                
                // Only draw some pixels for transparency effect
                if ((x + y) % 3 == 0 && edgeFactor * intensity > 0.1f) {
                    int redIntensity = (int)(200 * edgeFactor * intensity);
                    pixelFunc(x, y, redIntensity, 0, 0);
                }
            }
        }
    }
}

// ============== KILL TRACKING (NO STREAK MESSAGES) ==============
// Register a kill (no-op, kept for compatibility)
void registerKill(int currentTime) {
    // Kill tracking removed - no streak messages
    (void)currentTime;
}

// Reset kill streak (no-op, kept for compatibility)
void resetKillStreak(void) {
    // Kill tracking removed
}

// Draw kill streak message (no-op, kept for compatibility)
void drawKillStreakMessage(void (*pixelFunc)(int, int, int, int, int),
                           int screenWidth, int screenHeight,
                           int currentTime) {
    // Kill streak messages removed
    (void)pixelFunc;
    (void)screenWidth;
    (void)screenHeight;
    (void)currentTime;
}

// ============== FLASH EFFECTS ==============
#define PICKUP_FLASH_DURATION 150

typedef struct {
    int flashTime;
    int r, g, b;
} FlashEffect;

FlashEffect flashEffect = {0, 0, 0, 0};

// Trigger a screen flash
void triggerFlash(int r, int g, int b, int currentTime) {
    flashEffect.flashTime = currentTime;
    flashEffect.r = r;
    flashEffect.g = g;
    flashEffect.b = b;
}

// Draw flash overlay
void drawFlashOverlay(void (*pixelFunc)(int, int, int, int, int),
                      int screenWidth, int screenHeight,
                      int currentTime) {
    int elapsed = currentTime - flashEffect.flashTime;
    if (elapsed > PICKUP_FLASH_DURATION) return;
    
    float intensity = 1.0f - ((float)elapsed / (float)PICKUP_FLASH_DURATION);
    intensity *= 0.4f;  // Max 40% opacity
    
    int r = (int)(flashEffect.r * intensity);
    int g = (int)(flashEffect.g * intensity);
    int b = (int)(flashEffect.b * intensity);
    
    // Dithered overlay
    for (int y = 0; y < screenHeight; y++) {
        for (int x = (y % 2); x < screenWidth; x += 2) {
            pixelFunc(x, y, r, g, b);
        }
    }
}

// ============== INITIALIZATION ==============
void initEffects(void) {
    screenShake.intensity = 0;
    screenShake.offsetX = 0;
    screenShake.offsetY = 0;
    
    headBob.phase = 0;
    headBob.intensity = 0;
    
    initParticles();
    
    flashEffect.flashTime = 0;
}

#endif // EFFECTS_H
