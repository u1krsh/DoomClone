#include <stdio.h>
#include <GL/glut.h>
#include <math.h>

#define res 1 //resolition scale
#define SH 240*res //screen height
#define SW 320*res //screen width
#define HSH SH/2 //half screen height
#define HSW SW/2 //half screen width
#define pixelScale  3/res //open gl pixel size
#define GSLW SW*pixelScale //open gl window width
#define GSLH SH*pixelScale //open gl window height

#define M_PI 3.14159265358979323846  /* pi */

// Consolidated texture includes
#include "textures/all_textures.h"

// Console module
#include "console.h"

int numText = NUM_TEXTURES - 1;  // Use macro from all_textures.h (max texture index)

int numSect = 0;                          //number of sectors
int numWall = 0;                          //number of walls

// Collision detection constants
#define PLAYER_RADIUS 8  // Player collision radius


typedef struct {
	int fr1, fr2; // these are frame 1 and 2 for a constant frame rate
	int fps;      // current FPS
	int frameCount; // frame counter
	int fpsTimer;   // timer for FPS calculation
	int showFPS;    // toggle FPS display
}time;
time T;

typedef struct
{
	int w, s, a, d;           //move up, down, left, rigth
	int sl, sr;             //strafe left, right 
	int m;                 //move up, down, look up, down
	int showMap;           //toggle automap display
	int mapActive;         //is map currently active
	int mapAnimating;      //is map animation in progress
	float mapSlidePos;     //map slide position (0.0 = hidden, 1.0 = visible)
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


void load()
{
	FILE* fp = fopen("level.h", "r");
	if (fp == NULL) { printf("Error opening level.h"); return; }
	int s, w;

	fscanf_s(fp, "%i", &numSect);   //number of sectors 
	for (s = 0; s < numSect; s++)      //load all sectors
	{
		fscanf_s(fp, "%i", &S[s].ws);
		fscanf_s(fp, "%i", &S[s].we);
		fscanf_s(fp, "%i", &S[s].z1);
		fscanf_s(fp, "%i", &S[s].z2);
		fscanf_s(fp, "%i", &S[s].st);
		fscanf_s(fp, "%i", &S[s].ss);
	}
	fscanf_s(fp, "%i", &numWall);   //number of walls 
	for (s = 0; s < numWall; s++)      //load all walls
	{
		fscanf_s(fp, "%i", &W[s].x1);
		fscanf_s(fp, "%i", &W[s].y1);
		fscanf_s(fp, "%i", &W[s].x2);
		fscanf_s(fp, "%i", &W[s].y2);
		fscanf_s(fp, "%i", &W[s].wt);
		fscanf_s(fp, "%i", &W[s].u);
		fscanf_s(fp, "%i", &W[s].v);
		fscanf_s(fp, "%i", &W[s].shade);
	}
	fscanf_s(fp, "%i %i %i %i %i", &P.x, &P.y, &P.z, &P.a, &P.l); //player position, angle, look direction 
	fclose(fp);
}

void pixel(int x, int y, int r, int g, int b) { //draws pixel at x,y with color c

	//if (c == 6) { rgb[0] = 0; rgb[1] = 60; rgb[2] = 130; } //background 
	glColor3ub(r, g, b);
	glBegin(GL_POINTS);
	glVertex2i(x * pixelScale + 2, y * pixelScale + 2);
	glEnd();
}

void movePl()
{
	// Don't move if console is active
	if (console.active) return;
	
	//move up, down, left, right
	if (K.a == 1 && K.m == 0) { P.a -= 4; if (P.a < 0) { P.a += 360; } }
	if (K.d == 1 && K.m == 0) { P.a += 4; if (P.a > 359) { P.a -= 360; } }
	
	int dx = M.sin[P.a] * 10.0;
	int dy = M.cos[P.a] * 10.0;
	
	// Store old position for collision rollback
	int oldX = P.x;
	int oldY = P.y;
	int newX = P.x;
	int newY = P.y;
	
	// Calculate new position based on input
	if (K.w == 1 && K.m == 0) { newX += dx; newY += dy; }
	if (K.s == 1 && K.m == 0) { newX -= dx; newY -= dy; }
	
	//strafe left, right
	if (K.sr == 1) { newX += dy; newY -= dx; }
	if (K.sl == 1) { newX -= dy; newY += dx; }
	
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

void clipBehindPlayer(int* x1, int* y1, int* z1, int x2, int y2, int z2) {
	float da = *y1;
	float db = y2;
	float d = da - db; if (da == 0) { d = 1; }
	float s = da / (da - db);// intesection factor 
	*x1 = *x1 + s * (x2 - (*x1));
	*y1 = *y1 + s * (y2 - (*y1)); if (*y1 == 0) { *y1 = 1; }// prevents divide by 0
	*z1 = *z1 + s * (z2 - (*z1));
}


int dist(int x1, int y1, int x2, int y2) {
	int distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	return distance;
}

void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int s, int w, int frontBack) {
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

	// Get current texture data (handles animation)
	int texWidth = 0, texHeight = 0;
	const unsigned char* texData = NULL;
	
	#if defined(WALL57_ANIM_AVAILABLE) && WALL57_ANIM_AVAILABLE == 1
	if (wt == NUM_TEXTURES - 2) {  // WALL57 is second-to-last
		// Animated texture WALL57
		int t = glutGet(GLUT_ELAPSED_TIME);
		int frame = (t / WALL57_FRAME_MS) % WALL57_FRAME_COUNT;
		texWidth = WALL57_FRAME_WIDTH;
		texHeight = WALL57_FRAME_HEIGHT;
		texData = WALL57_frames[frame];
	}
	else
	#endif
	#if defined(WALL58_ANIM_AVAILABLE) && WALL58_ANIM_AVAILABLE == 1
	if (wt == NUM_TEXTURES - 1) {  // WALL58 is last
		// Animated texture WALL58
		int t = glutGet(GLUT_ELAPSED_TIME);
		int frame = (t / 150) % WALL58_FRAME_COUNT;  // 150ms per frame
		texWidth = WALL58_FRAME_WIDTH;
		texHeight = WALL58_FRAME_HEIGHT;
		if (frame == 0) texData = WALL58_frame_0;
		else if (frame == 1) texData = WALL58_frame_1;
		else texData = WALL58_frame_2;
	}
	else
	#endif
	{
		// Regular texture
		texWidth = Textures[wt].w;
		texHeight = Textures[wt].h;
		texData = Textures[wt].name;
	}

	// Calculate dynamic shading based on wall angle relative to player view
	// Get wall angle in world space (in degrees)
	float dx_wall = (float)(W[w].x2 - W[w].x1);
	float dy_wall = (float)(W[w].y2 - W[w].y1);
	float wallAngle = atan2f(dy_wall, dx_wall) * 57.2958f; // 180/pi = 57.2958
	if (wallAngle < 0) { wallAngle += 360.0f; }
	
	// Calculate angle difference between player view and wall normal
	// Wall normal is perpendicular to the wall (add 90 degrees)
	float wallNormal = wallAngle + 90.0f;
	if (wallNormal >= 360.0f) { wallNormal -= 360.0f; }
	
	// Calculate angle difference (how much the wall faces the player)
	float angleDiff = wallNormal - (float)P.a;
	
	// Normalize to -180 to 180 range
	while (angleDiff > 180.0f) { angleDiff -= 360.0f; }
	while (angleDiff < -180.0f) { angleDiff += 360.0f; }
	
	// Convert to 0-180 range (we only care about magnitude)
	if (angleDiff < 0) { angleDiff = -angleDiff; }
	
	// Calculate shade based on angle (0 = facing player directly, 180 = perpendicular)
	// Walls facing the player get less shade, perpendicular walls get more shade
	float shadeFactor = angleDiff / 180.0f; // 0.0 to 1.0
	
	// Apply smooth curve for more natural shading
	shadeFactor = shadeFactor * shadeFactor; // Square for smoother transition
	
	// Calculate final shade value (0-90 range for compatibility)
	int dynamicShade = (int)(shadeFactor * 90.0f);
	
	// Clamp to ensure valid range
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

		// Calculate horizontal texture coordinate using original unclipped coordinates
		float ht = ((float)(x - x1_orig) / (float)(x2_orig - x1_orig)) * texWidth * W[w].u;
		
		// Clamp ht to prevent out-of-bounds access
		if (ht < 0) ht = 0;
		if (ht >= texWidth * W[w].u) ht = texWidth * W[w].u - 0.001f;
		
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
				// Calculate vertical texture coordinate using original unclipped coordinates
				float vt = ((float)(y - y1_orig) / (float)wall_height) * texHeight * W[w].v;
				
				// Clamp vt to prevent out-of-bounds access
				if (vt < 0) vt = 0;
				if (vt >= texHeight * W[w].v) vt = texHeight * W[w].v - 0.001f;
				
				int ty = ((int)vt) % texHeight;

				// Clamp texture coordinates to valid range
				if (tx < 0) tx = 0;
				if (tx >= texWidth) tx = texWidth - 1;
				if (ty < 0) ty = 0;
				if (ty >= texHeight) ty = texHeight - 1;

				// Flip vertically and get pixel from texture
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

void draw3D() { // real sussy baka
	int wx[4], wy[4], wz[4];// world x and y
	float CS = M.cos[P.a]; //player cos and sin
	float SN = M.sin[P.a];
	int s, w, frontBack, cycles, x;
	//order sector by distace
	for (s = 0; s < numSect - 1; s++) {
		for (w = 0; w < numSect - s - 1; w++) {
			if (S[w].d < S[w + 1].d) {
				sectors st = S[w];//temp sector
				S[w] = S[w + 1];
				S[w + 1] = st;
			}
		}
	}




	//draw sectors
	for (s = 0; s < numSect; s++) {
		S[s].d = 0; // clear distance 
		if (P.z < S[s].z1) { S[s].surface = 1; cycles = 2; for (x = 0; x < SW; x++) { S[s].surf[x] = SH; } }//bottom surface
		else if (P.z > S[s].z2) { S[s].surface = 2; cycles = 2; for (x = 0; x < SW; x++) { S[s].surf[x] = 0; } }//top surface
		else { S[s].surface = 0; cycles = 1; }// no surface


		for (frontBack = 0; frontBack < cycles; frontBack++) {

			for (w = S[s].ws; w < S[s].we; w++) {

				// offset bottom two points of player
				int x1 = W[w].x1 - P.x, y1 = W[w].y1 - P.y;
				int x2 = W[w].x2 - P.x, y2 = W[w].y2 - P.y;

				//swap for surfaces
				if (frontBack == 1) { int swp = x1; x1 = x2; x2 = swp; swp = y1; y1 = y2; y2 = swp; }


				//world X position
				wx[0] = x1 * CS - y1 * SN;
				wx[1] = x2 * CS - y2 * SN;
				wx[2] = wx[0];
				wx[3] = wx[1];

				//world Y position
				wy[0] = x1 * SN + y1 * CS;
				wy[1] = x2 * SN + y2 * CS;
				wy[2] = wy[0];
				wy[3] = wy[1];

				S[s].d = dist(0, 0, (wx[0] + wx[1]) / 2, (wy[0] + wy[1]) / 2); // store wall distance

				// world Z position
				wz[0] = S[s].z1 - P.z + ((P.l * wy[0]) / 32.0);
				wz[1] = S[s].z1 - P.z + ((P.l * wy[1]) / 32.0);
				wz[2] = S[s].z2 - P.z + ((P.l * wy[0]) / 32.0);
				wz[3] = S[s].z2 - P.z + ((P.l * wy[1]) / 32.0);

				//dont draw if behinde player - FIXED: use continue instead of return
				if (wy[0] < 1 && wy[1] < 1) { continue; } //dont draw this wall behind player, but continue with next wall

				//point 1 behind player

				if (wy[0] < 1) {
					clipBehindPlayer(&wx[0], &wy[0], &wz[0], wx[1], wy[1], wz[1]);//bottom
					clipBehindPlayer(&wx[2], &wy[2], &wz[2], wx[3], wy[3], wz[3]);// top
				}

				// point 2 behind player
				if (wy[1] < 1) {
					clipBehindPlayer(&wx[1], &wy[1], &wz[1], wx[0], wy[0], wz[0]);//bottom
					clipBehindPlayer(&wx[3], &wy[3], &wz[3], wx[2], wy[2], wz[2]);// top
				}

				//screen x y position
				wx[0] = wx[0] * 200 / wy[0] + HSW;
				wy[0] = wz[0] * 200 / wy[0] + HSH;
				wx[1] = wx[1] * 200 / wy[1] + HSW; wy[1] = wz[1] * 200 / wy[1] + HSH;
				wx[2] = wx[2] * 200 / wy[2] + HSW; wy[2] = wz[2] * 200 / wy[2] + HSH;
				wx[3] = wx[3] * 200 / wy[3] + HSW; wy[3] = wz[3] * 200 / wy[3] + HSH;
				//draw points 
				//if (wx[0] > 0 && wz[0] < SW && wy[0]>0 && wy[0] < SH) { pixel(wx[0], wy[0], 0); }
				//if (wx[1] > 0 && wz[1] < SW && wy[1]>0 && wy[1] < SH) { pixel(wx[1], wy[1], 0); }
				drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], s, w, frontBack);
			}
			S[s].d /= (S[s].we - S[s].ws); // average
		}
	}
}

