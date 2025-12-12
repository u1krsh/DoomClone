#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>

#define res 1 //resolution scale
#define SH 240*res //screen height
#define SW 320*res //screen width
#define HSH SH/2 //half screen height
#define HSW SW/2 //half screen width
#define pixelScale  4/res //open gl pixel size
#define GSLW SW*pixelScale //open gl window width
#define GSLH SH*pixelScale //open gl window height
#define SCRPOS_H GSLH / 8 
#define SCRPOS_W GSLW / 5
#define M_PI 3.14159265358979323846  /* pi */

// Consolidated texture includes
#include "textures/all_textures.h"

// Console module
#include "console.h"

// Screen melt module
#include "screen_melt.h"

// FPS counter module
#include "fps_counter.h"

// Automap module
#include "automap.h"

// Enemy system
#include "enemy.h"

// HUD system
#include "hud.h"

// Weapon system
#include "weapon.h"

// Effects system (screen shake, particles, kill streaks)
#include "effects.h"

// Pickup system (health, armor, powerups)
#include "pickups.h"

// Sound system
#include "sound.h"

// Global pause state
int gamePaused = 0;
int hasBlueKey = 0; // Inventory flag

int numText = NUM_TEXTURES - 1;  // Use macro from all_textures.h (max texture index)

int numSect = 0;                          //number of sectors
int numWall = 0;                          //number of walls

// Collision detection constants
#define PLAYER_RADIUS 8  // Player collision radius


typedef struct {
	int fr1, fr2; // these are frame 1 and 2 for a constant frame rate
}time;
time T;

typedef struct
{
	int w, s, a, d;           //move up, down, left, rigth
	int sl, sr;             //strafe left, right 
	int m;                 //move up, down, look up, down
	int fire;              //fire weapon
	int firePressed;       //track if fire was already pressed (for single shot)
}keys;
keys K;

typedef struct {
	float cos[360];   //cos and sin values
	float sin[360];

}math;
math M;

typedef struct {
	int x, y, z; //position
	int a;// angle of rotation
	int l; // variable to look up and down
}player;
player P;

typedef struct {
	int x1, y1;//bottom line point 1
	int x2, y2;//bootom line point 2
	int c;// wall color 
	int wt, u, v; //wall texture and u/v tile
	int shade;             //shade of the wall
}walls;
walls W[256];

typedef struct {
	int ws, we; // wall number start and end
	int z1, z2; // height from bottom to top
	int x, y; // center of sector
	int d; // sorting drawing order
	int c1, c2; //bottom and top color
	int surf[SW]; // to gold points for surface
	int surface; //is there a surface to draw
	int ss, st; //surface texture, surface scale

}sectors;
sectors S[128];

typedef struct {
	int w, h;                             //texture width/height
	const unsigned char* name;           //texture name
}TextureMaps; TextureMaps Textures[64]; //increase for more textures  

// Depth buffer for sprite rendering
float depthBuffer[SW];

// Forward declarations
void drawEnemies();
void drawConsoleText();
void drawWallDebugOverlay();
void drawPauseMenu();
int checkWallCollision(int newX, int newY);
void toggleMouseLook();  // Add forward declaration for mouse look toggle

// Mouse look state (needs to be declared before mouseClick uses it)
int lastMouseX = -1;
int mouseEnabled = 0;

void load()
{
	FILE* fp = fopen("level.h", "r");
	if (fp == NULL) { printf("Error opening level.h"); return; }
	int s, w;

	fscanf(fp, "%i", &numSect);   //number of sectors 
	for (s = 0; s < numSect; s++)      //load all sectors
	{
		fscanf(fp, "%i", &S[s].ws);
		fscanf(fp, "%i", &S[s].we);
		fscanf(fp, "%i", &S[s].z1);
		fscanf(fp, "%i", &S[s].z2);
		fscanf(fp, "%i", &S[s].st);
		fscanf(fp, "%i", &S[s].ss);
	}
	fscanf(fp, "%i", &numWall);   //number of walls 
	for (s = 0; s < numWall; s++)      //load all walls
	{
		fscanf(fp, "%i", &W[s].x1);
		fscanf(fp, "%i", &W[s].y1);
		fscanf(fp, "%i", &W[s].x2);
		fscanf(fp, "%i", &W[s].y2);
		fscanf(fp, "%i", &W[s].wt);
		fscanf(fp, "%i", &W[s].u);
		fscanf(fp, "%i", &W[s].v);
		fscanf(fp, "%i", &W[s].shade);
	}
	fscanf(fp, "%i %i %i %i %i", &P.x, &P.y, &P.z, &P.a, &P.l); //player position, angle, look direction 

	// Load enemies
	int num_loaded_enemies = 0;
	if (fscanf(fp, "%i", &num_loaded_enemies) != EOF) {
		initEnemies(); // Reset enemies
		if (num_loaded_enemies > MAX_ENEMIES) num_loaded_enemies = MAX_ENEMIES;
		numEnemies = num_loaded_enemies;

		for (s = 0; s < numEnemies; s++)
		{
			int type;
			fscanf(fp, "%i %i %i %i", &enemies[s].x, &enemies[s].y, &enemies[s].z, &type);
			enemies[s].enemyType = type;
			enemies[s].active = 1;
			enemies[s].state = 0; // Idle
			enemies[s].animFrame = 0;
			enemies[s].lastAnimTime = 0;
		}
	}

	// Load pickups from level file
	int num_loaded_pickups = 0;
	if (fscanf(fp, "%i", &num_loaded_pickups) != EOF) {
		initPickups(); // Reset pickups
		
		for (s = 0; s < num_loaded_pickups && s < MAX_PICKUPS; s++)
		{
			int x, y, z, type, respawns;
			if (fscanf(fp, "%i %i %i %i %i", &x, &y, &z, &type, &respawns) == 5) {
				addPickup(type, x, y, z, respawns);
			}
		}
	}

	fclose(fp);
}

void pixel(int x, int y, int r, int g, int b) { //draws pixel at x,y with color c
	glColor3ub(r, g, b);
	glBegin(GL_POINTS);
	glVertex2i(x * pixelScale + 2, y * pixelScale + 2);
	glEnd();
}

