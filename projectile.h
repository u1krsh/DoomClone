#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "data_types.h"
#include "textures/cace_fire.h"
#include "textures/plasma_proj.h"
#include "textures/BOSSA1_proj.h"
#include "textures/plasma_proj_imp.h"
#include "enemy.h"
#include "lighting.h"

// Types handled in data_types.h
// Constants

// Constants
#define MAX_PROJECTILES 64
#define PLASMA_SPEED 8
#define BULLET_SPEED 16
#define SHELL_SPEED 6 
#define FIREBALL_SPEED 7
#define BOSSA1_SPEED 9

#define PLASMA_DAMAGE 10
#define BULLET_DAMAGE 3
#define SHELL_DAMAGE 2 
#define FIREBALL_DAMAGE 12
#define BOSSA1_DAMAGE 15



// Struct
typedef struct {
    float x, y, z;
    float dx, dy, dz;
    int type;
    int active;
    int damage;
    int lifeTime; // ms
    int spawnTime;
    int isPlayerProjectile; // 1 if fired by player, 0 if fired by enemy
    int lightIndex; // Index of dynamic light attached to this projectile (-1 if none)
} Projectile;

Projectile projectiles[MAX_PROJECTILES];

// Externs
extern int numResult;
extern int numSect;
extern int numWall;
extern walls W[]; 
extern sectors S[];
extern player P;
extern math M;
extern float depthBuffer[];
extern void damagePlayer(int damage, int currentTime);
extern void pixel(int x, int y, int r, int g, int b);
extern DoomTime T; // Access global time struct

// Helper function: Line-segment intersection check
// Returns 1 if line segments (p1->p2) and (p3->p4) intersect
static int proj_lineIntersectsLine(float p1x, float p1y, float p2x, float p2y,
                                    float p3x, float p3y, float p4x, float p4y) {
    float d = (p2x - p1x) * (p4y - p3y) - (p2y - p1y) * (p4x - p3x);
    
    // Parallel lines
    if (d == 0 || d < 0.001f && d > -0.001f) return 0;
    
    float t = ((p3x - p1x) * (p4y - p3y) - (p3y - p1y) * (p4x - p3x)) / d;
    float u = ((p3x - p1x) * (p2y - p1y) - (p3y - p1y) * (p2x - p1x)) / d;
    
    // Check if intersection point is within both line segments (with small epsilon for near-misses)
    float epsilon = 0.1f;
    return (t >= -epsilon && t <= 1.0f + epsilon && u >= -epsilon && u <= 1.0f + epsilon);
}

// Helper function: check collision with line segment using squared distance
static int proj_checkWallCollision(float px, float py, int x1, int y1, int x2, int y2, float radius) {
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    
    float pdx = px - x1;
    float pdy = py - y1;
    
    if (dx == 0 && dy == 0) {
        return (pdx * pdx + pdy * pdy) < (radius * radius);
    }
    
    // Project point onto line (parameter t)
    // t = Dot(P-A, B-A) / |B-A|^2
    float lenSq = dx * dx + dy * dy;
    float t = (pdx * dx + pdy * dy) / lenSq;
    
    // Clamp t to segment [0, 1]
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    
    // Closest point
    float closestX = x1 + t * dx;
    float closestY = y1 + t * dy;
    
    float distSqX = px - closestX;
    float distSqY = py - closestY;
    
    return (distSqX * distSqX + distSqY * distSqY) < (radius * radius);
}

void initProjectiles() {
    for(int i=0; i<MAX_PROJECTILES; i++) projectiles[i].active = 0;
}