//void testTextures() {
//	int x, y, t;
//	t = 0;
//	for (y = 0; y < Textures[t].h; y++) {
//		for (x = 0; x < Textures[t].w; x++) {
//			int pixelN = (Textures[t].h-y-1) * 3* Textures[t].w + x * 3;
//			int r = Textures[t].name[pixelN + 0];
//			int g = Textures[t].name[pixelN + 1];
//			int b = Textures[t].name[pixelN + 2];
//			pixel(x, y, r, g, b);
//		}
//	}
//}


void floors() {
	int x, y;
	int xo = SW / 2; //x offset
	int yo = SH / 2; // y offset
	float fov = 200.0;
	float lookUpDown = P.l * 2; if (lookUpDown > SH) { lookUpDown = SH; }
	for (y = -yo; y < -lookUpDown; y++) {
		for (x = -xo; x < xo; x++) {
			float fx = x / (float)y;
			float fy = fov / (float)y;

			float rx = fx * M.sin[P.a] - fy * M.cos[P.a] + (P.y / 30.0);
			float ry = fx * M.cos[P.a] - fy * M.sin[P.a] - (P.x / 30.0);
			if (rx < 0) { rx = -rx + 1; }
			if (ry < 0) { ry -= ry + 1; }

			if ((int)rx % 2 == (int)ry % 2) { pixel(x + xo, y + yo, 0, 60, 130); }
			else { pixel(x + xo, y + yo, 0, 60, 130); }
		}
	}
}

