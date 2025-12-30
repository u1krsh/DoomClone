#ifndef GLASS_H
#define GLASS_H

#include <math.h>
#include "data_types.h"

// ============================================================================
// Glass Material System
// ============================================================================
// Provides thick tinted glass walls with:
// - Variable tint colors (RGB)
// - Fresnel effect (more reflective at angles)
// - Specular highlights from lights
// - Thickness effect (double edge rendering)
// ============================================================================

// Material types
#define MATERIAL_SOLID 0
#define MATERIAL_GLASS 1
#define MATERIAL_FENCE 2  // Fully transparent (only texture alpha)

// Glass rendering constants
#define GLASS_BASE_OPACITY 0.35f      // Base opacity (35%)
#define GLASS_FRESNEL_POWER 2.0f      // Fresnel exponent (higher = sharper edge effect)
#define GLASS_FRESNEL_MIN 0.1f        // Minimum Fresnel contribution
#define GLASS_FRESNEL_MAX 0.6f        // Maximum Fresnel contribution
#define GLASS_SPECULAR_POWER 32.0f    // Specular sharpness
#define GLASS_SPECULAR_INTENSITY 0.8f // Specular brightness
#define GLASS_THICKNESS 3             // Pixels for thickness effect

// Default glass tint colors (preset options)
#define GLASS_TINT_CLEAR_R 220
#define GLASS_TINT_CLEAR_G 230
#define GLASS_TINT_CLEAR_B 240

#define GLASS_TINT_BLUE_R 150
#define GLASS_TINT_BLUE_G 180
#define GLASS_TINT_BLUE_B 220

#define GLASS_TINT_GREEN_R 150
#define GLASS_TINT_GREEN_G 200
#define GLASS_TINT_GREEN_B 160

#define GLASS_TINT_AMBER_R 220
#define GLASS_TINT_AMBER_G 180
#define GLASS_TINT_AMBER_B 120

#define GLASS_TINT_RED_R 220
#define GLASS_TINT_RED_G 140
#define GLASS_TINT_RED_B 140

// ============================================================================
// Helper math functions
// ============================================================================

