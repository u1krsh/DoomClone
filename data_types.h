#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#define res 1 //resolution scale
#define SH 240*res //screen height
#define SW 320*res //screen width
#define HSH SH/2 //half screen height
#define HSW SW/2 //half screen width
#define pixelScale  4/res //open gl pixel size
#define GSLW SW*pixelScale //open gl window width
#define GSLH SH*pixelScale //open gl window height

typedef struct {
	int fr1, fr2; // these are frame 1 and 2 for a constant frame rate
} DoomTime;

typedef struct
{
	int w, s, a, d;           //move up, down, left, right
	int sl, sr;             //strafe left, right 
	int m;                 //move up, down, look up, down
	int fire;              //fire weapon
	int firePressed;       //track if fire was already pressed (for single shot)
} keys;

typedef struct {
	float cos[360];   //cos and sin values
	float sin[360];
} math;

typedef struct {
	int x, y, z; //position
	int a;// angle of rotation
	int l; // variable to look up and down
} player;

typedef struct {
	int x1, y1;//bottom line point 1
	int x2, y2;//bottom line point 2
	int c;// wall color 
	int wt, u, v; //wall texture and u/v tile
	int shade;             //shade of the wall
} walls;

typedef struct {
	int ws, we; // wall number start and end
	int z1, z2; // height from bottom to top
	int x, y; // center of sector
	int d; // sorting drawing order
	int c1, c2; //bottom and top color
	int surf[SW]; // to hold points for surface
	int surface; //is there a surface to draw
	int ss, st; //surface texture, surface scale
} sectors;

typedef struct {
	int w, h;                             //texture width/height
	const unsigned char* name;           //texture name
} TextureMaps;

#endif // DATA_TYPES_H