void drawNumber(int n, int x, int y) {
	int i, s, xo, yo;
	s = n;
	if (s == 0) { xo = 0; yo = 0; } // 0
	if (s == 1) { xo = 16; yo = 0; } // 1
	if (s == 2) { xo = 32; yo = 0; } // 2
	if (s == 3) { xo = 48; yo = 0; } // 3
	if (s == 4) { xo = 64; yo = 0; } // 4
	if (s == 5) { xo = 80; yo = 0; } // 5
	if (s == 6) { xo = 96; yo = 0; } // 6
	if (s == 7) { xo = 112; yo = 0; } // 7
	if (s == 8) { xo = 128; yo = 0; } // 8
	if (s == 9) { xo = 144; yo = 0; } // 9

	int sx, sy;
	for (sy = 0; sy < 16; sy++) {
		for (sx = 0; sx < 16; sx++) {
			i = (sy * 160 + xo + sx) * 3;
			if (T_NUMBERS[i] > 0) {
				pixel(x + sx, y + sy, T_NUMBERS[i], T_NUMBERS[i + 1], T_NUMBERS[i + 2]);
			}
		}
	}
}

// Simple 3x5 pixel font for digits 0-9
void drawDigit(int digit, int x, int y, int r, int g, int b) {
	// Each digit is 3 pixels wide, 5 pixels tall
	int patterns[10][5] = {
		{0x7, 0x5, 0x5, 0x5, 0x7}, // 0
		{0x2, 0x6, 0x2, 0x2, 0x7}, // 1
		{0x7, 0x1, 0x7, 0x4, 0x7}, // 2
		{0x7, 0x1, 0x7, 0x1, 0x7}, // 3
		{0x5, 0x5, 0x7, 0x1, 0x1}, // 4
		{0x7, 0x4, 0x7, 0x1, 0x7}, // 5
		{0x7, 0x4, 0x7, 0x5, 0x7}, // 6
		{0x7, 0x1, 0x1, 0x1, 0x1}, // 7
		{0x7, 0x5, 0x7, 0x5, 0x7}, // 8
		{0x7, 0x5, 0x7, 0x1, 0x7}  // 9
	};
	
	if (digit < 0 || digit > 9) return;
	
	int row, col;
	for (row = 0; row < 5; row++) {
		for (col = 0; col < 3; col++) {
			if (patterns[digit][row] & (1 << (2 - col))) {
				pixel(x + col, y + row, r, g, b);
			}
		}
	}
}

