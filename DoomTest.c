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


typedef struct {
	int fr1, fr2; // these are frame 1 and 2 for a constant frame rate
}time; 
time T;

typedef struct {
	int w, a, s, d;
	int sl, sr;
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

void pixel(int x, int y, int c) { //draws pixel at x,y with color c
	int rgb[3] = {0,0,0}; // Initialize all elements to 0
	if (c == 0) { rgb[0] = 255; rgb[1] = 0; rgb[2] = 0; }// red
	if (c == 1) { rgb[0] = 160; rgb[1] = 0; rgb[2] = 0; }// darkred

	if (c == 2) { rgb[0] = 0; rgb[1] = 255; rgb[2] = 0; }// green
	if (c == 3) { rgb[0] = 0; rgb[1] = 160; rgb[2] = 0; }// darkgreen

	if (c == 4) { rgb[0] = 0; rgb[1] = 0; rgb[2] = 255; }// blue
	if (c == 5) { rgb[0] = 0; rgb[1] = 0; rgb[2] = 160; }// darkblue

	if (c == 6) { rgb[0] = 0; rgb[1] = 60; rgb[2] = 130; } //background 
	glColor3ub(rgb[0], rgb[1], rgb[2]);
	glBegin(GL_POINTS);
	glVertex2i(x*pixelScale+2, y*pixelScale+2);
	glEnd();	
}

void movePl() {
	// move player up and down
	if (K.a == 1) { P.a -= 4; if (P.a < 0) { P.a += 360; } }
	if (K.d == 1) { P.a += 4; if (P.a > 359) { P.a -= 360; } }
	int dx = M.sin[P.a] * 10.0; // calculate change in x
	int dy = M.cos[P.a] * 10.0; // calculate change in y
	if (K.w == 1) { P.x += dx; P.y += dy; } // move forward
	if (K.s == 1) { P.x -= dx; P.y -= dy; }

	//strafe left and right
	if (K.sr == 1) { P.x += dy; P.y -= dx; }
	if (K.sl == 1) { P.x -= dy; P.y += dx; }
}

void clearBackground() {
	int x, y;
	for (y = 0; y < SH; y++) {
		for (x = 0; x < SW; x++) {
			pixel(x, y, 6); //clear background 
		}
	}
}
 
void draw3D() { // real sussy baka
	int wx[4], wy[4], wz[4];// world x and y
	float CS = M.cos[P.a]; //player cos and sin
	float SN = M.sin[P.a];

	// offset bottom two points of player
	int x1 = 40 - P.x, y1 = 10 - P.y;
	int x2 = 40 - P.x, y2 = 290 - P.y;

	//world X position
	wx[0] = x1 * CS - y1 * SN;
	wx[1] = x2 * CS - y2 * SN;

	//world Y position
	wy[0] = x1 * SN + y1 * CS;
	wy[1] = x2 * SN + y2 * CS;

	// world Z position

	wz[0] = 0 - P.z;
	wz[1] = 0 - P.z;

	//screen x y position
	wx[0] = wx[0] * 200 / wy[0] + HSW; wy[0] = wz[0] * 200 / wy[0] + HSH;
	wx[1] = wx[1] * 200 / wy[1] + HSW; wy[1] = wz[1] * 200 / wy[1] + HSH;

	//draw points 
	if (wx[0] > 0 && wz[0] < SW && wy[0]>0 && wy[0] < SH) { pixel(wx[0], wy[0], 0); }
	if (wx[1] > 0 && wz[1] < SW && wy[1]>0 && wy[1] < SH) { pixel(wx[1], wy[1], 0); }
}

void display() {
	int x, y;

	if (T.fr1 - T.fr2 >= 50) { //20 fps
		clearBackground();
		movePl();
		draw3D();
		T.fr2 = T.fr1;
		glutSwapBuffers();
		glutReshapeWindow(GSLW, GSLH); // stops from resizeing window
	}
	T.fr1 = glutGet(GLUT_ELAPSED_TIME); 
	glutPostRedisplay();
}


void KeysDown(unsigned char key, int x, int y){
	if (key == 'w') { K.w = 1; }
	if (key == 'a') { K.a = 1; }
	if (key == 's') { K.s = 1; }
	if (key == 'd') { K.d = 1; }
	if (key == ',') { K.sl = 1; }
	if (key == '.') { K.sr = 1; }

}

void KeysUp(unsigned char key, int x, int y) {
	if (key == 'w') { K.w = 0; }
	if (key == 'a') { K.a = 0; }
	if (key == 's') { K.s = 0; }
	if (key == 'd') { K.d = 0; }
	if (key == ',') { K.sl = 0; }
	if (key == '.') { K.sr = 0; }
}

void init() {
	int x;
	for (x = 0; x < 360; x++){
		M.cos[x]=cos(x * 3.14159 / 180);
		M.sin[x]=sin(x * 3.14159 / 180);
	} 
	//init playe
	P.x = 70; P.y = -110; P.z = 20; P.a = 0; P.l = 0;
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


}