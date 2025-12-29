#ifndef GATES_H
#define GATES_H

#include <math.h>
#include "data_types.h"  // For TextureMaps

// Gate constants
#define MAX_GATES 32
#define MAX_SWITCHES 32

// Gate states
#define GATE_STATE_CLOSED 0
#define GATE_STATE_OPENING 1
#define GATE_STATE_OPEN 2
#define GATE_STATE_CLOSING 3

// Gate timing
#define GATE_OPEN_DURATION 3000  // ms to stay open before auto-close
#define GATE_USE_RADIUS 60       // Radius for E-key activation

// Gate structure
typedef struct {
    int x, y;           // Position
    int z_closed;       // Height when closed (floor level)
    int z_open;         // Height when fully open
    int z_current;      // Current height (animated)
    int gate_id;        // Unique ID for linking
    int trigger_radius; // Activation radius
    int texture;        // Wall texture
    int speed;          // Movement speed (units/frame)
    int width;          // Gate width
    int state;          // Current state
    int stateStartTime; // When state changed
    int active;
} Gate;

// Switch structure
typedef struct {
    int x, y, z;
    int linked_gate_id;
    int texture;
    int active;
} GameSwitch;  // Named GameSwitch to avoid conflict with C switch keyword

// Global arrays
Gate gates[MAX_GATES];
int numGates = 0;

GameSwitch switches[MAX_SWITCHES];
int numSwitches = 0;

// Initialize gate system
void initGates(void) {
    for (int i = 0; i < MAX_GATES; i++) {
        gates[i].active = 0;
        gates[i].state = GATE_STATE_CLOSED;
    }
    numGates = 0;
    
    for (int i = 0; i < MAX_SWITCHES; i++) {
        switches[i].active = 0;
    }
    numSwitches = 0;
}

// Find gate by ID
int findGateById(int gate_id) {
    for (int i = 0; i < numGates; i++) {
        if (gates[i].active && gates[i].gate_id == gate_id) {
            return i;
        }
    }
    return -1;
}

// Toggle gate state
void toggleGate(int gateIndex, int currentTime) {
    if (gateIndex < 0 || gateIndex >= numGates) return;
    if (!gates[gateIndex].active) return;
    
    Gate* g = &gates[gateIndex];
    
    if (g->state == GATE_STATE_CLOSED || g->state == GATE_STATE_CLOSING) {
        g->state = GATE_STATE_OPENING;
        g->stateStartTime = currentTime;
        printf("Gate %d: Opening\n", g->gate_id);
    } else if (g->state == GATE_STATE_OPEN || g->state == GATE_STATE_OPENING) {
        g->state = GATE_STATE_CLOSING;
        g->stateStartTime = currentTime;
        printf("Gate %d: Closing\n", g->gate_id);
    }
}

// Update all gates (animation)
void updateGates(int currentTime) {
    for (int i = 0; i < numGates; i++) {
        if (!gates[i].active) continue;
        
        Gate* g = &gates[i];
        
        switch (g->state) {
            case GATE_STATE_OPENING:
                // Move gate up
                g->z_current += g->speed;
                if (g->z_current >= g->z_open) {
                    g->z_current = g->z_open;
                    g->state = GATE_STATE_OPEN;
                    g->stateStartTime = currentTime;
                    printf("Gate %d: Fully open\n", g->gate_id);
                }
                break;
                
            case GATE_STATE_OPEN:
                // Auto-close after timeout
                if (currentTime - g->stateStartTime > GATE_OPEN_DURATION) {
                    g->state = GATE_STATE_CLOSING;
                    g->stateStartTime = currentTime;
                    printf("Gate %d: Auto-closing\n", g->gate_id);
                }
                break;
                
            case GATE_STATE_CLOSING:
                // Move gate down
                g->z_current -= g->speed;
                if (g->z_current <= g->z_closed) {
                    g->z_current = g->z_closed;
                    g->state = GATE_STATE_CLOSED;
                    printf("Gate %d: Fully closed\n", g->gate_id);
                }
                break;
                
            case GATE_STATE_CLOSED:
            default:
                // Nothing to do
                break;
        }
    }
}