void drawFPS() {
	// Draw FPS value in top-left corner
	int fps = T.fps;
	int x = 5;
	int y = SH - 10;
	
	// Draw each digit of the FPS
	if (fps >= 100) {
		drawDigit((fps / 100) % 10, x, y, 255, 255, 255);
		drawDigit((fps / 10) % 10, x + 4, y, 255, 255, 255);
		drawDigit(fps % 10, x + 8, y, 255, 255, 255);
	}
	else if (fps >= 10) {
		drawDigit((fps / 10) % 10, x, y, 255, 255, 255);
		drawDigit(fps % 10, x + 4, y, 255, 255, 255);
	}
	else {
		drawDigit(fps % 10, x, y, 255, 255, 255);
	}
}

void drawAutomap() {
	// Update animation
	if (K.mapAnimating) {
		if (K.mapActive) {
			// Slide down
			K.mapSlidePos += 0.15f;
			if (K.mapSlidePos >= 1.0f) {
				K.mapSlidePos = 1.0f;
				K.mapAnimating = 0;
			}
		} else {
			// Slide up
			K.mapSlidePos -= 0.15f;
			if (K.mapSlidePos <= 0.0f) {
				K.mapSlidePos = 0.0f;
				K.mapAnimating = 0;
			}
		}
	}
	
	// Don't draw if not visible
	if (K.mapSlidePos <= 0.0f) return;
	
	// Calculate visible height based on slide position
	int visibleHeight = (int)(SH * K.mapSlidePos);
	
	// Map covers full screen width, slides from top
	int mapX = 0;
	int mapY = SH - visibleHeight;
	int mapWidth = SW;
	int mapHeight = visibleHeight;
	
	// Don't draw if too small
	if (mapHeight < 10) return;
	
	int mapScale = 4; // Scale down factor for the map
	
	// Draw map background (black)
	int x, y;
	for (y = mapY; y < SH; y++) {
		for (x = 0; x < SW; x++) {
			pixel(x, y, 0, 0, 0); // Black background
		}
	}
	
	// Draw bottom border (red line - 2 pixels thick)
	for (x = 0; x < SW; x++) {
		pixel(x, mapY, 255, 0, 0); // Red border
		if (mapY + 1 < SH) {
			pixel(x, mapY + 1, 255, 0, 0);
		}
	}
	
	// Calculate map center (player position)
	int centerX = mapWidth / 2;
	int centerY = mapY + mapHeight / 2;
	
	// Draw walls
	int s, w;
	for (s = 0; s < numSect; s++) {
		for (w = S[s].ws; w < S[s].we; w++) {
			// Transform wall coordinates relative to player
			int wx1 = (W[w].x1 - P.x) / mapScale + centerX;
			int wy1 = (W[w].y1 - P.y) / mapScale + centerY;
			int wx2 = (W[w].x2 - P.x) / mapScale + centerX;
			int wy2 = (W[w].y2 - P.y) / mapScale + centerY;
			
			// Draw line using Bresenham's algorithm
			int dx = wx2 - wx1;
			int dy = wy2 - wy1;
			int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
			
			if (steps > 0) {
				float xInc = dx / (float)steps;
				float yInc = dy / (float)steps;
				float currX = wx1;
				float currY = wy1;
				
				int i;
				for (i = 0; i <= steps; i++) {
					int px = (int)currX;
					int py = (int)currY;
					
					// Clip to map bounds
					if (px >= mapX && px < mapX + mapWidth && py >= mapY && py < mapY + mapHeight) {
						pixel(px, py, 200, 200, 200); // Gray walls
					}
					
					currX += xInc;
					currY += yInc;
				}
			}
		}
	}
	
	// Draw player as upside-down cross (ankh/crucifix)
	// Vertical bar
	for (y = -6; y <= 2; y++) {
		pixel(centerX, centerY + y, 255, 0, 0);
	}
	// Horizontal bar (top)
	for (x = -3; x <= 3; x++) {
		pixel(centerX + x, centerY - 2, 255, 0, 0);
	}
	
	// Draw direction indicator (red line pointing in player's direction)
	int dirLen = 12; // Length of direction indicator
	int dirX = centerX + (int)(M.sin[P.a] * dirLen);
	int dirY = centerY + (int)(M.cos[P.a] * dirLen);
	
	// Draw direction line
	{
		int dx = dirX - centerX;
		int dy = dirY - centerY;
		int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
		
		if (steps > 0) {
			float xInc = dx / (float)steps;
			float yInc = dy / (float)steps;
			float currX = centerX;
			float currY = centerY;
			
			int i;
			for (i = 0; i <= steps; i++) {
				int px = (int)currX;
				int py = (int)currY;
				
				if (px >= mapX && px < mapX + mapWidth && py >= mapY && py < mapY + mapHeight) {
					pixel(px, py, 255, 255, 0); // Yellow direction line
				}
				
				currX += xInc;
				currY += yInc;
			}
		}
	}
}

