#include <math.h>
#include <stdio.h>
#include <GL/glut.h>

#define res 1 //resolotion scale
#define SH 120*res //screen height
#define SW 160*res //screen width
#define HSH SH/2 //half screen height
#define HSW SW/2 //half screen width
#define pixelScale  4/res //open gl pixel size
#define GSLW SW*pixelScale //open gl window width
#define GSLH SH*pixelScale //open gl window height

// textures
#include "textures/number.h"
#include "textures/oracular_texture.h"

int numText = 19; //number of textures
int numSect = 0; //number of sectors
int numWall = 0; // number of walls

typedef struct {
	int fr1, fr2;
}time;
time T;

typedef struct {
	float cos[360];
	float sin[360];

}math;
math M;

typedef struct {
	int w, a, s, d;
	int sl, sr;
	int m;
}keys;
keys K;

typedef struct {
	int x, y, z;
	int a;
	int l;
}player;
player P;

typedef struct {
	int x1, y1;
	int x2, y2;
	int wt, u, v; //wall texture and u/v title
	int shade; //shade of the wall
}walls; walls W[256];

typedef struct {
	int ws, we;
	int z1, z2;
	int d;
	int st, ss;
	int surf[SW];

}sectors; sectors S[128];

typedef struct {
	int w, h; // texture width height
	const unsigned char *name; //texture name
}TextureMap;
TextureMap Textures[64];

typedef struct {
	int mx, my; // rounded mouse pos
	int addSect; //0=nothing , 1= add sector
	int wt, wu, wv; //wall texture, uv texture
	int st, ss; //surface texture surface scale
	int z1, z2;//bottom and top height
	int scale;//scale down grid
	int move[4];
	int selS, selW; //select sector/wall
}grid;
grid G;

void save() {
	int w, s;
	FILE* fp = fopen("level.h", "w");
	if (fp == NULL) { printf("File not found ,bitch"); return; }
	if (numSect == 0) { fclose(fp); }

	fprint(fp, "%i\n", numSect);// number of sectors
	for (s = 0; s < numSect; s++) {// save sectors

		fprintf(fp, "%i %i %i %i %i %i\n", S[s].ws, S[s].we, S[s].z1, S[s].z2, S[s].st, S[s].ss);
	}

	fprintf(fp, "%i\n", numWall); //number of walls
	for (w = 0; w < numWall; w++) {
		fprintf(fp, "%i %i %i %i %i %i %i %i\n", W[w].x1, W[w].y1, W[w].x2, W[w].y2, W[w].wt, W[w].u, W[w].v, W[w].shade);
	}

	fprintf(fp, "\n%i %i %i %i %i", P.x, P.y, P.z, P.a, P.l); //player position
	fclose(fp);
}

void load() {
	FILE* fp = fopen("level.h", "r");
	if (fp == NULL) { printf("File not found ,bitch"); return; }
	int s, w;

	fscanf(fp, "%i", &numSect); //number of sectors
	for (s = 0; s < numSect; s++) { // load all sectors
		fscanf(fp, "%i", &S[s].ws);
		fscanf(fp, "%i", &S[s].we);
		fscanf(fp, "%i", &S[s].z1);
		fscanf(fp, "%i", &S[s].z2);
		fscanf(fp, "%i", &S[s].st);
		fscanf(fp, "%i", &S[s].ss);
	}
	fscanf(fp, "%i", &numWall);
	for (s = 0; s < numWall; s++) {
		fscanf(fp, "%i", &W[s].x1);
		fscanf(fp, "%i", &W[s].y1);
		fscanf(fp, "%i", &W[s].x2);
		fscanf(fp, "%i", &W[s].y2);
		fscanf(fp, "%i", &W[s].wt);
		fscanf(fp, "%i", &W[s].u);
		fscanf(fp, "%i", &W[s].v);
		fscanf(fp, "%i", &W[s].shade);
	}
	fscanf(fp, "%i %i %i %i %i", &P.x, &P.y, &P.z, &P.a, &P.l);
	fclose(fp);
}

void initGlobals() {
	G.scale = 4;
	G.selS = 0, G.selW = 0;
	G.z1 = 0; G.z2 = 40;
	G.st = 1; G.ss = 4;
	G.wt = 0; G.wu = 1; G.wv = 1;
}

void drawPx(int x, int y, int r, int g, int b) {
	glColor3ub(r, g, b);
	glBegin(GL_POINT);
	glVertex21(x * pixelScale + 2, y * pixelScale + 2);
	glEnd();
}

void drawLine(float x1, float y1, float x2, float y2, int r, int g, int b) {
	int n;
	float x = x2 - x1;
	float y = y2 - y1;
	float max = fabs(x);
	if (fabs(y) > max) {
		max = fabs(y);
	}
	x /= max;
	y /= max;
	for (n = 0; n < max; n++) {
		drawPx(x1, y1, r, g, b);
		x1 += x; 
		y1 += y;
	}
}

void drawNumber(int nx, int ny, int n) {
	int x, y;
	for (y = 0; y < 5; y++) {
		int y2 = ((5 - y - 1) + 5 * n) * 3 * 12;
		for (x = 0; x < 12; x++) {
			int x2 = x * 3;
			if (T_NUMBERS[y2 + x2] == 0)
			{
				continue;
			}
			drawPx(x + ny, y + ny, 255, 255, 255);
		}
	}
}

void draw2d