void movePl()
{
	// Don't move if console is active or game is paused
	if (console.active || gamePaused) return;

	//move up, down, left, right
	if (K.a == 1 && K.m == 0) { P.a -= 4; if (P.a < 0) { P.a += 360; } }
	if (K.d == 1 && K.m == 0) { P.a += 4; if (P.a > 359) { P.a -= 360; } }
	
	// Apply speed multiplier from powerups
	float speedMult = getSpeedMultiplier(T.fr1);
	int dx = (int)(M.sin[P.a] * 10.0 * speedMult);
	int dy = (int)(M.cos[P.a] * 10.0 * speedMult);
	
	// Store old position for collision rollback
	int oldX = P.x;
	int oldY = P.y;
	int newX = P.x;
	int newY = P.y;
	
	// Track if player is moving for head bob
	int isMoving = 0;
	
	// Calculate new position based on input
	if (K.w == 1 && K.m == 0) { newX += dx; newY += dy; isMoving = 1; }
	if (K.s == 1 && K.m == 0) { newX -= dx; newY -= dy; isMoving = 1; }
	
	//strafe left, right
	if (K.sr == 1) { newX += dy; newY -= dx; isMoving = 1; }
	if (K.sl == 1) { newX -= dy; newY += dx; isMoving = 1; }
	
	// Update head bobbing
	updateHeadBob(isMoving, T.fr1);
	
	// Only update position if no collision (and not in godMode or noclip)
	if (!godMode && !noclip) {
		if (!checkWallCollision(newX, newY)) {
			P.x = newX;
			P.y = newY;
		}
		else {
			// Try sliding along walls by testing X and Y movement separately
			if (!checkWallCollision(newX, oldY)) {
				P.x = newX; // Can move in X direction
			}
			else if (!checkWallCollision(oldX, newY)) {
				P.y = newY; // Can move in Y direction
			}
			// else: blocked completely, don't move
		}
	}
	else {
		// GodMode or noclip: no collision
		P.x = newX;
		P.y = newY;
	}
	
	//move up, down, look up, look down (only if godMode is enabled)
	if (godMode) {
		if (K.a == 1 && K.m == 1) { P.l -= 1; }
		if (K.d == 1 && K.m == 1) { P.l += 1; }
		if (K.w == 1 && K.m == 1) { P.z -= 4; }
		if (K.s == 1 && K.m == 1) { P.z += 4; }
	}
	else {
		// In non-godMode, allow look up/down but not flying
		if (K.a == 1 && K.m == 1) { P.l -= 1; }
		if (K.d == 1 && K.m == 1) { P.l += 1; }
	}
}

// Helper function: Check if point is on the left side of a line
int isOnLeftSide(int px, int py, int x1, int y1, int x2, int y2) {
	return ((x2 - x1) * (py - y1) - (y2 - y1) * (px - x1)) > 0;
}

// Helper function: Distance from point to line segment
float pointToLineDistance(int px, int py, int x1, int y1, int x2, int y2) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	
	if (dx == 0 && dy == 0) {
		// Line segment is actually a point
		int ddx = px - x1;
		int ddy = py - y1;
		return sqrt(ddx * ddx + ddy * ddy);
	}
	
	// Calculate projection factor
	float t = ((px - x1) * dx + (py - y1) * dy) / (float)(dx * dx + dy * dy);
	
	// Clamp t to [0, 1] to stay within line segment
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	
	// Calculate closest point on line segment
	float closestX = x1 + t * dx;
	float closestY = y1 + t * dy;
	
	// Calculate distance
	float distX = px - closestX;
	float distY = py - closestY;
	return sqrt(distX * distX + distY * distY);
}

// Check if moving from (x1,y1) collides with any wall
int checkWallCollision(int newX, int newY) {
	int s, w;
	
	// Check against all walls in all sectors
	for (s = 0; s < numSect; s++) {
		for (w = S[s].ws; w < S[s].we; w++) {
			// Calculate distance from new position to this wall
			float distance = pointToLineDistance(newX, newY, W[w].x1, W[w].y1, W[w].x2, W[w].y2);
			
			// If distance is less than player radius, collision detected
			if (distance < PLAYER_RADIUS) {
				return 1; // Collision detected
			}
		}
	}
	
	return 0; // No collision
}

void clearBackground() {
	int x, y;
	for (y = 0; y < SH; y++) {
		for (x = 0; x < SW; x++) {
			pixel(x, y, 0, 60, 130); //clear background 
		}
	}
}

void clipBehindPlayer(int* x1, int* y1, int* z1, int x2, int y2, int z2, float* u1, float u2) {
	float da = *y1;
	float db = y2;
	float d = da - db; if (da == 0) { d = 1; }
	float s = da / (da - db);// intesection factor 
	*x1 = *x1 + s * (x2 - (*x1));
	*y1 = *y1 + s * (y2 - (*y1)); if (*y1 == 0) { *y1 = 1; }// prevents divide by 0
	*z1 = *z1 + s * (z2 - (*z1));
    if (u1 != NULL) {
        *u1 = *u1 + s * (u2 - (*u1));
    }
}


int dist(int x1, int y1, int x2, int y2) {
	int distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	return distance;
}