// Check if position collides with any closed/closing gate
// Returns 1 if collision (blocked), 0 if clear
int checkGateCollision(int x, int y, int playerZ) {
    for (int i = 0; i < numGates; i++) {
        if (!gates[i].active) continue;
        
        Gate* g = &gates[i];
        
        // Only block if gate is not fully open
        if (g->state == GATE_STATE_OPEN) continue;
        
        // Check if player is within gate bounds
        int halfWidth = g->width / 2;
        int gateThickness = 10;  // Gate is thin barrier
        
        // Simple AABB check (gate oriented along X axis)
        if (x >= g->x - halfWidth - 8 && x <= g->x + halfWidth + 8 &&
            y >= g->y - gateThickness && y <= g->y + gateThickness) {
            // Check height - can pass if gate is open enough
            if (g->z_current < playerZ + 10) {
                return 1;  // Blocked
            }
        }
    }
    return 0;  // Clear
}

// Try to use (activate) nearby gate or switch
// Called when player presses R key
void tryUseNearby(int playerX, int playerY, int currentTime) {
    int closestGate = -1;
    int closestSwitch = -1;
    int minDist = 9999;
    
    // Check gates (direct activation)
    for (int i = 0; i < numGates; i++) {
        if (!gates[i].active) continue;
        
        int dx = playerX - gates[i].x;
        int dy = playerY - gates[i].y;
        int dist = (int)sqrt(dx * dx + dy * dy);
        
        if (dist < gates[i].trigger_radius && dist < minDist) {
            minDist = dist;
            closestGate = i;
            closestSwitch = -1;
        }
    }
    
    // Check switches
    for (int i = 0; i < numSwitches; i++) {
        if (!switches[i].active) continue;
        
        int dx = playerX - switches[i].x;
        int dy = playerY - switches[i].y;
        int dist = (int)sqrt(dx * dx + dy * dy);
        
        if (dist < GATE_USE_RADIUS && dist < minDist) {
            minDist = dist;
            closestSwitch = i;
            closestGate = -1;
        }
    }
    
    // Activate closest
    if (closestGate >= 0) {
        toggleGate(closestGate, currentTime);
    } else if (closestSwitch >= 0) {
        // Find linked gate and toggle it
        int linkedGateIdx = findGateById(switches[closestSwitch].linked_gate_id);
        if (linkedGateIdx >= 0) {
            toggleGate(linkedGateIdx, currentTime);
            printf("Switch activated gate %d\n", switches[closestSwitch].linked_gate_id);
        } else {
            printf("Switch: No gate with ID %d found\n", switches[closestSwitch].linked_gate_id);
        }
    }
}

// Forward declarations for rendering - these come from DoomTest.c
// Note: SW, SH are macros from data_types.h (included via DoomTest.c)
extern float depthBuffer[];     // Depth buffer for occlusion
extern void pixel(int x, int y, int r, int g, int b);

