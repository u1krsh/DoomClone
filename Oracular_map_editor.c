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

int numText = 19;
int numSect = 0;
int numWall = 0;