// Updated drawWall with perspective correct texture mapping and texture clipping
void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int s, int w, int frontBack, float d1, float d2, float s0, float s1) {
	int wt = W[w].wt;

	int x, y;
	// hold difference between bottom and top
	int dyb = b2 - b1; // y distance from bottom line 
	int dyt = t2 - t1; // top line
	int dx = x2 - x1; // x distance
	if (dx == 0) { dx = 1; } // prevent divide by zero
	int xs = x1; //hold initial x1 staring position

	// Store original x1, x2 for texture coordinate calculation before clipping
	int x1_orig = x1;
	int x2_orig = x2;

    // Perspective correction preparation
    float iz1 = 1.0f / d1; // 1/z at left edge
    float iz2 = 1.0f / d2; // 1/z at right edge
    
    // Get texture width first to calculate max u
    int texWidth = 0, texHeight = 0;
	const unsigned char* texData = NULL;

	#if defined(WALL57_ANIM_AVAILABLE) && WALL57_ANIM_AVAILABLE == 1
	if (wt == NUM_TEXTURES - 2) {
		int t = glutGet(GLUT_ELAPSED_TIME);
		int frame = (t / WALL57_FRAME_MS) % WALL57_FRAME_COUNT;
		texWidth = WALL57_FRAME_WIDTH;
		texHeight = WALL57_FRAME_HEIGHT;
		texData = WALL57_frames[frame];
	}
	else
	#endif
	#if defined(WALL58_ANIM_AVAILABLE) && WALL58_ANIM_AVAILABLE == 1
	if (wt == NUM_TEXTURES - 1) {
		int t = glutGet(GLUT_ELAPSED_TIME);
		int frame = (t / 150) % WALL58_FRAME_COUNT;
		texWidth = WALL58_FRAME_WIDTH;
		texHeight = WALL58_FRAME_HEIGHT;
		if (frame == 0) texData = WALL58_frame_0;
		else if (frame == 1) texData = WALL58_frame_1;
		else texData = WALL58_frame_2;
	}
	else
	#endif
	{
		texWidth = Textures[wt].w;
		texHeight = Textures[wt].h;
		texData = Textures[wt].name;
	}

    float total_u = texWidth * W[w].u;
    float uz1 = (s0 * total_u) * iz1; // u/z at left
    float uz2 = (s1 * total_u) * iz2; // u/z at right

	// Calculate dynamic shading based on wall angle relative to player view
	float dx_wall = (float)(W[w].x2 - W[w].x1);
	float dy_wall = (float)(W[w].y2 - W[w].y1);
	float wallAngle = atan2f(dy_wall, dx_wall) * 57.2958f; 
	if (wallAngle < 0) { wallAngle += 360.0f; }
	
	float wallNormal = wallAngle + 90.0f;
	if (wallNormal >= 360.0f) { wallNormal -= 360.0f; }
	
	float angleDiff = wallNormal - (float)P.a;
	while (angleDiff > 180.0f) { angleDiff -= 360.0f; }
	while (angleDiff < -180.0f) { angleDiff += 360.0f; }
	if (angleDiff < 0) { angleDiff = -angleDiff; }
	
	float shadeFactor = angleDiff / 180.0f;
	shadeFactor = shadeFactor * shadeFactor;
	int dynamicShade = (int)(shadeFactor * 90.0f);
	if (dynamicShade < 0) { dynamicShade = 0; }
	if (dynamicShade > 90) { dynamicShade = 90; }

	//clipping x
	if (x1 < 0) { x1 = 0; }
	if (x2 < 0) { x2 = 0; }
	if (x1 > SW) { x1 = SW; }
	if (x2 > SW) { x2 = SW; }

	//draw x vertices line
	for (x = x1; x < x2; x++) {
		int y1 = dyb * (x - xs + 0.5) / dx + b1; //y bottom point 
		int y2 = dyt * (x - xs + 0.5) / dx + t1;

        // Perspective Correct Interpolation
        // Calculate t (0.0 to 1.0) across the ORIGINAL wall span
        float t_step = (float)(x - x1_orig) / (float)(x2_orig - x1_orig);
        
        // Interpolate 1/z
        float iz = iz1 + (iz2 - iz1) * t_step;
        
        // Interpolate u/z
        float uz = uz1 + (uz2 - uz1) * t_step;
        
        // Recover u
        float ht = uz / iz;

		// Clamp ht to prevent out-of-bounds access
		if (ht < 0) ht = 0;
		if (ht >= total_u) ht = total_u - 0.001f;
		
		int tx = ((int)ht) % texWidth;

		// Store original y1, y2 for texture coordinate calculation before clipping
		int y1_orig = y1;
		int y2_orig = y2;

		//clipping y
		if (y1 < 0) { y1 = 0; }
		if (y2 < 0) { y2 = 0; }
		if (y1 > SH) { y1 = SH; }
		if (y2 > SH) { y2 = SH; }

		//draw front wall
		if (frontBack == 0) {
			if (S[s].surface == 1) { S[s].surf[x] = y1; } //bottom surface save top row
			if (S[s].surface == 2) { S[s].surf[x] = y2; }

			int wall_height = y2_orig - y1_orig;
			if (wall_height <= 0) continue;

			for (y = y1; y < y2; y++) {
				// Vertical perspective correction?
                // Vertical is usually linear in screen space for walls (constant Z per vertical slice)
                // So the original affine interpolation for y is actually correct for vertical mapping
                // because for a vertical wall strip, depth is constant!
				float vt = ((float)(y - y1_orig) / (float)wall_height) * texHeight * W[w].v;
				
				if (vt < 0) vt = 0;
				if (vt >= texHeight * W[w].v) vt = texHeight * W[w].v - 0.001f;
				
				int ty = ((int)vt) % texHeight;

				if (tx < 0) tx = 0;
				if (tx >= texWidth) tx = texWidth - 1;
				if (ty < 0) ty = 0;
				if (ty >= texHeight) ty = texHeight - 1;

				int pixelN = (texHeight - ty - 1) * 3 * texWidth + tx * 3;
				
				// Bounds check for pixel index
				int maxPixel = texWidth * texHeight * 3;
				if (pixelN >= 0 && pixelN + 2 < maxPixel) {
					// Use dynamic shade instead of static W[w].shade
					int r = texData[pixelN + 0] - dynamicShade; if (r < 0) { r = 0; }
					int g = texData[pixelN + 1] - dynamicShade; if (g < 0) { g = 0; }
					int b = texData[pixelN + 2] - dynamicShade; if (b < 0) { b = 0; }
					pixel(x, y, r, g, b);
				}
			}
		}
		if (frontBack == 1) {
			if (S[s].surface == 1) { y2 = S[s].surf[x]; } //bottom surface save top row
			if (S[s].surface == 2) { y1 = S[s].surf[x]; }
			for (y = y1; y < y2; y++) { pixel(x, y, 255, 0, 0); } //normal wall
		}
	}
}