void drawConsoleText() {
	if (console.slidePos <= 0.0f) return;
	
	// Calculate console height based on screen height percentage
	int consoleHeight = (int)(SH * CONSOLE_HEIGHT_PERCENT * console.slidePos);
	int x, y;
	
	// Draw semi-transparent console background
	for (y = SH - consoleHeight; y < SH; y++) {
		for (x = 0; x < SW; x++) {
			pixel(x, y, 0, 0, 0); // Black background
		}
	}
	
	// Draw console border
	for (x = 0; x < SW; x++) {
		pixel(x, SH - consoleHeight, 255, 255, 0); // Yellow border
	}
	
	// Calculate font scale based on resolution (1 for low res, 2 for medium, 3+ for high)
	int fontScale = 1;
	if (SH >= 480) fontScale = 2;
	if (SH >= 720) fontScale = 3;
	if (SH >= 1080) fontScale = 4;
	
	int lineHeight = 10 * fontScale; // Height of each text line
	
	// Draw message history (from top of console downward)
	int messageY = SH - consoleHeight + (5 * fontScale); // Start below border
	for (int i = 0; i < console.messageCount && i < CONSOLE_MESSAGE_LINES; i++) {
		if (console.messages[i][0] != '\0') {
			drawStringScaled(5 * fontScale, messageY, console.messages[i], 255, 255, 255, fontScale, pixel);
			messageY += lineHeight;
		}
	}
	
	// Draw prompt ">" and input text at bottom
	int textY = SH - lineHeight; // Position from bottom
	int textX = 5 * fontScale; // Scaled margin
	
	// Draw ">" prompt using the scaled font
	drawCharScaled(textX, textY, '>', 255, 255, 0, fontScale, pixel);
	
	textX += (10 * fontScale); // Move past the prompt (scaled spacing)
	
	// Draw input text using the scaled bitmap font
	drawStringScaled(textX, textY, console.input, 255, 255, 255, fontScale, pixel);
	
	// Draw cursor (blinking)
	int cursorX = textX + (console.inputPos * 8 * fontScale); // 8 pixels per character * scale
	if ((T.fr1 / 500) % 2 == 0) { // Blink every 500ms
		// Draw underscore cursor
		drawCharScaled(cursorX, textY, '_', 255, 255, 0, fontScale, pixel);
	}
}

