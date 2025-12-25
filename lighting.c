//=============================================================================
// LIGHTING.C - Dynamic Lighting Implementation
// Colored lighting with falloff, flicker effects, and fog
//=============================================================================

#include "lighting.h"
#include <stdlib.h>
#include <math.h>

//-----------------------------------------------------------------------------
// Global Lighting State
//-----------------------------------------------------------------------------

// Dark ambient by default for Doom-style atmosphere
int g_ambientR = 15;
int g_ambientG = 15;
int g_ambientB = 20;
int g_ambientIntensity = 30;  // Very dark (0-255)

// Fog settings - atmospheric fog for depth
FogSettings g_fog = {
    .enabled = 1,
    .r = 20,          // Slightly brighter fog color
    .g = 25,
    .b = 35,
    .startDist = 50.0f,   // Fog starts closer
    .endDist = 500.0f,    // Full fog at shorter distance
    .density = 0.004f     // Stronger density for more visible effect
};

// Light array
Light g_lights[MAX_LIGHTS];
int g_numLights = 0;

// Player flashlight
int g_flashlightEnabled = 0;
int g_flashlightIntensity = 200;
int g_flashlightRadius = 300;

//-----------------------------------------------------------------------------
// Internal Helpers
//-----------------------------------------------------------------------------

static float fastSqrt(float x) {
    // Quick approximation for performance
    if (x <= 0) return 0;
    return sqrtf(x);
}