void draw3D() {
	int wx[4], wy[4], wz[4];
	float CS = M.cos[P.a];
	float SN = M.sin[P.a];
	int s, w, frontBack, cycles, x;
	
	// Initialize depth buffer
	for (x = 0; x < SW; x++) {
		depthBuffer[x] = 99999.0f; // Far distance
	}
	
	//order sector by distance
	for (s = 0; s < numSect - 1; s++) {
		for (w = 0; w < numSect - s - 1; w++) {
			if (S[w].d < S[w + 1].d) {
				sectors st = S[w];
				S[w] = S[w + 1];
				S[w + 1] = st;
			}
		}
	}

	//draw sectors
	for (s = 0; s < numSect; s++) {
		S[s].d = 0;
		if (P.z < S[s].z1) { S[s].surface = 1; cycles = 2; for (x = 0; x < SW; x++) { S[s].surf[x] = SH; } }
		else if (P.z > S[s].z2) { S[s].surface = 2; cycles = 2; for (x = 0; x < SW; x++) { S[s].surf[x] = 0; } }
		else { S[s].surface = 0; cycles = 1; }

		for (frontBack = 0; frontBack < cycles; frontBack++) {
			for (w = S[s].ws; w < S[s].we; w++) {
				int x1 = W[w].x1 - P.x, y1 = W[w].y1 - P.y;
				int x2 = W[w].x2 - P.x, y2 = W[w].y2 - P.y;

				if (frontBack == 1) { int swp = x1; x1 = x2; x2 = swp; swp = y1; y1 = y2; y2 = swp; }

				wx[0] = x1 * CS - y1 * SN;
				wx[1] = x2 * CS - y2 * SN;
				wx[2] = wx[0];
				wx[3] = wx[1];

				wy[0] = x1 * SN + y1 * CS;
				wy[1] = x2 * SN + y2 * CS;
				wy[2] = wy[0];
				wy[3] = wy[1];

				S[s].d = dist(0, 0, (wx[0] + wx[1]) / 2, (wy[0] + wy[1]) / 2);

				wz[0] = S[s].z1 - P.z + ((P.l * wy[0]) / 32.0);
				wz[1] = S[s].z1 - P.z + ((P.l * wy[1]) / 32.0);
				wz[2] = S[s].z2 - P.z + ((P.l * wy[0]) / 32.0);
				wz[3] = S[s].z2 - P.z + ((P.l * wy[1]) / 32.0);

				if (wy[0] < 1 && wy[1] < 1) { continue; }

                // Texture coefficients (0.0 to 1.0)
                float u0 = 0.0f; // Start U ratio
                float u1 = 1.0f; // End U ratio
                if (frontBack == 1) { // If backface swapped
                    u0 = 1.0f; u1 = 0.0f;
                }

				if (wy[0] < 1) {
					clipBehindPlayer(&wx[0], &wy[0], &wz[0], wx[1], wy[1], wz[1], &u0, u1);
					clipBehindPlayer(&wx[2], &wy[2], &wz[2], wx[3], wy[3], wz[3], NULL, 0); // Don't need to update U again
				}

				if (wy[1] < 1) {
					clipBehindPlayer(&wx[1], &wy[1], &wz[1], wx[0], wy[0], wz[0], &u1, u0);
					clipBehindPlayer(&wx[3], &wy[3], &wz[3], wx[2], wy[2], wz[2], NULL, 0);
				}

				// Store depth information
				float depth0 = wy[0];
				float depth1 = wy[1];

				wx[0] = wx[0] * 200 / wy[0] + HSW;
				wy[0] = wz[0] * 200 / wy[0] + HSH;
				wx[1] = wx[1] * 200 / wy[1] + HSW; wy[1] = wz[1] * 200 / wy[1] + HSH;
				wx[2] = wx[2] * 200 / wy[2] + HSW; wy[2] = wz[2] * 200 / wy[2] + HSH;
				wx[3] = wx[3] * 200 / wy[3] + HSW; wy[3] = wz[3] * 200 / wy[3] + HSH;

				// Update depth buffer for this wall segment
				int startX = wx[0] < wx[1] ? wx[0] : wx[1];
				int endX = wx[0] > wx[1] ? wx[0] : wx[1];
				if (startX < 0) startX = 0;
				if (endX >= SW) endX = SW - 1;
				
				for (x = startX; x <= endX; x++) {
					float t = (endX - startX) > 0 ? (float)(x - startX) / (float)(endX - startX) : 0.0f;
					float depth = depth0 + (depth1 - depth0) * t;
					if (depth < depthBuffer[x]) {
						depthBuffer[x] = depth;
					}
				}


				drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], s, w, frontBack, depth0, depth1, u0, u1);
			}
			// Prevent division by zero for sectors with no walls
			int wallCount = S[s].we - S[s].ws;
			if (wallCount > 0) {
				S[s].d /= wallCount;
			}
		}
	}
	
	// FIXED: Draw enemy debug wireframes BEFORE drawing enemy sprites
	// This way, enemy sprites will occlude the wireframes properly
	if (isFPSDisplayEnabled()) {
		drawEnemyDebugOverlay(pixel, SW, SH, P.x, P.y, P.z, P.a, M.cos, M.sin, depthBuffer);
	}
	
	// Draw enemies as sprites (after walls and debug wireframes)
	drawEnemies();
}

// Draw enemies as billboarded sprites (supports multiple enemy types)
void drawEnemies() {
	if (!enemiesEnabled) return;
	
	int i;
	float CS = M.cos[P.a];
	float SN = M.sin[P.a];
	
	// Create array to sort enemies by distance
	typedef struct {
		int index;
		float distance;
	} EnemySort;
	
	EnemySort sortedEnemies[MAX_ENEMIES];
	int sortCount = 0;
	
	// Calculate distances and prepare for sorting
	for (i = 0; i < numEnemies; i++) {
		if (!enemies[i].active) continue;
		
		int relX = enemies[i].x - P.x;
		int relY = enemies[i].y - P.y;
		
		float distance = sqrt(relX * relX + relY * relY);
		sortedEnemies[sortCount].index = i;
		sortedEnemies[sortCount].distance = distance;
		sortCount++;
	}
	
	// Sort enemies by distance (furthest first) using bubble sort
	for (i = 0; i < sortCount - 1; i++) {
		for (int j = 0; j < sortCount - i - 1; j++) {
			if (sortedEnemies[j].distance < sortedEnemies[j + 1].distance) {
				EnemySort temp = sortedEnemies[j];
				sortedEnemies[j] = sortedEnemies[j + 1];
				sortedEnemies[j + 1] = temp;
			}
		}
	}
	
	// Draw enemies from furthest to closest
	for (int enemyIdx = 0; enemyIdx < sortCount; enemyIdx++) {
		i = sortedEnemies[enemyIdx].index;
		
		// Get sprite data based on enemy type
		const unsigned char* spriteData = NULL;
		int frameWidth = 0;
		int frameHeight = 0;
		int currentFrame = enemies[i].animFrame;
		
		// Select sprite based on enemy type
		switch (enemies[i].enemyType) {
			case ENEMY_TYPE_BOSSA1:
				currentFrame = currentFrame % BOSSA1_FRAME_COUNT;
				frameWidth = BOSSA1_frame_widths[currentFrame];
				frameHeight = BOSSA1_frame_heights[currentFrame];
				if (currentFrame == 0) spriteData = BOSSA1_frame_0;
				else if (currentFrame == 1) spriteData = BOSSA1_frame_1;
				else if (currentFrame == 2) spriteData = BOSSA1_frame_2;
				else spriteData = BOSSA1_frame_3;
				break;
				
			case ENEMY_TYPE_BOSSA2:
				currentFrame = currentFrame % BOSSA2_FRAME_COUNT;
				frameWidth = BOSSA2_frame_widths[currentFrame];
				frameHeight = BOSSA2_frame_heights[currentFrame];
				if (currentFrame == 0) spriteData = BOSSA2_frame_0;
				else if (currentFrame == 1) spriteData = BOSSA2_frame_1;
				else spriteData = BOSSA2_frame_2;
				break;
				
			case ENEMY_TYPE_BOSSA3:
				currentFrame = currentFrame % BOSSA3_FRAME_COUNT;
				frameWidth = BOSSA3_frame_widths[currentFrame];
				frameHeight = BOSSA3_frame_heights[currentFrame];
				if (currentFrame == 0) spriteData = BOSSA3_frame_0;
				else if (currentFrame == 1) spriteData = BOSSA3_frame_1;
				else if (currentFrame == 2) spriteData = BOSSA3_frame_2;
				else spriteData = BOSSA3_frame_3;
				break;
				
			default:
				// Fallback to BOSSA1
				currentFrame = 0;
				frameWidth = BOSSA1_frame_widths[0];
				frameHeight = BOSSA1_frame_heights[0];
				spriteData = BOSSA1_frame_0;
				break;
		}
		
		if (spriteData == NULL) continue;
		
		// Transform enemy position to camera space
		float relX = (float)(enemies[i].x - P.x);
		float relY = (float)(enemies[i].y - P.y);
		float relZ = (float)(enemies[i].z - P.z);
		
		// Rotate to camera space
		float camX = relX * CS - relY * SN;
		float camY = relX * SN + relY * CS;
		
		// Skip if behind player
		if (camY < 1.0f) continue;
		
		// Apply look up/down adjustment to Z position
		float adjustedZ = relZ + (P.l * camY) / 32.0f;
		
		// Project to screen
		float screenXf = camX * 200.0f / camY + HSW;
		float screenYf = adjustedZ * 200.0f / camY + HSH;
		
		int screenX = (int)(screenXf);
		int screenY = (int)(screenYf);
		
		// Calculate sprite size based on distance
		float scale = 200.0f / camY;
		int spriteHeight = (int)(frameHeight * scale);
		int spriteWidth = (int)(frameWidth * scale);
		
		// Ensure minimum size
		if (spriteWidth < 1) spriteWidth = 1;
		if (spriteHeight < 1) spriteHeight = 1;
		
		int x, y;
		int halfW = spriteWidth / 2;
		int halfH = spriteHeight / 2;
		
		// Calculate bounds
		int startY = screenY - halfH;
		int endY = screenY + halfH;
		int startX = screenX - halfW;
		int endX = screenX + halfW;
		
		// Draw sprite with depth testing
		for (y = startY; y < endY; y++) {
			if (y < 0 || y >= SH) continue;
			
			for (x = startX; x < endX; x++) {
				if (x < 0 || x >= SW) continue;
				
				// Depth test - only draw if sprite is closer than wall
				if (camY > depthBuffer[x]) continue;
				
				// Calculate texture coordinates
				float u = (float)(x - startX) / (float)spriteWidth;
				float v = (float)(y - startY) / (float)spriteHeight;
				
				// Clamp UV coordinates
				if (u < 0.0f) u = 0.0f;
				if (u >= 1.0f) u = 0.999f;
				if (v < 0.0f) v = 0.0f;
				if (v >= 1.0f) v = 0.999f;
				
				// Convert UV to texture pixel coordinates
				int tx = (int)(u * frameWidth);
				int ty = (int)(v * frameHeight);
				
				// Flip vertically
				ty = frameHeight - 1 - ty;
				
				// Clamp texture coordinates
				if (tx < 0) tx = 0;
				if (tx >= frameWidth) tx = frameWidth - 1;
				if (ty < 0) ty = 0;
				if (ty >= frameHeight) ty = frameHeight - 1;
				
				// Get pixel from sprite
				int pixelIndex = (ty * frameWidth + tx) * 3;
				
				// Bounds check
				if (pixelIndex < 0 || pixelIndex + 2 >= frameWidth * frameHeight * 3) continue;
				
				// Read RGB values from current frame
				int r = (unsigned char)spriteData[pixelIndex + 0];
				int g = (unsigned char)spriteData[pixelIndex + 1];
				int b = (unsigned char)spriteData[pixelIndex + 2];
				
				// Skip transparent pixels
				if (r == 1 && g == 0 && b == 0) continue;
				
				// Apply distance-based shading
				float shadeFactor = 1.0f - (camY / 800.0f);
				if (shadeFactor < 0.3f) shadeFactor = 0.3f;
				if (shadeFactor > 1.0f) shadeFactor = 1.0f;
				
				r = (int)(r * shadeFactor);
				g = (int)(g * shadeFactor);
				b = (int)(b * shadeFactor);
				
				// Clamp final values
				if (r > 255) r = 255; if (r < 0) r = 0;
				if (g > 255) g = 255; if (g < 0) g = 0;
				if (b > 255) b = 255; if (b < 0) b = 0;
				
				pixel(x, y, r, g, b);
				
				// Update depth buffer
				depthBuffer[x] = camY;
			}
		}
	}
}

