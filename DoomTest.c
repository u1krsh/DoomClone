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
	if (K.w == 1) { printf("forward\n"); }
	if (K.a == 1) { printf("left\n"); }
	if (K.s == 1) { printf("backwards\n"); }
	if (K.d == 1) { printf("right\n"); }

	if (K.sr == 1) { printf("strafe right\n"); }
	if (K.sl == 1) { printf("strafe left\n"); }
}

void clearBackground() {
	int x, y;
	for (y = 0; y < SH; y++) {
		for (x = 0; x < SW; x++) {
			pixel(x, y, 6); //clear background 
		}
	}
}

int tick;

void draw3D() { // real sussy baka
	int x, y, c = 0;
	for (y = 0; y < HSH; y++) {
		for (x = 0; x < HSW; x++) {
			pixel(x, y, c);
			c = c + 1;
			if (c > 6) {
				c = 0;
			}
		}
	}

	tick = tick + 1;
	if (tick > 20) {
		tick = 0;
	}
	pixel(HSW, HSH + tick, 0);
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