static float clampf(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static int clampi(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

// Simple pseudo-random for flicker (deterministic per phase)
static int flickerRandom(int seed) {
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return seed % 256;
}

//-----------------------------------------------------------------------------
// Lighting System Initialization
//-----------------------------------------------------------------------------

void initLighting(void) {
    // Reset all lights
    for (int i = 0; i < MAX_LIGHTS; i++) {
        g_lights[i].active = 0;
        g_lights[i].x = 0;
        g_lights[i].y = 0;
        g_lights[i].z = 0;
        g_lights[i].radius = 400;    // Increased for better coverage
        g_lights[i].intensity = 220;  // Slightly brighter
        g_lights[i].r = 255;
        g_lights[i].g = 255;
        g_lights[i].b = 255;
        g_lights[i].type = LIGHT_TYPE_POINT;
        g_lights[i].spotAngle = 45;
        g_lights[i].spotDirX = 0;
        g_lights[i].spotDirY = 0;
        g_lights[i].spotDirZ = -100;
        g_lights[i].flickerType = FLICKER_NONE;
        g_lights[i].flickerSpeed = 5;
        g_lights[i].flickerPhase = 0;
        g_lights[i].currentIntensity = 200.0f;
    }
    g_numLights = 0;
}

//-----------------------------------------------------------------------------
// Flicker Animation Update
//-----------------------------------------------------------------------------

void updateLightFlicker(int frameTimeMs) {
    for (int i = 0; i < g_numLights; i++) {
        if (!g_lights[i].active) continue;
        
        Light *light = &g_lights[i];
        float baseIntensity = (float)light->intensity;
        
        // Advance phase based on speed and frame time
        light->flickerPhase += light->flickerSpeed * frameTimeMs / 50;
        
        switch (light->flickerType) {
            case FLICKER_NONE:
                light->currentIntensity = baseIntensity;
                break;
                
            case FLICKER_CANDLE: {
                // Organic flickering - multiple overlapping sine waves
                float phase = light->flickerPhase * 0.01f;
                float flicker = 0.7f + 
                    0.15f * sinf(phase * 2.3f) +
                    0.10f * sinf(phase * 5.7f) +
                    0.05f * sinf(phase * 11.3f);
                // Add random jitter
                int jitter = flickerRandom(light->flickerPhase) % 30 - 15;
                light->currentIntensity = baseIntensity * clampf(flicker + jitter * 0.003f, 0.5f, 1.0f);
                break;
            }
            
            case FLICKER_STROBE: {
                // On/off blinking
                int period = 100 / (light->flickerSpeed + 1);
                int phase = (light->flickerPhase / 10) % (period * 2);
                light->currentIntensity = (phase < period) ? baseIntensity : baseIntensity * 0.1f;
                break;
            }
            
            case FLICKER_PULSE: {
                // Smooth sine wave pulsing
                float phase = light->flickerPhase * 0.005f;
                float pulse = 0.5f + 0.5f * sinf(phase);
                light->currentIntensity = baseIntensity * (0.3f + pulse * 0.7f);
                break;
            }
            
            case FLICKER_RANDOM: {
                // Random intensity each update
                int rand = flickerRandom(light->flickerPhase + i * 1000);
                float mult = 0.5f + (rand / 255.0f) * 0.5f;
                light->currentIntensity = baseIntensity * mult;
                break;
            }
            
            default:
                light->currentIntensity = baseIntensity;
                break;
        }
    }
}

//-----------------------------------------------------------------------------
// Distance Calculation
//-----------------------------------------------------------------------------

float distanceToLight(int px, int py, int pz, int lightIndex) {
    if (lightIndex < 0 || lightIndex >= g_numLights) return 99999.0f;
    
    Light *light = &g_lights[lightIndex];
    float dx = (float)(px - light->x);
    float dy = (float)(py - light->y);
    float dz = (float)(pz - light->z);
    
    return fastSqrt(dx*dx + dy*dy + dz*dz);
}

//-----------------------------------------------------------------------------
// Single Light Contribution
//-----------------------------------------------------------------------------

void calculateSingleLight(const Light *light, 
                          int worldX, int worldY, int worldZ,
                          float *outR, float *outG, float *outB) {
    if (!light->active) {
        *outR = 0; *outG = 0; *outB = 0;
        return;
    }
    
    // Calculate distance to light
    float dx = (float)(worldX - light->x);
    float dy = (float)(worldY - light->y);
    float dz = (float)(worldZ - light->z);
    float dist = fastSqrt(dx*dx + dy*dy + dz*dz);
    
    // Check if outside light radius
    if (dist > (float)light->radius) {
        *outR = 0; *outG = 0; *outB = 0;
        return;
    }
    
    // Calculate falloff (quadratic for realistic lighting)
    float normalizedDist = dist / (float)light->radius;
    // Use smooth linear falloff instead of quadratic for more even light spread
    float falloff = 1.0f - normalizedDist;
    // Apply slight curve for soft edge but not as aggressive as quadratic
    falloff = falloff * (0.5f + 0.5f * falloff);
    
    // Handle spotlight cone check
    if (light->type == LIGHT_TYPE_SPOT) {
        // Normalize direction from light to point
        float invDist = (dist > 1.0f) ? 1.0f / dist : 1.0f;
        float dirToPointX = dx * invDist;
        float dirToPointY = dy * invDist;
        float dirToPointZ = dz * invDist;
        
        // Normalize spotlight direction (stored as *100)
        float spotLen = fastSqrt(
            (float)(light->spotDirX * light->spotDirX) +
            (float)(light->spotDirY * light->spotDirY) +
            (float)(light->spotDirZ * light->spotDirZ)
        );
        if (spotLen < 1.0f) spotLen = 1.0f;
        float spotDirX = light->spotDirX / spotLen;
        float spotDirY = light->spotDirY / spotLen;
        float spotDirZ = light->spotDirZ / spotLen;
        
        // Dot product to check angle
        float dot = dirToPointX * spotDirX + dirToPointY * spotDirY + dirToPointZ * spotDirZ;
        float coneAngle = cosf((float)light->spotAngle * 0.5f * 3.14159f / 180.0f);
        
        if (dot < coneAngle) {
            // Outside spotlight cone
            *outR = 0; *outG = 0; *outB = 0;
            return;
        }
        
        // Soft edge falloff near cone boundary
        float spotFalloff = (dot - coneAngle) / (1.0f - coneAngle);
        falloff *= clampf(spotFalloff * 2.0f, 0.0f, 1.0f);
    }
    
    // Apply current intensity (includes flicker)
    float intensityMult = light->currentIntensity / 255.0f;
    
    // Calculate final light contribution
    *outR = (light->r / 255.0f) * falloff * intensityMult;
    *outG = (light->g / 255.0f) * falloff * intensityMult;
    *outB = (light->b / 255.0f) * falloff * intensityMult;
}

//-----------------------------------------------------------------------------
// Calculate Total Lighting at Point
//-----------------------------------------------------------------------------

void calculateLightingAtPoint(int worldX, int worldY, int worldZ,
                              int baseR, int baseG, int baseB,
                              int *outR, int *outG, int *outB) {
    // Start with ambient light
    float totalR = g_ambientR / 255.0f * g_ambientIntensity / 255.0f;
    float totalG = g_ambientG / 255.0f * g_ambientIntensity / 255.0f;
    float totalB = g_ambientB / 255.0f * g_ambientIntensity / 255.0f;
    
    // Add contribution from each active light (additive blending)
    for (int i = 0; i < g_numLights; i++) {
        if (!g_lights[i].active) continue;
        
        float lr, lg, lb;
        calculateSingleLight(&g_lights[i], worldX, worldY, worldZ, &lr, &lg, &lb);
        
        totalR += lr;
        totalG += lg;
        totalB += lb;
    }
    
    // Clamp total light to 0-1 range
    totalR = clampf(totalR, 0.0f, 1.5f);  // Allow slight overbright
    totalG = clampf(totalG, 0.0f, 1.5f);
    totalB = clampf(totalB, 0.0f, 1.5f);
    
    // Apply lighting to base texture color
    *outR = clampi((int)(baseR * totalR), 0, 255);
    *outG = clampi((int)(baseG * totalG), 0, 255);
    *outB = clampi((int)(baseB * totalB), 0, 255);
}

//-----------------------------------------------------------------------------
// Fog Application
//-----------------------------------------------------------------------------

void applyFog(int *r, int *g, int *b, float distance) {
    if (!g_fog.enabled) return;
    
    // Calculate fog factor based on distance
    float fogFactor = 0.0f;
    
    if (distance >= g_fog.endDist) {
        fogFactor = 1.0f;
    } else if (distance > g_fog.startDist) {
        // Smooth exponential fog blend
        float normalizedDist = (distance - g_fog.startDist) / (g_fog.endDist - g_fog.startDist);
        fogFactor = 1.0f - expf(-g_fog.density * normalizedDist * normalizedDist * 1000.0f);
        fogFactor = clampf(fogFactor, 0.0f, 1.0f);
    }
    
    // Blend toward fog color
    *r = (int)(*r * (1.0f - fogFactor) + g_fog.r * fogFactor);
    *g = (int)(*g * (1.0f - fogFactor) + g_fog.g * fogFactor);
    *b = (int)(*b * (1.0f - fogFactor) + g_fog.b * fogFactor);
}

//-----------------------------------------------------------------------------
// Combined Lighting + Fog (Convenience Function)
//-----------------------------------------------------------------------------

void applyLightingToPixel(int *r, int *g, int *b,
                          int worldX, int worldY, int worldZ,
                          float distance) {
    // Apply dynamic lighting
    int litR, litG, litB;
    calculateLightingAtPoint(worldX, worldY, worldZ, *r, *g, *b, &litR, &litG, &litB);
    
    // Apply fog
    applyFog(&litR, &litG, &litB, distance);
    
    *r = litR;
    *g = litG;
    *b = litB;
}

//-----------------------------------------------------------------------------
// Light Management
//-----------------------------------------------------------------------------

int addLight(int x, int y, int z, int radius, int intensity,
             int r, int g, int b, int type, int flickerType) {
    // Find first inactive slot
    int slot = -1;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (!g_lights[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return -1;  // No available slots
    
    Light *light = &g_lights[slot];
    light->active = 1;
    light->x = x;
    light->y = y;
    light->z = z;
    light->radius = radius;
    light->intensity = intensity;
    light->r = r;
    light->g = g;
    light->b = b;
    light->type = type;
    light->flickerType = flickerType;
    light->flickerSpeed = 5;
    light->flickerPhase = 0;
    light->currentIntensity = (float)intensity;
    light->spotAngle = 45;
    light->spotDirX = 0;
    light->spotDirY = 0;
    light->spotDirZ = -100;
    
    if (slot >= g_numLights) {
        g_numLights = slot + 1;
    }
    
    return slot;
}

void removeLight(int index) {
    if (index < 0 || index >= MAX_LIGHTS) return;
    
    g_lights[index].active = 0;
    
    // Update numLights if we removed the last one
    while (g_numLights > 0 && !g_lights[g_numLights - 1].active) {
        g_numLights--;
    }
}