void drawConsoleText() {
	if (console.slidePos <= 0.0f) return;
	
	int consoleHeight = (int)(SH * CONSOLE_HEIGHT_PERCENT * console.slidePos);
	int x, y;
	
	// Draw console background
	for (y = SH - consoleHeight; y < SH; y++) {
		for (x = 0; x < SW; x++) {
			pixel(x, y, 0, 0, 0);
		}
	}
	
	// Draw console border
	for (x = 0; x < SW; x++) {
		pixel(x, SH - consoleHeight, 255, 255, 0);
	}
	
	int fontScale = 1;
	if (SH >= 480) fontScale = 2;
	if (SH >= 720) fontScale = 3;
	if (SH >= 1080) fontScale = 4;
	
	int lineHeight = 10 * fontScale;
	
	// Draw message history
	int messageY = SH - consoleHeight + (5 * fontScale);
	for (int i = 0; i < console.messageCount && i < CONSOLE_MESSAGE_LINES; i++) {
		if (console.messages[i][0] != '\0') {
			drawStringScaled(5 * fontScale, messageY, console.messages[i], 255, 255, 255, fontScale, pixel);
			messageY += lineHeight;
		}
	}
	
	// Draw prompt and input
	int textY = SH - lineHeight;
	int textX = 5 * fontScale;
	
	drawCharScaled(textX, textY, '>', 255, 255, 0, fontScale, pixel);
	textX += (10 * fontScale);
	drawStringScaled(textX, textY, console.input, 255, 255, 255, fontScale, pixel);
	
	// Draw cursor
	int cursorX = textX + (console.inputPos * 8 * fontScale);
	if ((T.fr1 / 500) % 2 == 0) {
		drawCharScaled(cursorX, textY, '_', 255, 255, 0, fontScale, pixel);
	}
}

// Draw pause menu overlay
void drawPauseMenu() {
	if (!gamePaused) return;
	
	int x, y;
	
	// Draw semi-transparent overlay
	for (y = 0; y < SH; y++) {
		for (x = 0; x < SW; x++) {
			// Draw every other pixel for transparency effect
			if ((x + y) % 2 == 0) {
				pixel(x, y, 0, 0, 0);
			}
		}
	}
	
	// Calculate font scale based on screen height
	int fontScale = 2;
	if (SH >= 480) fontScale = 3;
	if (SH >= 720) fontScale = 4;
	if (SH >= 1080) fontScale = 6;
	
	// "GAME PAUSED" text
	const char* pauseText = "GAME PAUSED";
	int textLength = strlen(pauseText);
	
	// Calculate centered position
	int charWidth = 8 * fontScale;
	int textWidth = textLength * charWidth;
	int textX = (SW - textWidth) / 2;
	int textY = (SH / 2) - (4 * fontScale);
	
	// Draw "GAME PAUSED" in red with console font
	drawStringScaled(textX, textY, pauseText, 255, 0, 0, fontScale, pixel);
	
	// Draw "Press ESC to resume" below in smaller text
	int helpScale = fontScale / 2;
	if (helpScale < 1) helpScale = 1;
	const char* helpText = "Press ESC to resume";
	int helpLength = strlen(helpText);
	int helpWidth = helpLength * 8 * helpScale;
	int helpX = (SW - helpWidth) / 2;
	int helpY = textY - (15 * fontScale);
	
	drawStringScaled(helpX, helpY, helpText, 200, 200, 200, helpScale, pixel);
}