void display() {
	int x, y;

	if (T.fr1 - T.fr2 >= 28) { // 35 fps (1000ms/35 = ~28.57ms per frame)
		clearBackground();
		movePl();
		draw3D();
		
		// Draw automap on top of 3D view if active
		drawAutomap();
		
		// Update console animation
		updateConsole();
		
		if (T.showFPS) { // Only draw FPS if enabled
			drawFPS();
		}
		
		// Draw console on top of everything
		drawConsoleText();
		
		// Calculate FPS
		T.frameCount++;
		if (T.fr1 - T.fpsTimer >= 1000) { // Update FPS every second
			T.fps = T.frameCount;
			T.frameCount = 0;
			T.fpsTimer = T.fr1;
		}
		
		T.fr2 = T.fr1;
		glutSwapBuffers();
		glutReshapeWindow(GSLW, GSLH); // stops from resizeing window
	}
	T.fr1 = glutGet(GLUT_ELAPSED_TIME);
	glutPostRedisplay();
}


void KeysDown(unsigned char key, int x, int y)
{
	// Toggle console with backtick/tilde key
	if (key == '`' || key == '~') {
		toggleConsole();
		return;
	}
	
	// Toggle automap with Tab key (ASCII 9)
	if (key == 9) {
		K.mapActive = !K.mapActive;
		K.mapAnimating = 1;
		return;
	}
	
	// If console is active, send input to console
	if (console.active) {
		consoleHandleKey(key);
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
	if (key == 13) { load(); } //enter key to load level
}

void specialKeys(int key, int x, int y)
{
	// Don't process special keys if console is active
	if (console.active) return;
	
	if (key == GLUT_KEY_F1) { T.showFPS = !T.showFPS; } // Toggle FPS display with F1
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
}

int loadSector[] =
{//wall start, wall end, z1 height , z2 height 
	0, 4, 0, 40,2,3, // sector 1
	4, 8, 0, 40,4,5, //sector 2
	8, 12, 0, 40,5,3,// sector 3
	12, 16, 0, 40,0,1 // sector 4
};
 

void init() {
	int x;
	for (x = 0; x < 360; x++) {
		M.cos[x] = cos(x * 3.14159 / 180);
		M.sin[x] = sin(x * 3.14159 / 180);
	}
	//init playe
	P.x = 70; P.y = -110; P.z = 20; P.a = 0; P.l = 0;

	// Initialize FPS counter
	T.fps = 0;
	T.frameCount = 0;
	T.fpsTimer = 0;
	T.fr1 = 0;
	T.fr2 = 0;
	T.showFPS = 0; // FPS counter starts hidden

	// Initialize keys
	K.showMap = 0; // Automap starts hidden
	K.mapActive = 0;
	K.mapAnimating = 0;
	K.mapSlidePos = 0.0f;


	// Initialize console with screen dimensions
	initConsole(SW, SH);

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
}

int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowPosition(GSLW / 2, GSLH / 2);
	glutInitWindowSize(GSLW, GSLH);
	glutCreateWindow("");
	glPointSize(pixelScale); //pixel size
	gluOrtho2D(0, GSLW, 0, GSLH); // origin
	init();
	glutDisplayFunc(display);
	glutKeyboardFunc(KeysDown);
	glutKeyboardUpFunc(KeysUp);
	glutSpecialFunc(specialKeys); // Register special keys handler
	glutMainLoop();
	return 0;
}