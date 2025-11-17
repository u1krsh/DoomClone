#include <stdio.h>
#include <GL/glut.h>
#include <math.h>

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
#include "textures/T_00.h"
#include "textures/T_01.h"  // Add 32x32 test texture
#include "textures/T_02.h"
#include "textures/T_03.h"
#include "textures/T_04.h"
#include "textures/T_05.h"
#include "textures/T_06.h"

int numText = 7;                          //number of textures (increased from 1 to 2)

int numSect = 0;                          //number of sectors
int numWall = 0;                          //number of walls



typedef struct {
	int fr1, fr2; // these are frame 1 and 2 for a constant frame rate
}time; 
time T;

typedef struct
{
	int w, s, a, d;           //move up, down, left, rigth
	int sl, sr;             //strafe left, right 
	int m;                 //move up, down, look up, down
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

void pixel(int x, int y,int r, int g, int b) { //draws pixel at x,y with color c

	//if (c == 6) { rgb[0] = 0; rgb[1] = 60; rgb[2] = 130; } //background 
	glColor3ub(r,g,b);
	glBegin(GL_POINTS);
	glVertex2i(x*pixelScale+2, y*pixelScale+2);
	glEnd();	
}

void movePl()
{
	//move up, down, left, right
	if (K.a == 1 && K.m == 0) { P.a -= 4; if (P.a < 0) { P.a += 360; } }
	if (K.d == 1 && K.m == 0) { P.a += 4; if (P.a > 359) { P.a -= 360; } }
	int dx = M.sin[P.a] * 10.0;
	int dy = M.cos[P.a] * 10.0;
	if (K.w == 1 && K.m == 0) { P.x += dx; P.y += dy; }
	if (K.s == 1 && K.m == 0) { P.x -= dx; P.y -= dy; }
	//strafe left, right
	if (K.sr == 1) { P.x += dy; P.y -= dx; }
	if (K.sl == 1) { P.x -= dy; P.y += dx; }
	//move up, down, look up, look down
	if (K.a == 1 && K.m == 1) { P.l -= 1; }
	if (K.d == 1 && K.m == 1) { P.l += 1; }
	if (K.w == 1 && K.m == 1) { P.z -= 4; }
	if (K.s == 1 && K.m == 1) { P.z += 4; }
}

void clearBackground() {
	int x, y;
	for (y = 0; y < SH; y++) {
		for (x = 0; x < SW; x++) {
			pixel(x, y, 0,60,130); //clear background 
		}
	}
}

void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2) {
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

void drawWall(int x1, int x2, int b1, int b2, int t1, int t2,int s, int w, int frontBack) {
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
		float ht = ((float)(x - x1_orig) / (float)(x2_orig - x1_orig)) * Textures[wt].w * W[w].u;
		int tx = ((int)ht) % Textures[wt].w;
		
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
				float vt = ((float)(y - y1_orig) / (float)wall_height) * Textures[wt].h * W[w].v;
				int ty = ((int)vt) % Textures[wt].h;
				
				// Flip vertically and get pixel from texture
				int pixelN = (Textures[wt].h - ty - 1) * 3 * Textures[wt].w + ((int)ht%Textures[wt].w)*3;
				int r = Textures[wt].name[pixelN + 0] - W[w].shade; if (r < 0) { r = 0; }
				int g = Textures[wt].name[pixelN + 1] - W[w].shade; if (g < 0) { g = 0; }
				int b = Textures[wt].name[pixelN + 2] - W[w].shade; if (b < 0) { b = 0; }
				pixel(x, y, r, g, b);
			} 
		}
		if (frontBack == 1) {
			if (S[s].surface == 1) { y2 = S[s].surf[x]; } //bottom surface save top row
			if (S[s].surface == 2) { y1 = S[s].surf[x]; }
			for (y = y1; y < y2; y++) { pixel(x, y, 255,0,0); } //normal wall
		}
	}
}

void draw3D() { // real sussy baka
	int wx[4], wy[4], wz[4];// world x and y
	float CS = M.cos[P.a]; //player cos and sin
	float SN = M.sin[P.a];
	int s, w, frontBack, cycles,x;
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
		else if (P.z > S[s].z2) { S[s].surface = 2; cycles = 2; for (x = 0; x < SW; x++) { S[s].surf[x] = 0; }}//top surface
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

				//dont draw if behinde player
				if (wy[0] < 1 && wy[1] < 1) { return; } //dont draw wall behind player

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
			float fx = x/(float) y;
			float fy = fov / (float) y;

			float rx = fx * M.sin[P.a] - fy * M.cos[P.a]+(P.y/30.0);
			float ry = fx * M.cos[P.a] - fy * M.sin[P.a]-(P.x/30.0);
			if (rx < 0) { rx = -rx + 1; }
			if (ry < 0) { ry -= ry + 1; }

			if ((int)rx % 2 == (int)ry % 2) { pixel(x + xo, y + yo, 0, 60, 130); }
			else { pixel(x + xo, y + yo, 0, 60, 130); }
		}
	}
}



void display() {
	int x, y;

	if (T.fr1 - T.fr2 >= 50) { //20 fps
		clearBackground();
		movePl();
		floors();/*draw3D();*/
		T.fr2 = T.fr1;
		glutSwapBuffers();
		glutReshapeWindow(GSLW, GSLH); // stops from resizeing window
	}
	T.fr1 = glutGet(GLUT_ELAPSED_TIME); 
	glutPostRedisplay();
}


void KeysDown(unsigned char key, int x, int y)
{
	if (key == 'w') { K.w = 1; }
	if (key == 's') { K.s = 1; }
	if (key == 'a') { K.a = 1; }
	if (key == 'd') { K.d = 1; }
	if (key == 'm') { K.m = 1; }
	if (key == '.') { K.sr = 1; }
	if (key == ',') { K.sl = 1; }
	if (key == 13) { load(); } //enter key to load level
}
void KeysUp(unsigned char key, int x, int y)
{
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
	for (x = 0; x < 360; x++){
		M.cos[x]=cos(x * 3.14159 / 180);
		M.sin[x]=sin(x * 3.14159 / 180);
	} 
	//init playe
	P.x = 70; P.y = -110; P.z = 20; P.a = 0; P.l = 0;

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
    //Textures[5].name = T_05;

    Textures[6].w = T_06_WIDTH;
    Textures[6].h = T_06_HEIGHT;
    Textures[6].name = T_06;

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
	glutMainLoop();
	return 0;


}////