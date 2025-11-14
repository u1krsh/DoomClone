//#include <stdio.h>
//#include <GL/glut.h>
//#include <math.h>
//
//#define res 1 //resolotion scale
//#define SH 120*res //screen height
//#define SW 160*res //screen width
//#define HSH SH/2 //half screen height
//#define HSW SW/2 //half screen width
//#define pixelScale  4/res //open gl pixel size
//#define GSLW SW*pixelScale //open gl window width
//#define GSLH SH*pixelScale //open gl window height
//#define numSect 4 // number of sectors
//#define numWall 16 // number of walls 
//
//typedef struct {
//	int fr1, fr2; // these are frame 1 and 2 for a constant frame rate
//}time; 
//time T;
//
//typedef struct
//{
//	int w, s, a, d;           //move up, down, left, rigth
//	int sl, sr;             //strafe left, right 
//	int m;                 //move up, down, look up, down
//}keys; 
//keys K;
//
//typedef struct {
//	float cos[360];   //cos and sin values
//	float sin[360];
//
//}math;
//math M;
//typedef struct {
//	int x, y, z; //position
//	int a;// angle of rotation
//	int l; // variable to look up and down
//}player; 
//player P;
//
//typedef struct {
//	int x1, y1;//bottom line point 1
//	int x2, y2;//bootom line point 2
//	int c;// wall color 
//}walls;
//walls W[30];
//
//typedef struct {
//	int ws, we; // wall number start and end
//	int z1, z2; // height from bottom to top
//	int x, y; // center of sector
//	int d; // sorting drawing order
//	int c1, c2; //bottom and top color
//	int surf[SW]; // to gold points for surface
//	int surface; //is there a surface to draw
//}sectors;
//sectors S[30];
//
//
//void pixel(int x, int y, int c) { //draws pixel at x,y with color c
//	int rgb[3] = {0,0,0}; // Initialize all elements to 0
//	if (c == 0) { rgb[0] = 255; rgb[1] = 0; rgb[2] = 0; }// red
//	if (c == 1) { rgb[0] = 160; rgb[1] = 0; rgb[2] = 0; }// darkred
//
//	if (c == 2) { rgb[0] = 0; rgb[1] = 255; rgb[2] = 0; }// green
//	if (c == 3) { rgb[0] = 0; rgb[1] = 160; rgb[2] = 0; }// darkgreen
//
//	if (c == 4) { rgb[0] = 0; rgb[1] = 0; rgb[2] = 255; }// blue
//	if (c == 5) { rgb[0] = 0; rgb[1] = 0; rgb[2] = 160; }// darkblue
//
//	if (c == 6) { rgb[0] = 0; rgb[1] = 60; rgb[2] = 130; } //background 
//	glColor3ub(rgb[0], rgb[1], rgb[2]);
//	glBegin(GL_POINTS);
//	glVertex2i(x*pixelScale+2, y*pixelScale+2);
//	glEnd();	
//}
//
//void movePl()
//{
//	//move up, down, left, right
//	if (K.a == 1 && K.m == 0) { P.a -= 4; if (P.a < 0) { P.a += 360; } }
//	if (K.d == 1 && K.m == 0) { P.a += 4; if (P.a > 359) { P.a -= 360; } }
//	int dx = M.sin[P.a] * 10.0;
//	int dy = M.cos[P.a] * 10.0;
//	if (K.w == 1 && K.m == 0) { P.x += dx; P.y += dy; }
//	if (K.s == 1 && K.m == 0) { P.x -= dx; P.y -= dy; }
//	//strafe left, right
//	if (K.sr == 1) { P.x += dy; P.y -= dx; }
//	if (K.sl == 1) { P.x -= dy; P.y += dx; }
//	//move up, down, look up, look down
//	if (K.a == 1 && K.m == 1) { P.l -= 1; }
//	if (K.d == 1 && K.m == 1) { P.l += 1; }
//	if (K.w == 1 && K.m == 1) { P.z -= 4; }
//	if (K.s == 1 && K.m == 1) { P.z += 4; }
//}
//
//void clearBackground() {
//	int x, y;
//	for (y = 0; y < SH; y++) {
//		for (x = 0; x < SW; x++) {
//			pixel(x, y, 6); //clear background 
//		}
//	}
//}
//
//void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2) {
//	float da = *y1;
//	float db = y2;
//	float d = da - db; if (da == 0) { d = 1; }
//	float s = da / (da - db);// intesection factor 
//	*x1 = *x1 + s * (x2 - (*x1));
//	*y1 = *y1 + s * (y2 - (*y1)); if (*y1 == 0) { *y1 = 1; }// prevents divide by 0
//	*z1 = *z1 + s * (z2 - (*z1));
//}
//
//
//int dist(int x1, int y1, int x2, int y2) {
//	int distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
//	return distance;
//}
//
//void drawWall(int x1, int x2, int b1, int b2, int t1, int t2,int c, int s) {
//	int x, y;
//	// hold difference between bottom and top
//	int dyb = b2 - b1; // y distance from bottom line 
//	int dyt = t2 - t1; // top line
//	int dx = x2 - x1; // x distance
//	if (dx == 0) { dx = 1; } // prevent divide by zero
//	int xs = x1; //hold initial x1 staring position
//	//clipping x
//	if (x1 < 1) { x1 = 1; }
//	if (x2 < 1) { x2 = 1; }
//	if (x1 > SW - 1) { x1 = SW - 1; }
//	if (x2 > SW - 1) { x2 = SW - 1; }
//
//	//draw x vertices line
//	for (x = x1; x < x2; x++) {
//		int y1 = dyb * (x - xs + 0.5) / dx + b1; //y bottom point 
//		int y2 = dyt * (x - xs + 0.5) / dx + t1;
//		//clipping y
//		if (y1 < 1) { y1 = 1; }
//		if (y2 < 1) { y2 = 1; }
//		if (y1 > SH - 1) { y1 = SH - 1; }
//		if (y2 > SH - 1) { y2 = SH - 1; }
//		//pixel(x, y1, 0);//bottom
//		//pixel(x, y2, 0);//top
//		//for (y = y1; y < y2; y++) {
//		//	pixel(x, y, c);
//		//}
//
//		if (S[s].surface == 1) { S[s].surf[x] = y1; continue; } //save bottom points
//		if (S[s].surface == 2) { S[s].surf[x] = y2; continue; } //save top    points
//		if (S[s].surface == -1) { for (y = S[s].surf[x]; y < y1; y++) { pixel(x, y, S[s].c1); }; } //bottom
//		if (S[s].surface == -2) { for (y = y2; y < S[s].surf[x]; y++) { pixel(x, y, S[s].c2); }; } //top
//		for (y = y1; y < y2; y++) { pixel(x, y, c); } //normal wall
//	}
//}
//
//void draw3D() { // real sussy baka
//	int wx[4], wy[4], wz[4];// world x and y
//	float CS = M.cos[P.a]; //player cos and sin
//	float SN = M.sin[P.a];
//	int s, w, loop;
//	//order sector by distace
//	for (s = 0; s < numSect - 1; s++) {
//		for (w = 0; w < numSect - s - 1; w++) {
//			if (S[w].d < S[w + 1].d) {
//				sectors st = S[w];//temp sector
//				S[w] = S[w + 1];
//				S[w + 1] = st;
//			}
//		}
//	}
//
//
//
//
//	//draw sectors
//	for (s = 0; s < numSect; s++) {
//		S[s].d = 0; // clear distance 
//		if (P.z < S[s].z1) { S[s].surface = 1; }//bottom surface
//		else if (P.z > S[s].z2) { S[s].surface = 2; }//top surface
//		else { S[s].surface = 0; }// no surface
//
//
//		for (loop = 0; loop < 2; loop++) {
//
//			for (w = S[s].ws; w < S[s].we; w++) {
//
//				// offset bottom two points of player
//				int x1 = W[w].x1 - P.x, y1 = W[w].y1 - P.y;
//				int x2 = W[w].x2 - P.x, y2 = W[w].y2 - P.y;
//
//				//swap for surfaces
//				if (loop == 0) { int swp = x1; x1 = x2; x2 = swp; swp = y1; y1 = y2; y2 = swp; }
//
//
//				//world X position
//				wx[0] = x1 * CS - y1 * SN;
//				wx[1] = x2 * CS - y2 * SN;
//				wx[2] = wx[0];
//				wx[3] = wx[1];
//
//				//world Y position
//				wy[0] = x1 * SN + y1 * CS;
//				wy[1] = x2 * SN + y2 * CS;
//				wy[2] = wy[0];
//				wy[3] = wy[1];
//
//				S[s].d = dist(0, 0, (wx[0] + wx[1]) / 2, (wy[0] + wy[1]) / 2); // store wall distance
//
//				// world Z position
//				wz[0] = S[s].z1 - P.z + ((P.l * wy[0]) / 32.0);
//				wz[1] = S[s].z1 - P.z + ((P.l * wy[1]) / 32.0);
//				wz[2] = wz[0] + S[s].z2;
//				wz[3] = wz[1] + S[s].z2;
//
//				//dont draw if behinde player
//				if (wy[0] < 1 && wy[1] < 1) { return; } //dont draw wall behind player
//
//				//point 1 behind player
//
//				if (wy[0] < 1) {
//					clipBehindPlayer(&wx[0], &wy[0], &wz[0], wx[1], wy[1], wz[1]);//bottom
//					clipBehindPlayer(&wx[2], &wy[2], &wz[2], wx[3], wy[3], wz[3]);// top
//				}
//
//				// point 2 behind player
//				if (wy[1] < 1) {
//					clipBehindPlayer(&wx[1], &wy[1], &wz[1], wx[0], wy[0], wz[0]);//bottom
//					clipBehindPlayer(&wx[3], &wy[3], &wz[3], wx[2], wy[2], wz[2]);// top
//				}
//
//				//screen x y position
//				wx[0] = wx[0] * 200 / wy[0] + HSW;
//				wy[0] = wz[0] * 200 / wy[0] + HSH;
//				wx[1] = wx[1] * 200 / wy[1] + HSW; wy[1] = wz[1] * 200 / wy[1] + HSH;
//				wx[2] = wx[2] * 200 / wy[2] + HSW; wy[2] = wz[2] * 200 / wy[2] + HSH;
//				wx[3] = wx[3] * 200 / wy[3] + HSW; wy[3] = wz[3] * 200 / wy[3] + HSH;
//				//draw points 
//				//if (wx[0] > 0 && wz[0] < SW && wy[0]>0 && wy[0] < SH) { pixel(wx[0], wy[0], 0); }
//				//if (wx[1] > 0 && wz[1] < SW && wy[1]>0 && wy[1] < SH) { pixel(wx[1], wy[1], 0); }
//				drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], W[w].c, s);
//			}
//			S[s].d /= (S[s].we - S[s].ws); // average
//			S[s].surface *= -1; // flip to negative to draw surface
//		}
//	}
//}
//
//void display() {
//	int x, y;
//
//	if (T.fr1 - T.fr2 >= 50) { //20 fps
//		clearBackground();
//		movePl();
//		draw3D();
//		T.fr2 = T.fr1;
//		glutSwapBuffers();
//		glutReshapeWindow(GSLW, GSLH); // stops from resizeing window
//	}
//	T.fr1 = glutGet(GLUT_ELAPSED_TIME); 
//	glutPostRedisplay();
//}
//
//
//void KeysDown(unsigned char key, int x, int y)
//{
//	if (key == 'w' == 1) { K.w = 1; }
//	if (key == 's' == 1) { K.s = 1; }
//	if (key == 'a' == 1) { K.a = 1; }
//	if (key == 'd' == 1) { K.d = 1; }
//	if (key == 'm' == 1) { K.m = 1; }
//	if (key == ',' == 1) { K.sr = 1; }
//	if (key == '.' == 1) { K.sl = 1; }
//}
//void KeysUp(unsigned char key, int x, int y)
//{
//	if (key == 'w' == 1) { K.w = 0; }
//	if (key == 's' == 1) { K.s = 0; }
//	if (key == 'a' == 1) { K.a = 0; }
//	if (key == 'd' == 1) { K.d = 0; }
//	if (key == 'm' == 1) { K.m = 0; }
//	if (key == ',' == 1) { K.sr = 0; }
//	if (key == '.' == 1) { K.sl = 0; }
//}
//
//int loadSector[] =
//{//wall start, wall end, z1 height , z2 height 
//	0, 4, 0, 40,2,3, // sector 1
//	4, 8, 0, 40,4,5, //sector 2
//	8, 12, 0, 40,5,3,// sector 3
//	12, 16, 0, 40,0,1 // sector 4
//};
//
//
//int loadWalls[] =
//{//x1, y1, x2,y2, color
//	0, 0, 32, 0, 0,
//	32, 0, 32, 32, 1,
//	32, 32, 0, 32, 0,
//	0,32, 0, 0, 1,
//
//	64, 0, 96, 0, 2,
//	96, 0, 96, 32, 3,
//	96, 32, 64, 32, 2,
//	64, 32, 64, 0, 3,
//
//	64, 64, 96, 64, 4,
//	96, 64 , 96, 96, 5,
//	96, 96, 64, 96, 4,
//	64, 96, 64, 64, 5,
//
//	0, 64, 32, 64, 5,
//	32, 64, 32, 96, 2,
//	32, 96, 0, 96, 5,
//	0, 96, 0, 64, 4,
//
//
//
//};
//
//void init() {
//	int x;
//	for (x = 0; x < 360; x++){
//		M.cos[x]=cos(x * 3.14159 / 180);
//		M.sin[x]=sin(x * 3.14159 / 180);
//	} 
//	//init playe
//	P.x = 70; P.y = -110; P.z = 20; P.a = 0; P.l = 0;
//
//	//load sectors
//	int s, w, v1 = 0, v2 = 0;
//	for (s = 0; s < numSect; s++) {
//		S[s].ws = loadSector[v1 + 0]; //wall start number 
//		S[s].we = loadSector[v1 + 1];//wall end number
//		S[s].z1 = loadSector[v1 + 2];//sector bottom height
//		S[s].z2 = loadSector[v1 + 3] - loadSector[v1+2]; //sector top height
//		S[s].c1 = loadSector[v1 + 4];
//		S[s].c2 = loadSector[v1 + 5];
// 		v1 += 6;
//		for (w = S[s].ws; w < S[s].we; w++) {
//			W[w].x1 = loadWalls[v2 + 0]; // bottom x1
//			W[w].y1 = loadWalls[v2 + 1]; // bottom y1
//			W[w].x2 = loadWalls[v2 + 2]; //top x2
//			W[w].y2 = loadWalls[v2 + 3]; // top y2
//			W[w].c = loadWalls[v2 + 4]; //wall color
//			v2 += 5;
//		}
//	}
//}
//
//int main(int argc, char* argv[]) {
//	glutInit(&argc, argv);
//	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
//	glutInitWindowPosition(GSLW / 2, GSLH / 2);
//	glutInitWindowSize(GSLW, GSLH);
//	glutCreateWindow("");
//	glPointSize(pixelScale); //pixel size
//	gluOrtho2D(0, GSLW, 0, GSLH); // origin
//	init();
//	glutDisplayFunc(display);
//	glutKeyboardFunc(KeysDown);
//	glutKeyboardUpFunc(KeysUp);
//	glutMainLoop();
//	return 0;
//
//
//}