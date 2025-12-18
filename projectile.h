#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "data_types.h"
#include "textures/cace_fire.h"

// Types
#define PROJ_TYPE_PLASMA 0
#define PROJ_TYPE_BULLET 1
#define PROJ_TYPE_SHELL 2
#define PROJ_TYPE_FIREBALL 3

// Constants
#define MAX_PROJECTILES 64
#define PLASMA_SPEED 8
#define BULLET_SPEED 16
#define SHELL_SPEED 6 
#define FIREBALL_SPEED 7

#define PLASMA_DAMAGE 10
#define BULLET_DAMAGE 3
#define SHELL_DAMAGE 2 
#define FIREBALL_DAMAGE 12

#define PROJ_RADIUS 4

// Struct
typedef struct {
    float x, y, z;
    float dx, dy, dz;
    int type;
    int active;
    int damage;
    int lifeTime; // ms
    int spawnTime;
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

// Helper function: Distance from point to line segment
static float proj_pointToLineDistance(int px, int py, int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    
    if (dx == 0 && dy == 0) {
        int ddx = px - x1;
        int ddy = py - y1;
        return sqrt((float)(ddx * ddx + ddy * ddy));
    }
    
    float t = ((px - x1) * dx + (py - y1) * dy) / (float)(dx * dx + dy * dy);
    
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    float closestX = x1 + t * dx;
    float closestY = y1 + t * dy;
    
    float distX = px - closestX;
    float distY = py - closestY;
    return sqrt(distX * distX + distY * distY);
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
}

void updateProjectiles(int currentTime) {
    for(int i=0; i<MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;

        if (currentTime - projectiles[i].spawnTime > projectiles[i].lifeTime) {
            projectiles[i].active = 0;
            continue;
        }

        float newX = projectiles[i].x + projectiles[i].dx;
        float newY = projectiles[i].y + projectiles[i].dy;
        float newZ = projectiles[i].z + projectiles[i].dz;

        // Wall Collision
        int collided = 0;
        for (int s = 0; s < numSect; s++) {
            for (int w = S[s].ws; w < S[s].we; w++) {
                float dist = proj_pointToLineDistance((int)newX, (int)newY, W[w].x1, W[w].y1, W[w].x2, W[w].y2);
                if (dist < PROJ_RADIUS) {
                    collided = 1;
                    break;
                }
            }
            if (collided) break;
        }

        if (collided) {
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
            damagePlayer(projectiles[i].damage, currentTime);
            projectiles[i].active = 0;
            continue;
        }

        projectiles[i].x = newX;
        projectiles[i].y = newY;
        projectiles[i].z = newZ;
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
        
        // Make fireball slightly bigger
        if (projectiles[i].type == PROJ_TYPE_FIREBALL) size = (int)(size * 2.5f);
        
        if (size < 2) size = 2;

        int startX = screenX - size/2;
        int endX = screenX + size/2;
        int startY = screenY - size/2;
        int endY = screenY + size/2;

        // Determine texture to use
        const unsigned char* projTexture = NULL;
        int texW = 0, texH = 0;
        
        if (projectiles[i].type == PROJ_TYPE_PLASMA) { // BOSSA1
            projTexture = BOSSA1_PROJ;
            texW = BOSSA1_PROJ_WIDTH;
            texH = BOSSA1_PROJ_HEIGHT;
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
        if (projectiles[i].type == PROJ_TYPE_PLASMA) { pr=0; pg=255; pb=0; }
        else if (projectiles[i].type == PROJ_TYPE_BULLET) { pr=255; pg=255; pb=0; }
        else if (projectiles[i].type == PROJ_TYPE_FIREBALL) { pr=255; pg=0; pb=0; }
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
                         
                         // Transparency check (1,0,0)
                         if (r == 1 && g == 0 && b == 0) continue;
                         
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
