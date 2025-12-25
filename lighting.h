#ifndef LIGHTING_H
#define LIGHTING_H

//=============================================================================
// DYNAMIC LIGHTING SYSTEM
// Intricate colored lighting with fog, flicker effects, and multiple light types
//=============================================================================

#include <math.h>

// Maximum lights in the scene
#define MAX_LIGHTS 64

// Light types
#define LIGHT_TYPE_POINT 0   // Omni-directional point light
#define LIGHT_TYPE_SPOT  1   // Directional spotlight with cone

// Flicker patterns for animated lights
#define FLICKER_NONE    0    // Static light
#define FLICKER_CANDLE  1    // Subtle organic flickering
#define FLICKER_STROBE  2    // On/off blinking
#define FLICKER_PULSE   3    // Smooth sine wave pulsing
#define FLICKER_RANDOM  4    // Random intensity variation

// Light falloff modes
#define FALLOFF_LINEAR    0  // Linear falloff (more gradual)
#define FALLOFF_QUADRATIC 1  // Inverse square (realistic, sharper)

//-----------------------------------------------------------------------------
// Light Structure
//-----------------------------------------------------------------------------
typedef struct {
    int active;              // Is this light enabled?
    
    // Position in world coordinates
    int x, y, z;
    
    // Light properties
    int radius;              // Maximum reach in world units
    int intensity;           // Base brightness (0-255)
    
    // RGB Color (0-255 each)
    int r, g, b;
    
    // Light type
    int type;                // LIGHT_TYPE_POINT or LIGHT_TYPE_SPOT
    
    // Spotlight properties (only used if type == LIGHT_TYPE_SPOT)
    int spotAngle;           // Cone angle in degrees (0-180)
    int spotDirX, spotDirY, spotDirZ;  // Direction vector (normalized * 100)
    
    // Flicker animation
    int flickerType;         // FLICKER_* constant
    int flickerSpeed;        // Animation speed (1-10)
    int flickerPhase;        // Internal animation phase
    
    // Calculated at runtime
    float currentIntensity;  // Actual intensity after flicker applied
} Light;

//-----------------------------------------------------------------------------
// Fog Settings
//-----------------------------------------------------------------------------
typedef struct {
    int enabled;             // Is fog active?
    int r, g, b;             // Fog color
    float startDist;         // Distance where fog begins
    float endDist;           // Distance where fully fogged
    float density;           // Exponential fog density (0.0-1.0)
} FogSettings;

//-----------------------------------------------------------------------------
// Global Lighting State
//-----------------------------------------------------------------------------

// Ambient light (dark by default for Doom-style atmosphere)
extern int g_ambientR;
extern int g_ambientG;
extern int g_ambientB;
extern int g_ambientIntensity;  // 0-255, controls how dark "dark" is

// Fog configuration
extern FogSettings g_fog;

// Light array
extern Light g_lights[MAX_LIGHTS];
extern int g_numLights;

// Player flashlight (optional directional light from camera)
extern int g_flashlightEnabled;
extern int g_flashlightIntensity;
extern int g_flashlightRadius;

//-----------------------------------------------------------------------------
// Lighting Functions
//-----------------------------------------------------------------------------

// Initialize lighting system with dark ambient
void initLighting(void);

// Update flicker animations (call once per frame)
void updateLightFlicker(int frameTimeMs);

// Calculate total light contribution at a world point
// Returns combined RGB values accounting for all lights + ambient
void calculateLightingAtPoint(int worldX, int worldY, int worldZ,
                              int baseR, int baseG, int baseB,
                              int *outR, int *outG, int *outB);

// Apply fog effect based on distance
// Modifies r,g,b in-place to blend toward fog color
void applyFog(int *r, int *g, int *b, float distance);

// Combined lighting + fog in one call (convenience function)
// Takes texture color, world position, and distance
// Outputs final lit & fogged color
void applyLightingToPixel(int *r, int *g, int *b,
                          int worldX, int worldY, int worldZ,
                          float distance);

// Add a new light at runtime
int addLight(int x, int y, int z, int radius, int intensity,
             int r, int g, int b, int type, int flickerType);

// Remove a light by index
void removeLight(int index);

// Get distance from a point to a light
float distanceToLight(int px, int py, int pz, int lightIndex);

// Calculate light contribution from a single light source
void calculateSingleLight(const Light *light, 
                          int worldX, int worldY, int worldZ,
                          float *outR, float *outG, float *outB);

#endif // LIGHTING_H