// Draw wall collision zones for debug visualization (OPTIMIZED)
void drawWallDebugOverlay() {
	if (!isFPSDisplayEnabled()) return;
	
	int s, w;
	float CS = M.cos[P.a];
	float SN = M.sin[P.a];
	
	// Draw wall collision zones
	for (s = 0; s < numSect; s++) {
		for (w = S[s].ws; w < S[s].we; w++) {
			// Transform wall endpoints to camera space
			int x1 = W[w].x1 - P.x;
			int y1 = W[w].y1 - P.y;
			int x2 = W[w].x2 - P.x;
			int y2 = W[w].y2 - P.y;
			
			// Rotate to camera space
			float camX1 = x1 * CS - y1 * SN;
			float camY1 = x1 * SN + y1 * CS;
			float camX2 = x2 * CS - y2 * SN;
			float camY2 = x2 * SN + y2 * CS;
			
			// Skip if both points are behind player
			if (camY1 < 1.0f && camY2 < 1.0f) continue;
			
			// Calculate wall heights in camera space
			int wz1_bottom = S[s].z1 - P.z + ((P.l * camY1) / 32.0);
			int wz1_top = S[s].z2 - P.z + ((P.l * camY1) / 32.0);
			int wz2_bottom = S[s].z1 - P.z + ((P.l * camY2) / 32.0);
			int wz2_top = S[s].z2 - P.z + ((P.l * camY2) / 32.0);
			
			// Clip if needed
			if (camY1 < 1.0f) {
				float t = (1.0f - camY1) / (camY2 - camY1);
				camX1 = camX1 + t * (camX2 - camX1);
				camY1 = 1.0f;
				wz1_bottom = wz1_bottom + t * (wz2_bottom - wz1_bottom);
				wz1_top = wz1_top + t * (wz2_top - wz1_top);
			}
			if (camY2 < 1.0f) {
				float t = (1.0f - camY2) / (camY1 - camY2);
				camX2 = camX2 + t * (camX1 - camX2);
				camY2 = 1.0f;
				wz2_bottom = wz2_bottom + t * (wz1_bottom - wz2_bottom);
				wz2_top = wz2_top + t * (wz1_top - wz2_top);
			}
			
			// Project to screen
			int sx1 = (int)(camX1 * 200.0f / camY1 + SW / 2);
			int sy1_bottom = (int)(wz1_bottom * 200.0f / camY1 + SH / 2);
			int sy1_top = (int)(wz1_top * 200.0f / camY1 + SH / 2);
			
			int sx2 = (int)(camX2 * 200.0f / camY2 + SW / 2);
			int sy2_bottom = (int)(wz2_bottom * 200.0f / camY2 + SH / 2);
			int sy2_top = (int)(wz2_top * 200.0f / camY2 + SH / 2);
			
			// OPTIMIZED: Draw only edges, skip if off-screen
			int steps = abs(sx2 - sx1);
			if (steps < 1) steps = 1;
			
			// Skip walls that are completely off-screen
			if ((sx1 < 0 && sx2 < 0) || (sx1 >= SW && sx2 >= SW)) continue;
			
			float xInc = (float)(sx2 - sx1) / (float)steps;
			float yInc_top = (float)(sy2_top - sy1_top) / (float)steps;
			float yInc_bottom = (float)(sy2_bottom - sy1_bottom) / (float)steps;
			
			float xx = (float)sx1;
			float yy_top = (float)sy1_top;
			float yy_bottom = (float)sy1_bottom;
			
			// OPTIMIZED: Draw wall outline (reduced frequency, every 2 pixels instead of 1)
			for (int i = 0; i <= steps; i += 2) {
				int px = (int)xx;
				int py_top = (int)yy_top;
				int py_bottom = (int)yy_bottom;
				
				// Bounds check
				if (px < 0 || px >= SW) {
					xx += xInc * 2;
					yy_top += yInc_top * 2;
					yy_bottom += yInc_bottom * 2;
					continue;
				}
				
				// Draw top edge
				if (py_top >= 0 && py_top < SH) {
					pixel(px, py_top, 255, 0, 255); // Magenta for wall hitbox
				}
				
				// Draw bottom edge
				if (py_bottom >= 0 && py_bottom < SH) {
					pixel(px, py_bottom, 255, 0, 255); // Magenta for wall hitbox
				}
				
				// OPTIMIZED: Draw vertical line connecting top and bottom (every 8 pixels instead of 4)
				if (i % 8 == 0) {
					int y_start = py_bottom < py_top ? py_bottom : py_top;
					int y_end = py_bottom > py_top ? py_bottom : py_top;
					
					// Clamp to screen bounds
					if (y_start < 0) y_start = 0;
					if (y_end >= SH) y_end = SH - 1;
					
					// Draw vertical line with skipping (every 3 pixels)
					for (int yy = y_start; yy <= y_end; yy += 3) {
						pixel(px, yy, 255, 0, 255); // Magenta vertical line
					}
				}
				
				xx += xInc * 2;
				yy_top += yInc_top * 2;
				yy_bottom += yInc_bottom * 2;
			}
		}
	}
}