// Draw gates as oriented wall segments
// Uses player position and angle for perspective
void drawGates(int playerX, int playerY, int playerZ, int playerAngle, 
               float cosAngle, float sinAngle, int* textures, int texCount) {
    
    // Add external reference to Textures array
    extern TextureMaps Textures[];
    
    for (int i = 0; i < numGates; i++) {
        if (!gates[i].active) continue;
        
        Gate* g = &gates[i];
        
        // Gate is rendered as a wall segment
        // Endpoints of gate (perpendicular to player's facing direction for better visibility)
        int halfWidth = g->width / 2;
        int gx1 = g->x - halfWidth;
        int gy1 = g->y;
        int gx2 = g->x + halfWidth;
        int gy2 = g->y;
        
        // Transform to view space (relative to player)
        float x1 = (float)(gx1 - playerX);
        float y1 = (float)(gy1 - playerY);
        float x2 = (float)(gx2 - playerX);
        float y2 = (float)(gy2 - playerY);
        
        // Rotate to camera space (same as main game)
        float wx1 = x1 * cosAngle - y1 * sinAngle;
        float wy1 = x1 * sinAngle + y1 * cosAngle;
        float wx2 = x2 * cosAngle - y2 * sinAngle;
        float wy2 = x2 * sinAngle + y2 * cosAngle;
        
        // Skip if behind player
        if (wy1 <= 1 && wy2 <= 1) continue;
        
        // Clip to near plane
        if (wy1 < 1) {
            float t = (1 - wy1) / (wy2 - wy1);
            wx1 = wx1 + t * (wx2 - wx1);
            wy1 = 1;
        }
        if (wy2 < 1) {
            float t = (1 - wy2) / (wy1 - wy2);
            wx2 = wx2 + t * (wx1 - wx2);
            wy2 = 1;
        }
        
        // Project to screen
        int screenX1 = SW/2 + (int)(wx1 * 200 / wy1);
        int screenX2 = SW/2 + (int)(wx2 * 200 / wy2);
        
        // Calculate gate vertical bounds (using current animated Z position)
        int gateBottom = g->z_current;  // Animated position
        int gateTop = gateBottom + 40;  // Gate is 40 units tall
        
        // Apply look adjustment (same as walls)
        float lookAdjust1 = (playerAngle > 0) ? (playerAngle * wy1) / 32.0f : 0;
        float lookAdjust2 = (playerAngle > 0) ? (playerAngle * wy2) / 32.0f : 0;
        
        // Project Y coordinates for both depth points
        int screenBot1 = SH/2 + (int)((playerZ - gateBottom) * 200 / wy1);
        int screenTop1 = SH/2 + (int)((playerZ - gateTop) * 200 / wy1);
        int screenBot2 = SH/2 + (int)((playerZ - gateBottom) * 200 / wy2);
        int screenTop2 = SH/2 + (int)((playerZ - gateTop) * 200 / wy2);
        
        // Ensure left-to-right rendering order
        if (screenX1 > screenX2) {
            int tmp;
            float ftmp;
            tmp = screenX1; screenX1 = screenX2; screenX2 = tmp;
            tmp = screenBot1; screenBot1 = screenBot2; screenBot2 = tmp;
            tmp = screenTop1; screenTop1 = screenTop2; screenTop2 = tmp;
            ftmp = wy1; wy1 = wy2; wy2 = ftmp;
            ftmp = wx1; wx1 = wx2; wx2 = ftmp;
        }
        
        // Skip if too narrow
        if (screenX2 - screenX1 < 1) continue;
        
        // Get texture data (use wall texture if available)
        int texIdx = g->texture;
        int texWidth = 64, texHeight = 64;  // Default
        const unsigned char* texData = NULL;
        
        // Use Textures array if valid index
        if (texIdx >= 0 && texIdx < 32 && Textures[texIdx].name != NULL) {
            texWidth = Textures[texIdx].w;
            texHeight = Textures[texIdx].h;
            texData = Textures[texIdx].name;
        }
        
        // Draw vertical strips
        for (int x = screenX1; x < screenX2 && x < SW; x++) {
            if (x < 0) continue;
            
            // Interpolation factor
            float t = (float)(x - screenX1) / (float)(screenX2 - screenX1);
            
            // Interpolate depth (camera Y)
            float depth = wy1 + t * (wy2 - wy1);
            
            // Depth test - only draw if closer than current depth buffer
            if (depth >= depthBuffer[x]) continue;
            
            // Interpolate Y coordinates
            int yTop = (int)(screenTop1 + t * (screenTop2 - screenTop1));
            int yBot = (int)(screenBot1 + t * (screenBot2 - screenBot1));
            
            // Clamp to screen
            if (yTop < 0) yTop = 0;
            if (yBot >= SH) yBot = SH - 1;
            if (yTop >= yBot) continue;
            
            // Calculate texture U coordinate with perspective correction
            float tx_f = t * g->width;  // Position along gate width
            int tx = ((int)tx_f) % texWidth;
            if (tx < 0) tx += texWidth;
            
            // Draw vertical line with texture or fallback color
            int wallHeight = yBot - yTop;
            
            for (int y = yTop; y < yBot; y++) {
                // Calculate texture V coordinate
                float v_t = (float)(y - yTop) / (float)wallHeight;
                int ty = (int)(v_t * texHeight);
                if (ty < 0) ty = 0;
                if (ty >= texHeight) ty = texHeight - 1;
                ty = texHeight - 1 - ty;  // Flip vertically
                
                int r, g_col, b;
                
                if (texData != NULL) {
                    // Sample from texture
                    int pixelIdx = (ty * texWidth + tx) * 3;
                    r = texData[pixelIdx + 0];
                    g_col = texData[pixelIdx + 1];
                    b = texData[pixelIdx + 2];
                } else {
                    // Fallback: metallic brown/gray color for gate
                    r = 120;
                    g_col = 100;
                    b = 80;
                }
                
                // Apply distance-based shading
                float shade = 1.0f - depth / 600.0f;
                if (shade < 0.25f) shade = 0.25f;
                if (shade > 1.0f) shade = 1.0f;
                
                r = (int)(r * shade);
                g_col = (int)(g_col * shade);
                b = (int)(b * shade);
                
                // Clamp colors
                if (r > 255) r = 255; if (r < 0) r = 0;
                if (g_col > 255) g_col = 255; if (g_col < 0) g_col = 0;
                if (b > 255) b = 255; if (b < 0) b = 0;
                
                pixel(x, y, r, g_col, b);
            }
            
            // Update depth buffer so walls behind don't overdraw
            depthBuffer[x] = depth;
        }
    }
}

#endif // GATES_H