void spawnProjectile(float x, float y, float z, float angle, int type, int currentTime) {
    int slot = -1;
    for(int i=0; i<MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return; // Full

    projectiles[slot].active = 1;
    projectiles[slot].x = x;
    projectiles[slot].y = y;
    projectiles[slot].z = z;
    projectiles[slot].type = type;
    projectiles[slot].spawnTime = currentTime;
    projectiles[slot].isPlayerProjectile = 0; // Default to enemy projectile
    projectiles[slot].lightIndex = -1; // No light by default
    
    // printf("DEBUG: Spawning Projectile Type %d at %.2f, %.2f, %.2f (Slot %d)\n", type, x, y, z, slot);
    projectiles[slot].lifeTime = 3000; // 3 seconds default

    float speed = 0;
    if (type == PROJ_TYPE_PLASMA) speed = PLASMA_SPEED;
    else if (type == PROJ_TYPE_BULLET) speed = BULLET_SPEED;
    else if (type == PROJ_TYPE_SHELL) {
         speed = SHELL_SPEED;
         // Spread
         float spread = ((rand()%20) - 10) * 0.005f; 
         angle += spread;
    }
    else if (type == PROJ_TYPE_FIREBALL) speed = FIREBALL_SPEED;
    else if (type == PROJ_TYPE_BOSSA1) speed = BOSSA1_SPEED;

    projectiles[slot].dx = sin(angle) * speed;
    projectiles[slot].dy = cos(angle) * speed;

    // Calculate vertical aim (dz) to hit player's face
    float distX = P.x - x;
    float distY = P.y - y;
    float dist2D = sqrt(distX*distX + distY*distY);
    float timeToHit = dist2D / speed;
    
    if (timeToHit > 0) {
        projectiles[slot].dz = (P.z - z) / timeToHit;
    } else {
        projectiles[slot].dz = 0;
    }

    if (type == PROJ_TYPE_PLASMA) projectiles[slot].damage = PLASMA_DAMAGE;
    else if (type == PROJ_TYPE_BULLET) projectiles[slot].damage = BULLET_DAMAGE;
    else if (type == PROJ_TYPE_SHELL) projectiles[slot].damage = SHELL_DAMAGE;
    else if (type == PROJ_TYPE_FIREBALL) projectiles[slot].damage = FIREBALL_DAMAGE;
    else if (type == PROJ_TYPE_BOSSA1) projectiles[slot].damage = BOSSA1_DAMAGE;
    
    // Create dynamic light for glowing projectiles
    if (type == PROJ_TYPE_PLASMA) {
        // Bright cyan plasma light
        int lightIdx = addLight((int)x, (int)y, (int)z, 120, 200, 50, 255, 255, 0, 3); // LIGHT_TYPE_POINT=0, FLICKER_PULSE=3
        projectiles[slot].lightIndex = lightIdx;
    }
    else if (type == PROJ_TYPE_FIREBALL) {
        // Warm orange/red fireball light with flicker
        int lightIdx = addLight((int)x, (int)y, (int)z, 150, 220, 255, 150, 50, 0, 1); // FLICKER_CANDLE=1
        projectiles[slot].lightIndex = lightIdx;
    }
}

void updateProjectiles(int currentTime) {
    for(int i=0; i<MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;

        if (currentTime - projectiles[i].spawnTime > projectiles[i].lifeTime) {
            // Remove associated light before deactivating
            if (projectiles[i].lightIndex >= 0) {
                removeLight(projectiles[i].lightIndex);
                projectiles[i].lightIndex = -1;
            }
            projectiles[i].active = 0;
            continue;
        }

        float newX = projectiles[i].x + projectiles[i].dx;
        float newY = projectiles[i].y + projectiles[i].dy;
        float newZ = projectiles[i].z + projectiles[i].dz;

        // Wall Collision - check both path and destination
        int collided = 0;
        float radius = (float)PROJ_RADIUS;
        float oldX = projectiles[i].x;
        float oldY = projectiles[i].y;
        
        for (int s = 0; s < numSect; s++) {
            for (int w = S[s].ws; w < S[s].we; w++) {
                // Check 1: Does the movement path intersect the wall?
                if (proj_lineIntersectsLine(oldX, oldY, newX, newY,
                                           (float)W[w].x1, (float)W[w].y1,
                                           (float)W[w].x2, (float)W[w].y2)) {
                    collided = 1;
                    break;
                }
                
                // Check 2: Is the destination too close to the wall?
                if (proj_checkWallCollision(newX, newY, W[w].x1, W[w].y1, W[w].x2, W[w].y2, radius)) {
                    collided = 1;
                    break;
                }
            }
            if (collided) break;
        }

        if (collided) {
            // Spawn visual effect for plasma wall collision using plasma_proj_imp animation
            if (projectiles[i].type == PROJ_TYPE_PLASMA) {
                // Find empty slot for impact effect
                int slot = -1;
                for(int j=0; j<MAX_PROJECTILES; j++) {
                    if (!projectiles[j].active) {
                        slot = j;
                        break;
                    }
                }
                if (slot != -1) {
                    projectiles[slot].active = 1;
                    projectiles[slot].x = projectiles[i].x;
                    projectiles[slot].y = projectiles[i].y;
                    projectiles[slot].z = projectiles[i].z;
                    projectiles[slot].type = PROJ_TYPE_PLASMA_IMP; // Use plasma impact animation
                    projectiles[slot].spawnTime = currentTime;
                    projectiles[slot].lifeTime = 600; // 3 frames * 200ms = 600ms
                    projectiles[slot].dx = 0; // Stationary
                    projectiles[slot].dy = 0;
                    projectiles[slot].dz = 0;
                    projectiles[slot].damage = 0; // No damage
                    projectiles[slot].isPlayerProjectile = 0;
                    projectiles[slot].lightIndex = -1;
                }
            }
            
            // Remove associated light before deactivating
            if (projectiles[i].lightIndex >= 0) {
                removeLight(projectiles[i].lightIndex);
                projectiles[i].lightIndex = -1;
            }
            projectiles[i].active = 0; // Destroy on wall hit
            continue;
        }

        // Player Collision
        // Use P.x, P.y (integers)
        float dx = newX - P.x;
        float dy = newY - P.y;
        float distToPlayer = sqrt(dx*dx + dy*dy);
        
        // Assume player radius 8
        if (distToPlayer < 8 + PROJ_RADIUS) {
            // Remove associated light before deactivating
            if (projectiles[i].lightIndex >= 0) {
                removeLight(projectiles[i].lightIndex);
                projectiles[i].lightIndex = -1;
            }
            damagePlayer(projectiles[i].damage, currentTime);
            projectiles[i].active = 0;
            continue;
        }

        projectiles[i].x = newX;
        projectiles[i].y = newY;
        projectiles[i].z = newZ;
        
        // Update the attached light position to follow the projectile
        if (projectiles[i].lightIndex >= 0 && projectiles[i].lightIndex < MAX_LIGHTS) {
            g_lights[projectiles[i].lightIndex].x = (int)newX;
            g_lights[projectiles[i].lightIndex].y = (int)newY;
            g_lights[projectiles[i].lightIndex].z = (int)newZ;
        }
        
        // Enemy Collision (Simple distance check)
        // Only player projectiles hurt enemies
        if (projectiles[i].isPlayerProjectile) {
             extern int numEnemies;
             extern Enemy enemies[];
             extern void damageEnemy(int, int, int);
             
             for (int e = 0; e < numEnemies; e++) {
                 if (!enemies[e].active || enemies[e].state == 4) continue; // 4 = Dead? Need ENEMY_STATE_DEAD constant or check state
                 
                 float ex = enemies[e].x - newX;
                 float ey = enemies[e].y - newY;
                 // Enemy radius approx 20
                 if (ex*ex + ey*ey < 20*20) {
                     // Remove associated light before deactivating
                     if (projectiles[i].lightIndex >= 0) {
                         removeLight(projectiles[i].lightIndex);
                         projectiles[i].lightIndex = -1;
                     }
                     damageEnemy(e, projectiles[i].damage, currentTime);
                     projectiles[i].active = 0;
                     break; 
                 }
             }
             if (!projectiles[i].active) continue;
        }
    }
}

void drawProjectiles() {
    float CS = M.cos[P.a];
    float SN = M.sin[P.a];
    int currentTime = T.fr1;

    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;

        float relX = projectiles[i].x - P.x;
        float relY = projectiles[i].y - P.y;
        float relZ = projectiles[i].z - P.z;

        float camX = relX * CS - relY * SN;
        float camY = relX * SN + relY * CS;

        if (camY < 1.0f) continue; // Behind camera

        float adjustedZ = relZ + (P.l * camY) / 32.0f;
        
        float screenXf = camX * 200.0f / camY + HSW;
        float screenYf = adjustedZ * 200.0f / camY + HSH;
        
        int screenX = (int)screenXf;
        int screenY = (int)screenYf;
        
        // Size
        float scale = 200.0f / camY;
        int size = (int)(PROJ_RADIUS * 2 * scale);
        
        // Make fireball and plasma impact bigger
        if (projectiles[i].type == PROJ_TYPE_FIREBALL || projectiles[i].type == PROJ_TYPE_PLASMA_IMP) {
            size = (int)(size * 2.5f);
        }
        
        if (size < 2) size = 2;

        int startX = screenX - size/2;
        int endX = screenX + size/2;
        int startY = screenY - size/2;
        int endY = screenY + size/2;

        // Determine texture to use
        const unsigned char* projTexture = NULL;
        int texW = 0, texH = 0;
        
        if (projectiles[i].type == PROJ_TYPE_PLASMA) {
            projTexture = PLASMA_PROJ;
            texW = PLASMA_PROJ_WIDTH;
            texH = PLASMA_PROJ_HEIGHT;
        }
        else if (projectiles[i].type == PROJ_TYPE_BOSSA1) {
            projTexture = BOSSA1_PROJ;
            texW = BOSSA1_PROJ_WIDTH;
            texH = BOSSA1_PROJ_HEIGHT;
        }
        else if (projectiles[i].type == PROJ_TYPE_PLASMA_IMP) {
            // Animated Plasma Impact
            int animTime = currentTime - projectiles[i].spawnTime;
            int frame = (animTime / PLASMA_PROJ_IMP_FRAME_MS) % PLASMA_PROJ_IMP_FRAME_COUNT;
            
            projTexture = PLASMA_PROJ_IMP_frames[frame];
            texW = PLASMA_PROJ_IMP_frame_widths[frame];
            texH = PLASMA_PROJ_IMP_frame_heights[frame];
        }
        else if (projectiles[i].type == PROJ_TYPE_FIREBALL) {
            // Animated Fireball
            int animTime = currentTime - projectiles[i].spawnTime;
            int frame = (animTime / CACE_FIRE_FRAME_MS) % CACE_FIRE_FRAME_COUNT;
            
            projTexture = CACE_FIRE_frames[frame];
            texW = CACE_FIRE_frame_widths[frame];
            texH = CACE_FIRE_frame_heights[frame];
        }

        int pr, pg, pb;
        if (projectiles[i].type == PROJ_TYPE_PLASMA) { pr=0; pg=255; pb=255; } // Cyan fallback
        else if (projectiles[i].type == PROJ_TYPE_BULLET) { pr=255; pg=255; pb=0; }
        else if (projectiles[i].type == PROJ_TYPE_FIREBALL) { pr=255; pg=0; pb=0; }
        else if (projectiles[i].type == PROJ_TYPE_BOSSA1) { pr=255; pg=128; pb=0; } // Orange
        else if (projectiles[i].type == PROJ_TYPE_PLASMA_IMP) { pr=100; pg=200; pb=255; } // Light blue
        else { pr=255; pg=100; pb=0; } // Shell

        for (int y = startY; y < endY; y++) {
             if (y < 0 || y >= SH) continue;
             for (int x = startX; x < endX; x++) {
                 if (x < 0 || x >= SW) continue;
                 
                 // Depth test
                 if (camY < depthBuffer[x]) {
                     if (projTexture) {
                         // Texture mapping
                         int tx = (int)((x - startX) * texW / (float)size);
                         int ty = (int)((y - startY) * texH / (float)size);
                         
                         // Clamp (safety)
                         if (tx < 0) tx = 0; if (tx >= texW) tx = texW - 1;
                         if (ty < 0) ty = 0; if (ty >= texH) ty = texH - 1;
                         
                         // Flip vertically
                         ty = texH - 1 - ty;
                         
                         int idx = (ty * texW + tx) * 3;
                          int r = projTexture[idx];
                          int g = projTexture[idx+1];
                          int b = projTexture[idx+2];
                          
                          // Transparency check (1,0,0) or pure black (0,0,0)
                          if ((r == 1 && g == 0 && b == 0) || (r == 0 && g == 0 && b == 0)) continue;
                          
                          pixel(x, y, r, g, b);
                     } else {
                         pixel(x, y, pr, pg, pb);
                     }
                 }
             }
        }
    }
}

#endif // PROJECTILE_H