void display() {
	int x, y;

	if (T.fr1 - T.fr2 >= 28) { // 35 fps (1000ms/35 = ~28.57ms per frame)
		// If we should show main screen (haven't pressed Enter yet), just draw that
		if (shouldShowMainScreen()) {
			drawMainScreen(pixel, SW, SH);
		}
		else {
			// Update effects
			updateScreenShake(T.fr1);
			updateParticles(T.fr1);
			
			// Normal game rendering
			clearBackground();
			
			// Check if player is dead
			if (playerDead) {
				// Still render the world but don't update
				draw3D();
				
				// Draw particles on top
				drawParticles(pixel, SW, SH, P.x, P.y, P.z, P.a, M.cos, M.sin, T.fr1);
				
				drawDeathScreen(pixel, SW, SH);
				
				// Draw console on top of everything
				drawConsoleText();
			}
			else {
				// Only update player movement and enemies if not paused
				if (!gamePaused) {
					movePl();
					// Update enemy AI with current time for animation
					updateEnemies(P.x, P.y, P.z, T.fr1);
					
					// Update pickups
					updatePickups(P.x, P.y, P.z, T.fr1);
					
					// Handle weapon firing - only fire once per click (not auto-fire)
					if (K.fire && !K.firePressed) {
						int targetEnemy = getEnemyInCrosshair(P.x, P.y, P.a, M.cos, M.sin);
						if (fireWeapon(targetEnemy, T.fr1)) {
							// Play weapon sound
							playWeaponSound(weapon.currentWeapon);
							
							// Add screen shake when firing
							if (weapon.currentWeapon == WEAPON_SHOTGUN) {
								addScreenShake(4.0f);
							} else if (weapon.currentWeapon == WEAPON_CHAINGUN) {
								addScreenShake(1.5f);
							} else if (weapon.currentWeapon == WEAPON_PISTOL) {
								addScreenShake(1.0f);
							}
							K.firePressed = 1;  // Mark as pressed to prevent auto-fire
						}
					}
					
					// Update weapon state
					int isMoving = (K.w || K.s || K.sl || K.sr);
					updateWeapon(isMoving, T.fr1);
				}
				
				draw3D();
				
				// Draw pickups
				drawPickups(pixel, SW, SH, P.x, P.y, P.z, P.a, M.cos, M.sin, depthBuffer, T.fr1);
				
				// Draw particles on top
				drawParticles(pixel, SW, SH, P.x, P.y, P.z, P.a, M.cos, M.sin, T.fr1);
				
				// Check if aiming at enemy for crosshair color
				int targetEnemy = getEnemyInCrosshair(P.x, P.y, P.a, M.cos, M.sin);
				
				// Draw crosshair (changes color when targeting enemy)
				if (!isFPSDisplayEnabled()) {
					drawCrosshair(pixel, SW, SH, targetEnemy >= 0);
				}
				
				// Draw muzzle flash BEFORE weapon sprite (so it appears behind the gun)
				drawMuzzleFlash(pixel, SW, SH, T.fr1);
				
				// Draw weapon sprite at bottom center of screen (on top of muzzle flash)
				drawWeaponSprite(pixel, SW, SH, T.fr1);
				
				// Draw damage overlay (red flash when hit)
				drawDamageOverlay(pixel, SW, SH, T.fr1);
				
				// Draw low health warning effect
				drawLowHealthOverlay(pixel, SW, SH, playerHealth, T.fr1);
				
				// Draw pickup flash effect
				drawFlashOverlay(pixel, SW, SH, T.fr1);
				
				// Draw kill streak messages
				drawKillStreakMessage(pixel, SW, SH, T.fr1);
				
				// Draw HUD (health, armor, etc.)
				drawHUD(pixel, SW, SH);
				
				// Draw weapon HUD (ammo count)
				drawWeaponHUD(pixel, SW, SH);
				
				// Draw powerup status indicators
				drawPowerupStatus(pixel, SW, SH, T.fr1);
				
				// Update and draw automap (modular)
				updateAutomap();
				drawAutomap(pixel, SW, SH, (PlayerState*)&P, (WallData*)W, (SectorData*)S, numSect, (MathTable*)&M);
				
				// Update console animation
				updateConsole();
				
				// Update and draw FPS counter (modular)
				updateFPSCounter(T.fr1);
				
				// Draw debug overlay if enabled
				if (isFPSDisplayEnabled()) {
					drawWallDebugOverlay();
					drawDebugOverlay(pixel, SW, SH, P.x, P.y, P.z, P.a, P.l);
					
					int playerScreenRadius = 15;
					int centerX = SW / 2;
					int centerY = SH / 2;
					
					for (int angle = 0; angle < 360; angle += 15) {
						int x = centerX + (int)(playerScreenRadius * M.cos[angle]);
						int y = centerY + (int)(playerScreenRadius * M.sin[angle]);
						if (x >= 0 && x < SW && y >= 0 && y < SH) {
							pixel(x, y, 0, 255, 255);
						}
					}
				}

				// Draw FPS text
				drawFPSCounter(pixel, SH);
				
				// Draw pause menu if game is paused
				drawPauseMenu();
				
				// Draw console on top of everything
				drawConsoleText();
			}
			
			// Update and draw screen melt effect on top of everything (modular)
			updateScreenMelt();
			drawScreenMelt(pixel, SW, SH);
		}
		
		T.fr2 = T.fr1;
		glutSwapBuffers();
		glutReshapeWindow(GSLW, GSLH);
	}
	T.fr1 = glutGet(GLUT_ELAPSED_TIME);
	glutPostRedisplay();
}


void KeysDown(unsigned char key, int x, int y)
{
	// Toggle pause with Escape key (ESC = 27)
	if (key == 27) {
		// Don't toggle pause if console is active
		if (!console.active) {
			gamePaused = !gamePaused;
		}
		return;
	}
	
	// Toggle console with backtick/tilde key
	if (key == '`' || key == '~') {
		toggleConsole();
		return;
	}
	
	// Toggle automap with Tab key (ASCII 9) - now modular
	if (key == 9) {
		toggleAutomap();
		return;
	}
	
	// Toggle mouse look with F key
	if (key == 'f' && !console.active && !gamePaused) {
		toggleMouseLook();
		return;
	}
	
	// If console is active, send input to console
	if (console.active) {
		consoleHandleKey(key);
		return;
	}
	
	// Don't process game controls if paused
	if (gamePaused) return;
	
	// If player is dead, Enter key restarts
	if (playerDead) {
		if (key == 13) {
			// Reset player
			playerHealth = 100;
			playerMaxHealth = 100;
			playerArmor = 0;
			playerDead = 0;
			initWeapons();
			load();
			startScreenMelt();
		}
		return;
	}
	
	// Normal game controls
	if (key == 'w') { K.w = 1; }
	if (key == 's') { K.s = 1; }
	if (key == 'a') { K.a = 1; }
	if (key == 'd') { K.d = 1; }
	if (key == 'm') { K.m = 1; }
	if (key == '.') { K.sr = 1; }
	if (key == ',') { K.sl = 1; }
	
	// Fire weapon with space bar
	if (key == ' ') { K.fire = 1; }
	
	// Weapon switching with number keys
	if (key == '1') { selectWeapon(WEAPON_FIST); }
	if (key == '2') { selectWeapon(WEAPON_PISTOL); }
	if (key == '3') { selectWeapon(WEAPON_SHOTGUN); }
	if (key == '4') { selectWeapon(WEAPON_CHAINGUN); }
	
	// Weapon switching with Q/E
	if (key == 'q') { prevWeapon(); }
	if (key == 'e') { nextWeapon(); }
	
	// Toggle HUD with H
	if (key == 'h') { toggleHUD(); }
	
	if (key == 13) { 
		load(); // Reload level
		startScreenMelt(); // Start melt effect (modular)
	}
}

void specialKeys(int key, int x, int y)
{
	// Toggle pause with Escape key
	if (key == 27) { // GLUT_KEY_ESC is 27
		// Don't toggle pause if console is active
		if (!console.active) {
			gamePaused = !gamePaused;
		}
		return;
	}
	
	// Don't process special keys if console is active or paused
	if (console.active || gamePaused) return;
	
	// Toggle FPS display with F1 - now modular
	if (key == GLUT_KEY_F1) { toggleFPSDisplay(); }
}