static inline float clampf(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static inline int clampi(int v, int min, int max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

// Calculate Fresnel factor (view-angle based reflection)
// Returns value from 0 (perpendicular view) to 1 (grazing angle)
static float calculateFresnel(float viewDotNormal) {
    // Schlick's approximation for Fresnel
    float f0 = 0.04f;  // Base reflectance for glass
    float fresnel = f0 + (1.0f - f0) * powf(1.0f - viewDotNormal, GLASS_FRESNEL_POWER);
    return clampf(fresnel, GLASS_FRESNEL_MIN, GLASS_FRESNEL_MAX);
}

// Calculate specular highlight intensity from a light source
// lightDir: normalized direction TO the light
// viewDir: normalized view direction
// normal: surface normal
static float calculateSpecular(float lightDirX, float lightDirY, float lightDirZ,
                               float viewDirX, float viewDirY, float viewDirZ,
                               float normalX, float normalY, float normalZ) {
    // Calculate reflection vector: R = 2*(N.L)*N - L
    float ndotl = normalX * lightDirX + normalY * lightDirY + normalZ * lightDirZ;
    if (ndotl < 0) return 0.0f;
    
    float reflX = 2.0f * ndotl * normalX - lightDirX;
    float reflY = 2.0f * ndotl * normalY - lightDirY;
    float reflZ = 2.0f * ndotl * normalZ - lightDirZ;
    
    // Calculate specular: (R.V)^n
    float rdotv = reflX * viewDirX + reflY * viewDirY + reflZ * viewDirZ;
    if (rdotv < 0) return 0.0f;
    
    float specular = powf(rdotv, GLASS_SPECULAR_POWER) * GLASS_SPECULAR_INTENSITY;
    return clampf(specular, 0.0f, 1.0f);
}

// ============================================================================
// Frame buffer for glass rendering (stores what's behind glass)
// ============================================================================

// We need to store the background color to blend with glass
// This will be populated during solid wall rendering
static int glassBackgroundR[SW * SH];
static int glassBackgroundG[SW * SH];
static int glassBackgroundB[SW * SH];
static int glassBackgroundValid[SW * SH];

// Clear glass background buffer
static void clearGlassBackground(void) {
    for (int i = 0; i < SW * SH; i++) {
        glassBackgroundValid[i] = 0;
    }
}

// Set background pixel (called from solid wall rendering)
static inline void setGlassBackground(int x, int y, int r, int g, int b) {
    if (x >= 0 && x < SW && y >= 0 && y < SH) {
        int idx = y * SW + x;
        glassBackgroundR[idx] = r;
        glassBackgroundG[idx] = g;
        glassBackgroundB[idx] = b;
        glassBackgroundValid[idx] = 1;
    }
}

// Get background pixel for blending
static inline int getGlassBackground(int x, int y, int* r, int* g, int* b) {
    if (x >= 0 && x < SW && y >= 0 && y < SH) {
        int idx = y * SW + x;
        if (glassBackgroundValid[idx]) {
            *r = glassBackgroundR[idx];
            *g = glassBackgroundG[idx];
            *b = glassBackgroundB[idx];
            return 1;
        }
    }
    // Default background (dark blue-gray like sky)
    *r = 30;
    *g = 40;
    *b = 60;
    return 0;
}

// ============================================================================
// Glass wall storage for deferred rendering
// ============================================================================

#define MAX_GLASS_WALLS 64

typedef struct {
    int x1, x2;           // Screen X range
    int y1[SW], y2[SW];   // Screen Y range per column (top/bottom)
    float depth[SW];      // Depth per column
    int tint_r, tint_g, tint_b;
    int opacity;
    float normalX, normalY;  // Wall normal (for Fresnel/specular)
    int worldX, worldY;      // World position for lighting
    int texIdx;              // Texture index
    int valid;               // Is this entry used?
} GlassWallData;

static GlassWallData glassWalls[MAX_GLASS_WALLS];
static int numGlassWalls = 0;

// Clear glass wall list
static void clearGlassWalls(void) {
    numGlassWalls = 0;
    for (int i = 0; i < MAX_GLASS_WALLS; i++) {
        glassWalls[i].valid = 0;
    }
}

// Add a glass wall for deferred rendering
static int addGlassWall(int x1, int x2, int tint_r, int tint_g, int tint_b, 
                        int opacity, float normalX, float normalY,
                        int worldX, int worldY, int texIdx) {
    if (numGlassWalls >= MAX_GLASS_WALLS) return -1;
    
    int idx = numGlassWalls++;
    glassWalls[idx].x1 = x1;
    glassWalls[idx].x2 = x2;
    glassWalls[idx].tint_r = tint_r;
    glassWalls[idx].tint_g = tint_g;
    glassWalls[idx].tint_b = tint_b;
    glassWalls[idx].opacity = opacity;
    glassWalls[idx].normalX = normalX;
    glassWalls[idx].normalY = normalY;
    glassWalls[idx].worldX = worldX;
    glassWalls[idx].worldY = worldY;
    glassWalls[idx].texIdx = texIdx;
    glassWalls[idx].valid = 1;
    
    // Initialize Y ranges
    for (int x = 0; x < SW; x++) {
        glassWalls[idx].y1[x] = SH;
        glassWalls[idx].y2[x] = 0;
        glassWalls[idx].depth[x] = 99999.0f;
    }
    
    return idx;
}

// Set column data for a glass wall
static void setGlassWallColumn(int wallIdx, int screenX, int yTop, int yBot, float depth) {
    if (wallIdx < 0 || wallIdx >= numGlassWalls) return;
    if (screenX < 0 || screenX >= SW) return;
    
    glassWalls[wallIdx].y1[screenX] = yTop;
    glassWalls[wallIdx].y2[screenX] = yBot;
    glassWalls[wallIdx].depth[screenX] = depth;
}

// ============================================================================
// Main glass rendering function
// Called after all solid walls are rendered
// ============================================================================

// External pixel function from main game
extern void pixel(int x, int y, int r, int g, int b);

void renderGlassWalls(float playerX, float playerY, float playerZ, 
                      float playerAngle, float cosAngle, float sinAngle) {
    
    if (numGlassWalls > 0) {
        printf("RENDER: Processing %d glass walls\n", numGlassWalls);
    }
    
    for (int w = 0; w < numGlassWalls; w++) {
        if (!glassWalls[w].valid) continue;
        
        GlassWallData* gw = &glassWalls[w];
        
        // Calculate view direction (player looking direction)
        float viewDirX = sinAngle;
        float viewDirY = cosAngle;
        float viewDirZ = 0;
        
        // Calculate dot product for Fresnel (view . normal)
        float viewDotNormal = fabsf(viewDirX * gw->normalX + viewDirY * gw->normalY);
        float fresnel = calculateFresnel(viewDotNormal);
        
        // Base opacity modulated by Fresnel (edges are more reflective)
        float baseOpacity = (float)gw->opacity / 255.0f;
        float opacity = baseOpacity + fresnel * (1.0f - baseOpacity) * 0.5f;
        opacity = clampf(opacity, 0.0f, 0.95f);
        
        int columnsRendered = 0;
        int pixelsRendered = 0;
        
        // Render each column
        for (int x = gw->x1; x < gw->x2 && x < SW; x++) {
            if (x < 0) continue;
            
            int yTop = gw->y1[x];
            int yBot = gw->y2[x];
            float depth = gw->depth[x];
            
            // Debug first few columns
            if (x == gw->x1 || x == gw->x1 + 1) {
                printf("  Col x=%d: yTop=%d, yBot=%d, depth=%.1f\n", x, yTop, yBot, depth);
            }
            
            if (yTop >= yBot || yTop >= SH || yBot < 0) continue;
            if (depth >= 99999.0f) continue;
            
            columnsRendered++;
            
            // Clamp Y range
            if (yTop < 0) yTop = 0;
            if (yBot >= SH) yBot = SH - 1;
            
            // Distance-based shading
            float shade = 1.0f - depth / 600.0f;
            shade = clampf(shade, 0.3f, 1.0f);
            
            // Apply tint with shading
            int tintR = (int)(gw->tint_r * shade);
            int tintG = (int)(gw->tint_g * shade);
            int tintB = (int)(gw->tint_b * shade);
            
            // Calculate specular highlight (fake light from above-front)
            float lightDirX = -sinAngle * 0.2f;
            float lightDirY = -cosAngle * 0.2f;
            float lightDirZ = 0.9f;  // Light mostly from above
            float specular = calculateSpecular(lightDirX, lightDirY, lightDirZ,
                                               -viewDirX, -viewDirY, 0,
                                               gw->normalX, gw->normalY, 0);
            
            // Render each pixel in the column
            for (int y = yTop; y < yBot; y++) {
                // Get background color
                int bgR, bgG, bgB;
                getGlassBackground(x, y, &bgR, &bgG, &bgB);
                
                // Blend: background * (1-opacity) + tint * opacity
                float invOpacity = 1.0f - opacity;
                int r = (int)(bgR * invOpacity + tintR * opacity);
                int g = (int)(bgG * invOpacity + tintG * opacity);
                int b = (int)(bgB * invOpacity + tintB * opacity);
                
                // Add specular highlight (white shine)
                int specAdd = (int)(specular * 200 * shade);
                r += specAdd;
                g += specAdd;
                b += specAdd;
                
                // Add Fresnel edge brightness
                int fresnelAdd = (int)(fresnel * 50 * shade);
                r += fresnelAdd;
                g += fresnelAdd;
                b += fresnelAdd;
                
                // Clamp final color
                r = clampi(r, 0, 255);
                g = clampi(g, 0, 255);
                b = clampi(b, 0, 255);
                
                pixel(x, y, r, g, b);
            }
            
            // Thickness effect: Draw dark edge line at bottom
            if (yBot < SH - 1) {
                int edgeY = yBot;
                int edgeShade = (int)(30 * shade);
                int bgR, bgG, bgB;
                getGlassBackground(x, edgeY, &bgR, &bgG, &bgB);
                pixel(x, edgeY, clampi(bgR - edgeShade, 0, 255), 
                               clampi(bgG - edgeShade, 0, 255), 
                               clampi(bgB - edgeShade, 0, 255));
            }
            
            // Thickness effect: Draw highlight at top edge
            if (yTop > 0) {
                int edgeY = yTop;
                int edgeShade = (int)(40 * shade);
                int bgR, bgG, bgB;
                getGlassBackground(x, edgeY, &bgR, &bgG, &bgB);
                pixel(x, edgeY, clampi(tintR + edgeShade, 0, 255), 
                               clampi(tintG + edgeShade, 0, 255), 
                               clampi(tintB + edgeShade, 0, 255));
            }
        }
    }
}

// ============================================================================
// Initialize glass system
// ============================================================================

void initGlassSystem(void) {
    clearGlassWalls();
    clearGlassBackground();
}

#endif // GLASS_H