void KeysUp(unsigned char key, int x, int y)
{
	// Don't process key releases if console is active
	if (console.active && key != '`' && key != '~') {
		return;
	}
	
	if (key == 'w') { K.w = 0; }
	if (key == 's') { K.s = 0; }
	if (key == 'a') { K.a = 0; }
	if (key == 'd') { K.d = 0; }
	if (key == 'm') { K.m = 0; }
	if (key == '.') { K.sr = 0; }
	if (key == ',') { K.sl = 0; }
	if (key == ' ') { K.fire = 0; K.firePressed = 0; }  // Reset both on release
}

// Mouse handling
void mouseClick(int button, int state, int x, int y) {
	// Enable mouse look on any click if not already enabled
	if (!mouseEnabled && state == GLUT_DOWN && !console.active && !gamePaused && !playerDead) {
		toggleMouseLook();
	}
	
	if (console.active || gamePaused || playerDead) return;
	
	// Left mouse button for shooting
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			K.fire = 1;
			// Don't reset firePressed here - let the firing logic handle it
		} else {
			K.fire = 0;
			K.firePressed = 0;  // Reset on mouse release to allow next shot
		}
	}
	
	// Right mouse button to release mouse
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		if (mouseEnabled) {
			toggleMouseLook();  // Release mouse on right click
		}
	}
	
	// Scroll wheel for weapon switching
	if (button == 3) { // Scroll up
		nextWeapon();
	}
	if (button == 4) { // Scroll down
		prevWeapon();
	}
}

// Mouse motion for looking around
void mouseMotion(int x, int y) {
	if (!mouseEnabled || console.active || gamePaused || playerDead) return;
	
	// Calculate center of window
	int centerX = GSLW / 2;
	int centerY = GSLH / 2;
	
	// Skip if mouse is at center (we just warped it there)
	if (x == centerX && y == centerY) return;
	
	// Calculate mouse delta
	int deltaX = x - centerX;
	
	// Apply mouse sensitivity (higher = faster turning)
	float sensitivity = 0.12f;
	int angleChange = (int)(deltaX * sensitivity);
	
	// Update player angle
	P.a += angleChange;
	if (P.a < 0) P.a += 360;
	if (P.a >= 360) P.a -= 360;
	
	// Warp mouse back to center
	glutWarpPointer(centerX, centerY);
}

void toggleMouseLook() {
	mouseEnabled = !mouseEnabled;
	if (mouseEnabled) {
		glutSetCursor(GLUT_CURSOR_NONE); // Hide cursor
		glutWarpPointer(GSLW / 2, GSLH / 2);
	} else {
		glutSetCursor(GLUT_CURSOR_INHERIT); // Show cursor
	}
}

void init() {
	int x;
	for (x = 0; x < 360; x++) {
		M.cos[x] = cos(x * 3.14159 / 180);
		M.sin[x] = sin(x * 3.14159 / 180);
	}
	
	// Initialize player
	P.x = 70; P.y = -110; P.z = 20; P.a = 0; P.l = 0;

	// Initialize timing
	T.fr1 = 0;
	T.fr2 = 0;
	
	// Initialize keys
	K.w = 0; K.s = 0; K.a = 0; K.d = 0;
	K.sl = 0; K.sr = 0; K.m = 0; K.fire = 0; K.firePressed = 0;

	// Initialize console with screen dimensions
	initConsole(SW, SH);

	// Initialize FPS counter (modular)
	initFPSCounter();
	
	// Initialize automap (modular)
	initAutomap();
	
	// Initialize HUD
	initHUD();
	
	// Initialize weapon system
	initWeapons();
	
	// Initialize enemy system
	initEnemies();
	
	// Initialize effects system (NEW)
	initEffects();
	
	// Initialize pickup system (NEW)
	initPickups();
	
	// Initialize sound system
	initSound();
	
	// Add different enemy types in the world for variety
	addEnemyType(200, 200, 20, ENEMY_TYPE_BOSSA1);
	addEnemyType(400, 300, 20, ENEMY_TYPE_BOSSA2);
	addEnemyType(150, 350, 20, ENEMY_TYPE_BOSSA3);
	addEnemyType(300, 150, 20, ENEMY_TYPE_BOSSA1);
	addEnemyType(250, 400, 20, ENEMY_TYPE_BOSSA2);
	
	// Pickups are now loaded from level.h

	// Initialize texture 0 (128x128 - raw RGB format)
	Textures[0].w = T_00_WIDTH;
	Textures[0].h = T_00_HEIGHT;
	Textures[0].name = T_00;

	// Initialize texture 1 (32x32 checkerboard - raw RGB format)
	Textures[1].w = T_01_WIDTH;
	Textures[1].h = T_01_HEIGHT;
	Textures[1].name = T_01;

	//56x56 texture
	Textures[2].w = T_02_WIDTH;
	Textures[2].h = T_02_HEIGHT;
	Textures[2].name = T_02;

	Textures[3].w = T_03_WIDTH;
	Textures[3].h = T_03_HEIGHT;
	Textures[3].name = T_03;

	Textures[4].w = T_04_WIDTH;
	Textures[4].h = T_04_HEIGHT;
	Textures[4].name = T_04;

	Textures[5].w = T_05_WIDTH;
	Textures[5].h = T_05_HEIGHT;
	Textures[5].name = T_05;

	Textures[6].w = T_06_WIDTH;
	Textures[6].h = T_06_HEIGHT;
	Textures[6].name = T_06;

	// Initialize animated texture 7 (WALL57 - 4 frames)
	Textures[7].w = WALL57_FRAME_WIDTH;
	Textures[7].h = WALL57_FRAME_HEIGHT;
	Textures[7].name = WALL57_frames[0];  // Start with first frame

	// Initialize animated texture 8 (WALL58 - 3 frames)
	Textures[8].w = WALL58_FRAME_WIDTH;
	Textures[8].h = WALL58_FRAME_HEIGHT;
	Textures[8].name = WALL58_frame_0;  // Start with first frame

	// Load the map automatically at startup
	load();
	
	// Initialize screen melt effect (but don't start it yet - wait for Enter key)
	initScreenMelt();
}
int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowPosition(SCRPOS_W, SCRPOS_H);
	glutInitWindowSize(GSLW, GSLH);
	glutCreateWindow("Doom Clone");
	glPointSize(pixelScale);
	gluOrtho2D(0, GSLW, 0, GSLH);
	init();
	glutDisplayFunc(display);
	glutKeyboardFunc(KeysDown);
	glutKeyboardUpFunc(KeysUp);
	glutSpecialFunc(specialKeys);
	glutMouseFunc(mouseClick);
	glutPassiveMotionFunc(mouseMotion);
	glutMotionFunc(mouseMotion);
	glutMainLoop();
	return 0;
}