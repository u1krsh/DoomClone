#include <math.h>
#include <stdio.h>
#include <GL/glut.h>

#define res 1 //resolition scale
#define SH 120*res //screen height
#define SW 160*res //screen width
#define HSH SH/2 //half screen height
#define HSW SW/2 //half screen width
#define pixelScale  4/res //open gl pixel size
#define GLSW SW*pixelScale //open gl window width
#define GLSH SH*pixelScale //open gl window height
# define M_PI 3.14159265358979323846  /* pi */
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

// Toggle large preview overlay (legacy, now unused)
//static int g_showPreview = 0; // REMOVED overlay flag
static int previewWindow = 0; // second window id (0 = none)
static int mainWindow = 0;    // main window id

//------------------------------------------------------------------------------

typedef struct
{
    int fr1, fr2;           //frame 1 frame 2, to create constant frame rate
}time; time T;

typedef struct
{
    float cos[360];        //Save sin cos in values 0-360 degrees 
    float sin[360];
}math; math M;

typedef struct
{
    int w, s, a, d;           //move up, down, left, rigth
    int sl, sr;             //strafe left, right 
    int m;                 //move up, down, look up, down
}keys; keys K;

typedef struct
{
    int x, y, z;             //player position. Z is up
    int a;                 //player angle of rotation left right
    int l;                 //variable to look up and down
}player; player P;

typedef struct
{
    int x1, y1;             //bottom line point 1
    int x2, y2;             //bottom line point 2
    int wt, u, v;            //wall texture and u/v tile
    int shade;             //shade of the wall
}walls; walls W[256];

typedef struct
{
    int ws, we;             //wall number start and end
    int z1, z2;             //height of bottom and top 
    int d;                 //add y distances to sort drawing order
    int st, ss;             //surface texture, surface scale 
    int surf[SW];          //to hold points for surfaces
}sectors; sectors S[128];

typedef struct
{
    int w, h;                             //texture width/height
    const unsigned char* name;           //texture name
}TexureMaps; TexureMaps Textures[64]; //increase for more textures

typedef struct
{
    int mx, my;        //rounded mouse position
    int addSect;      //0=nothing, 1=add sector
    int wt, wu, wv;     //wall    texture, uv texture tile
    int st, ss;        //surface texture, surface scale 
    int z1, z2;        //bottom and top height
    int scale;        //scale down grid
    int move[4];      //0=wall ID, 1=v1v2, 2=wallID, 3=v1v2
    int selS, selW;    //select sector/wall
}grid; grid G;

//------------------------------------------------------------------------------

void save() //save file
{
    int w, s;
    FILE* fp = fopen("level.h", "w");
    if (fp == NULL) { printf("Error opening the file level.h"); return; }
    if (numSect == 0) { fclose(fp); return; } //nothing, clear file 

    fprintf(fp, "%i\n", numSect); //number of sectors 
    for (s = 0; s < numSect; s++)      //save sector
    {
        fprintf(fp, "%i %i %i %i %i %i\n", S[s].ws, S[s].we, S[s].z1, S[s].z2, S[s].st, S[s].ss);
    }

    fprintf(fp, "%i\n", numWall); //number of walls
    for (w = 0; w < numWall; w++)      //save walls
    {
        fprintf(fp, "%i %i %i %i %i %i %i %i\n", W[w].x1, W[w].y1, W[w].x2, W[w].y2, W[w].wt, W[w].u, W[w].v, W[w].shade);
    }
    fprintf(fp, "\n%i %i %i %i %i\n", P.x, P.y, P.z, P.a, P.l); //player position 
    fclose(fp);
}

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

void initGlobals() 	       //define grid globals
{
    G.scale = 4;                //scale down grid
    G.selS = 0, G.selW = 0;       //select sector, walls
    G.z1 = 0;   G.z2 = 40;        //sector bottom top height
    G.st = 0;   G.ss = 4;         //sector texture, scale
    G.wt = 0;   G.wu = 1; G.wv = 1; //wall texture, u,v
}

void drawPixel(int x, int y, int r, int g, int b) //draw a pixel at x/y with rgb
{
    glColor3ub(r, g, b);
    glBegin(GL_POINTS);
    glVertex2i(x * pixelScale + 2, y * pixelScale + 2);
    glEnd();
}

// Raw pixel (no pixelScale) for preview window
static void drawPixelRaw(int x, int y, int r, int g, int b)
{
    glColor3ub(r, g, b);
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void drawLine(float x1, float y1, float x2, float y2, int r, int g, int b)
{
    int n;
    float x = x2 - x1;
    float y = y2 - y1;
    float max = fabs(x); if (fabs(y) > max) { max = fabs(y); }
    x /= max; y /= max;
    for (n = 0; n < max; n++)
    {
        drawPixel(x1, y1, r, g, b);
        x1 += x; y1 += y;
    }
}

void drawNumber(int nx, int ny, int n)
{
    int x, y;
    for (y = 0; y < 5; y++)
    {
        int y2 = ((5 - y - 1) + 5 * n) * 3 * 12;
        for (x = 0; x < 12; x++)
        {
            int x2 = x * 3;
            if (T_NUMBERS[y2 + x2] == 0) { continue; }
            drawPixel(x + nx, y + ny, 255, 255, 255);
        }
    }
}

// Draw a texture preview in an arbitrary box using drawPixel (scaled by pixelScale) for main window
static void drawTexturePreview(int texIndex, int destX, int destY, int boxW, int boxH)
{
    if (texIndex < 0) return;
    int tw = Textures[texIndex].w;
    int th = Textures[texIndex].h;
    const unsigned char* data = Textures[texIndex].name;
    if (!data || tw <= 0 || th <= 0 || boxW <= 0 || boxH <= 0) return;

    // Scale to fit while preserving aspect ratio
    float sx = (float)boxW / (float)tw;
    float sy = (float)boxH / (float)th;
    float scale = sx < sy ? sx : sy;
    int dvw = (int)floorf(tw * scale);
    int dvh = (int)floorf(th * scale);
    if (dvw < 1) dvw = 1;
    if (dvh < 1) dvh = 1;
    int offx = (boxW - dvw) / 2;
    int offy = (boxH - dvh) / 2;

    for (int py = 0; py < boxH; ++py)
    {
        for (int px = 0; px < boxW; ++px)
        {
            // Only draw inside the centered destination rectangle
            if (px < offx || px >= offx + dvw || py < offy || py >= offy + dvh) { continue; }

            // Normalized coords in [0,1]
            float u = dvw > 1 ? (float)(px - offx) / (float)(dvw - 1) : 0.0f;
            float v = dvh > 1 ? (float)(py - offy) / (float)(dvh - 1) : 0.0f;

            // Map to source texel space
            float sx = u * (float)(tw - 1);
            float sy = v * (float)(th - 1);

            int x0 = (int)floorf(sx);
            int y0 = (int)floorf(sy);
            int x1 = x0 + 1; if (x1 >= tw) x1 = tw - 1;
            int y1 = y0 + 1; if (y1 >= th) y1 = th - 1;
            float ax = sx - (float)x0;
            float ay = sy - (float)y0;

            // Flip vertically because textures are stored top-to-bottom in arrays used elsewhere
            int fy0 = (th - 1) - y0;
            int fy1 = (th - 1) - y1;

            int idx00 = (fy0 * tw + x0) * 3;
            int idx10 = (fy0 * tw + x1) * 3;
            int idx01 = (fy1 * tw + x0) * 3;
            int idx11 = (fy1 * tw + x1) * 3;

            float r00 = data[idx00 + 0], g00 = data[idx00 + 1], b00 = data[idx00 + 2];
            float r10 = data[idx10 + 0], g10 = data[idx10 + 1], b10 = data[idx10 + 2];
            float r01 = data[idx01 + 0], g01 = data[idx01 + 1], b01 = data[idx01 + 2];
            float r11 = data[idx11 + 0], g11 = data[idx11 + 1], b11 = data[idx11 + 2];

            // Bilinear filter
            float i0r = r00 + (r10 - r00) * ax;
            float i0g = g00 + (g10 - g00) * ax;
            float i0b = b00 + (b10 - b00) * ax;
            float i1r = r01 + (r11 - r01) * ax;
            float i1g = g01 + (g11 - g01) * ax;
            float i1b = b01 + (b11 - b01) * ax;
            int r = (int)(i0r + (i1r - i0r) * ay + 0.5f);
            int g = (int)(i0g + (i1g - i0g) * ay + 0.5f);
            int b = (int)(i0b + (i1b - i0b) * ay + 0.5f);

            if (r < 0) r = 0; if (r > 255) r = 255;
            if (g < 0) g = 0; if (g > 255) g = 255;
            if (b < 0) b = 0; if (b > 255) b = 255;

            drawPixel(destX + px, destY + py, r, g, b);
        }
    }
}

// Draw into separate preview window using raw pixels filling up to 512x512 while preserving aspect ratio
static void drawTexturePreviewWindow(int texIndex, int winW, int winH)
{
    if (texIndex < 0) return;
    int tw = Textures[texIndex].w;
    int th = Textures[texIndex].h;
    const unsigned char* data = Textures[texIndex].name;
    if (!data || tw <= 0 || th <= 0) return;

    // Fit texture preserving aspect
    float sx = (float)winW / (float)tw;
    float sy = (float)winH / (float)th;
    float scale = sx < sy ? sx : sy;
    int dvw = (int)floorf(tw * scale);
    int dvh = (int)floorf(th * scale);
    if (dvw < 1) dvw = 1; if (dvh < 1) dvh = 1;
    int offx = (winW - dvw) / 2;
    int offy = (winH - dvh) / 2;

    for (int py = 0; py < dvh; ++py)
    {
        float v = dvh > 1 ? (float)py / (float)(dvh - 1) : 0.0f;
        float syf = v * (float)(th - 1);
        int y0 = (int)syf;
        int y1 = y0 + 1; if (y1 >= th) y1 = th - 1;
        float ay = syf - (float)y0;
        int fy0 = (th - 1) - y0; // vertical flip
        int fy1 = (th - 1) - y1;
        for (int px = 0; px < dvw; ++px)
        {
            float u = dvw > 1 ? (float)px / (float)(dvw - 1) : 0.0f;
            float sxf = u * (float)(tw - 1);
            int x0 = (int)sxf;
            int x1 = x0 + 1; if (x1 >= tw) x1 = tw - 1;
            float ax = sxf - (float)x0;
            int idx00 = (fy0 * tw + x0) * 3;
            int idx10 = (fy0 * tw + x1) * 3;
            int idx01 = (fy1 * tw + x0) * 3;
            int idx11 = (fy1 * tw + x1) * 3;
            float r00 = data[idx00+0], g00 = data[idx00+1], b00 = data[idx00+2];
            float r10 = data[idx10+0], g10 = data[idx10+1], b10 = data[idx10+2];
            float r01 = data[idx01+0], g01 = data[idx01+1], b01 = data[idx01+2];
            float r11 = data[idx11+0], g11 = data[idx11+1], b11 = data[idx11+2];
            float i0r = r00 + (r10 - r00) * ax;
            float i0g = g00 + (g10 - g00) * ax;
            float i0b = b00 + (b10 - b00) * ax;
            float i1r = r01 + (r11 - r01) * ax;
            float i1g = g01 + (g11 - g01) * ax;
            float i1b = b01 + (b11 - b01) * ax;
            int r = (int)(i0r + (i1r - i0r) * ay + 0.5f);
            int g = (int)(i0g + (i1g - i0g) * ay + 0.5f);
            int b = (int)(i0b + (i1b - i0b) * ay + 0.5f);
            drawPixelRaw(offx + px, offy + py, r, g, b);
        }
    }
}

static void previewDisplay()
{
    if (previewWindow == 0) { return; }
    glutSetWindow(previewWindow);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    drawTexturePreviewWindow(G.wt, 512, 512);
    glutSwapBuffers();
    glutPostRedisplay();
    glutSetWindow(mainWindow);
}

static void createPreviewWindow()
{
    if (previewWindow != 0) { return; }
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(GLSW + 40, 40);
    previewWindow = glutCreateWindow("Texture Preview");
    gluOrtho2D(0, 512, 0, 512);
    glPointSize(1); // fine-grained points
    glutDisplayFunc(previewDisplay);
}

static void destroyPreviewWindow()
{
    if (previewWindow == 0) { return; }
    glutDestroyWindow(previewWindow);
    previewWindow = 0;
    glutSetWindow(mainWindow);
}

void draw2D()
{
    int s, w, x, y, c;
    //draw background color
    for (y = 0; y < 120; y++)
    {
        int y2 = (SH - y - 1) * 3 * 160; //invert height, x3 for rgb, x15 for texture width
        for (x = 0; x < 160; x++)
        {
            int pixel = x * 3 + y2;
            int r = oracular_texture[pixel + 0];
            int g = oracular_texture[pixel + 1];
            int b = oracular_texture[pixel + 2];
            if (G.addSect > 0 && y > 48 - 8 && y < 56 - 8 && x>144) { r = r >> 1; g = g >> 1; b = b >> 1; } //darken sector button
            drawPixel(x, y, r, g, b);
        }
    }

    //draw sectors
    for (s = 0; s < numSect; s++)
    {
        for (w = S[s].ws; w < S[s].we; w++)
        {
            if (s == G.selS - 1) //if this sector is selected
            {
                //set sector to globals
                S[G.selS - 1].z1 = G.z1;
                S[G.selS - 1].z2 = G.z2;
                S[G.selS - 1].st = G.st;
                S[G.selS - 1].ss = G.ss;
                //yellow select
                if (G.selW == 0) { c = 80; } //all walls yellow
                else if (G.selW + S[s].ws - 1 == w) { c = 80; W[w].wt = G.wt; W[w].u = G.wu; W[w].v = G.wv; } //one wall selected
                else { c = 0; } //grey walls
            }
            else { c = 0; } //sector not selected, grey

            drawLine(W[w].x1 / G.scale, W[w].y1 / G.scale, W[w].x2 / G.scale, W[w].y2 / G.scale, 128 + c, 128 + c, 128 - c);
            drawPixel(W[w].x1 / G.scale, W[w].y1 / G.scale, 255, 255, 255);
            drawPixel(W[w].x2 / G.scale, W[w].y2 / G.scale, 255, 255, 255);
        }
    }

    //draw player
    int dx = M.sin[P.a] * 12;
    int dy = M.cos[P.a] * 12;
    drawPixel(P.x / G.scale, P.y / G.scale, 0, 255, 0);
    drawPixel((P.x + dx) / G.scale, (P.y + dy) / G.scale, 0, 175, 0);

    // Wall texture preview (15x15 box at 145, 97)
    if (Textures[G.wt].w > 0 && Textures[G.wt].h > 0)
    {
        drawTexturePreview(G.wt, 144, 105 - 8, 15, 15);
    }

    // Surface texture preview (15x15 box at 145, 81)
    if (Textures[G.st].w > 0 && Textures[G.st].h > 0)
    {
        drawTexturePreview(G.st, 144, 105 - 24 - 8, 15, 15);
    }

    //draw numbers
    drawNumber(140, 90, G.wu);   //wall u
    drawNumber(146, 90, G.wv);   //wall v
    drawNumber(146, 66, G.ss);   //surface v
    drawNumber(146, 58, G.z2);   //top height
    drawNumber(146, 50, G.z1);   //bottom height
    drawNumber(146, 26, G.selS); //sector number
    drawNumber(146, 18, G.selW); //wall number
}

//darken buttons
int dark = 0;
void darken()                       //draw a pixel at x/y with rgb
{
    int x, y, xs, xe, ys, ye;
    if (dark == 0) { return; }             //no buttons were clicked
    if (dark == 1) { xs = -3; xe = 15; ys = 0 / G.scale; ye = 32 / G.scale; } //save button
    if (dark == 2) { xs = 0; xe = 3; ys = 96 / G.scale; ye = 128 / G.scale; } //u left
    if (dark == 3) { xs = 4; xe = 8; ys = 96 / G.scale; ye = 128 / G.scale; } //u right
    if (dark == 4) { xs = 7; xe = 11; ys = 96 / G.scale; ye = 128 / G.scale; } //v left
    if (dark == 5) { xs = 11; xe = 15; ys = 96 / G.scale; ye = 128 / G.scale; } //u right
    if (dark == 6) { xs = 0; xe = 8; ys = 192 / G.scale; ye = 224 / G.scale; } //u left
    if (dark == 7) { xs = 8; xe = 15; ys = 192 / G.scale; ye = 224 / G.scale; } //u right
    if (dark == 8) { xs = 0; xe = 7; ys = 224 / G.scale; ye = 256 / G.scale; } //Top left
    if (dark == 9) { xs = 7; xe = 15; ys = 224 / G.scale; ye = 256 / G.scale; } //Top right
    if (dark == 10) { xs = 0; xe = 7; ys = 256 / G.scale; ye = 288 / G.scale; } //Bot left
    if (dark == 11) { xs = 7; xe = 15; ys = 256 / G.scale; ye = 288 / G.scale; } //Bot right
    if (dark == 12) { xs = 0; xe = 7; ys = 352 / G.scale; ye = 386 / G.scale; } //sector left
    if (dark == 13) { xs = 7; xe = 15; ys = 352 / G.scale; ye = 386 / G.scale; } //sector right
    if (dark == 14) { xs = 0; xe = 7; ys = 386 / G.scale; ye = 416 / G.scale; } //wall left
    if (dark == 15) { xs = 7; xe = 15; ys = 386 / G.scale; ye = 416 / G.scale; } //wall right
    if (dark == 16) { xs = -3; xe = 15; ys = 416 / G.scale; ye = 448 / G.scale; } //delete
    if (dark == 17) { xs = -3; xe = 15; ys = 448 / G.scale; ye = 480 / G.scale; } //load

    for (y = ys; y < ye; y++)
    {
        for (x = xs; x < xe; x++)
        {
            glColor4f(0, 0, 0, 0.4);
            glBegin(GL_POINTS);
            glVertex2i(x * pixelScale + 2 + 580, (120 - y) * pixelScale);
            glEnd();
        }
    }
}

void mouse(int button, int state, int x, int y)
{
    int s, w;
    //round mouse x,y
    G.mx = x / pixelScale;
    G.my = SH - y / pixelScale;
    G.mx = ((G.mx + 4) >> 3) << 3;
    G.my = ((G.my + 4) >> 3) << 3; //nearest 8th 

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        //2D view buttons only
        if (x > 580)
        {
            //2d 3d view buttons
            if (y > 0 && y < 32) { save(); dark = 1; }
            //wall texture
            if (y > 32 && y < 96) { if (x < 610) { G.wt -= 1; if (G.wt < 0) { G.wt = numText; } } else { G.wt += 1; if (G.wt > numText) { G.wt = 0; } } }
            //wall uv
            if (y > 96 && y < 128)
            {
                if (x < 595) { dark = 2; G.wu -= 1; if (G.wu < 1) { G.wu = 1; } }
                else if (x < 610) { dark = 3; G.wu += 1; if (G.wu > 9) { G.wu = 9; } }
                else if (x < 625) { dark = 4; G.wv -= 1; if (G.wv < 1) { G.wv = 1; } }
                else if (x < 640) { dark = 5; G.wv += 1; if (G.wv > 9) { G.wv = 9; } }
            }
            //surface texture
            if (y > 128 && y < 192) { if (x < 610) { G.st -= 1; if (G.st < 0) { G.st = numText; } } else { G.st += 1; if (G.st > numText) { G.st = 0; } } }
            //surface uv
            if (y > 192 && y < 222)
            {
                if (x < 610) { dark = 6; G.ss -= 1; if (G.ss < 1) { G.ss = 1; } }
                else { dark = 7; G.ss += 1; if (G.ss > 9) { G.ss = 9; } }
            }
            //top height
            if (y > 222 && y < 256)
            {
                if (x < 610) { dark = 8; G.z2 -= 5; if (G.z2 == G.z1) { G.z1 -= 5; } }
                else { dark = 9; G.z2 += 5; }
            }
            //bot height
            if (y > 256 && y < 288)
            {
                if (x < 610) { dark = 10; G.z1 -= 5; }
                else { dark = 11; G.z1 += 5; if (G.z1 == G.z2) { G.z2 += 5; } }
            }
            //add sector
            if (y > 288 && y < 318) { G.addSect += 1; G.selS = 0; G.selW = 0; if (G.addSect > 1) { G.addSect = 0; } }
            //limit
            if (G.z1 < 0) { G.z1 = 0; } if (G.z1 > 145) { G.z1 = 145; }
            if (G.z2 < 5) { G.z2 = 5; } if (G.z2 > 150) { G.z2 = 150; }

            //select sector
            if (y > 352 && y < 386)
            {
                G.selW = 0;
                if (x < 610) { dark = 12; G.selS -= 1; if (G.selS < 0) { G.selS = numSect; } }
                else { dark = 13; G.selS += 1; if (G.selS > numSect) { G.selS = 0; } }
                int s = G.selS - 1;
                G.z1 = S[s].z1; //sector bottom height
                G.z2 = S[s].z2; //sector top height
                G.st = S[s].st; //surface texture
                G.ss = S[s].ss; //surface scale
                G.wt = W[S[s].ws].wt;
                G.wu = W[S[s].ws].u;
                G.wv = W[S[s].ws].v;
                if (G.selS == 0) { initGlobals(); } //defaults 
            }
            //select sector's walls 
            int snw = S[G.selS - 1].we - S[G.selS - 1].ws; //sector's number of walls
            if (y > 386 && y < 416)
            {
                if (x < 610) //select sector wall left
                {
                    dark = 14;
                    G.selW -= 1; if (G.selW < 0) { G.selW = snw; }
                }
                else //select sector wall right
                {
                    dark = 15;
                    G.selW += 1; if (G.selW > snw) { G.selW = 0; }
                }
                if (G.selW > 0)
                {
                    G.wt = W[S[G.selS - 1].ws + G.selW - 1].wt; //printf("ws,%i,%i\n",G.wt, 1 );
                    G.wu = W[S[G.selS - 1].ws + G.selW - 1].u;
                    G.wv = W[S[G.selS - 1].ws + G.selW - 1].v;
                }
            }
            //delete
            if (y > 416 && y < 448)
            {
                dark = 16;
                if (G.selS > 0)
                {
                    int d = G.selS - 1;                             //delete this one
                    //printf("%i before:%i,%i\n",d, numSect,numWall);
                    numWall -= (S[d].we - S[d].ws);                 //first subtract number of walls
                    for (x = d; x < numSect; x++) { S[x] = S[x + 1]; }       //remove from array
                    numSect -= 1;                                 //1 less sector
                    G.selS = 0; G.selW = 0;                         //deselect
                    //printf("after:%i,%i\n\n",numSect,numWall);
                }
            }

            //load
            if (y > 448 && y < 480) { dark = 17; load(); }
        }

        //clicked on grid
        else
        {
            //init new sector
            if (G.addSect == 1)
            {
                S[numSect].ws = numWall;                                   //clear wall start
                S[numSect].we = numWall + 1;                                 //add 1 to wall end
                S[numSect].z1 = G.z1;
                S[numSect].z2 = G.z2;
                S[numSect].st = G.st;
                S[numSect].ss = G.ss;
                W[numWall].x1 = G.mx * G.scale; W[numWall].y1 = G.my * G.scale;  //x1,y1 
                W[numWall].x2 = G.mx * G.scale; W[numWall].y2 = G.my * G.scale;  //x2,y2
                W[numWall].wt = G.wt;
                W[numWall].u = G.wu;
                W[numWall].v = G.wv;
                numWall += 1;                                              //add 1 wall
                numSect += 1;                                              //add this sector
                G.addSect = 3;                                             //go to point 2
            }

            //add point 2
            else if (G.addSect == 3)
            {
                if (S[numSect - 1].ws == numWall - 1 && G.mx * G.scale <= W[S[numSect - 1].ws].x1)
                {
                    numWall -= 1; numSect -= 1; G.addSect = 0;
                    printf("walls must be counter clockwise\n");
                    return;
                }

                //point 2
                W[numWall - 1].x2 = G.mx * G.scale; W[numWall - 1].y2 = G.my * G.scale; //x2,y2
                //automatic shading 
                float ang = atan2f(W[numWall - 1].x2 - W[numWall - 1].x1, W[numWall - 1].y2 - W[numWall - 1].y1);
                ang = (ang * 180) / M_PI;      //radians to degrees
                if (ang < 0) { ang += 360; }    //correct negative
                int shade = ang;           //shading goes from 0-90-0-90-0
                if (shade > 180) { shade = 180 - (shade - 180); }
                if (shade > 90) { shade = 90 - (shade - 90); }
                W[numWall - 1].shade = shade;

                //check if sector is closed
                if (W[numWall - 1].x2 == W[S[numSect - 1].ws].x1 && W[numWall - 1].y2 == W[S[numSect - 1].ws].y1)
                {
                    W[numWall - 1].wt = G.wt;
                    W[numWall - 1].u = G.wu;
                    W[numWall - 1].v = G.wv;
                    G.addSect = 0;
                }
                //not closed, add new wall
                else
                {
                    //init next wall
                    S[numSect - 1].we += 1;                                      //add 1 to wall end
                    W[numWall].x1 = G.mx * G.scale; W[numWall].y1 = G.my * G.scale;  //x1,y1 
                    W[numWall].x2 = G.mx * G.scale; W[numWall].y2 = G.my * G.scale;  //x2,y2
                    W[numWall - 1].wt = G.wt;
                    W[numWall - 1].u = G.wu;
                    W[numWall - 1].v = G.wv;
                    W[numWall].shade = 0;
                    numWall += 1;                                              //add 1 wall
                }
            }
        }
    }

    //clear variables to move point
    for (w = 0; w < 4; w++) { G.move[w] = -1; }

    if (G.addSect == 0 && button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    {
        //move point hold id 
        for (s = 0; s < numSect; s++)
        {
            for (w = S[s].ws; w < S[s].we; w++)
            {
                int x1 = W[w].x1, y1 = W[w].y1;
                int x2 = W[w].x2, y2 = W[w].y2;
                int mx = G.mx * G.scale, my = G.my * G.scale;
                if (mx<x1 + 3 && mx>x1 - 3 && my<y1 + 3 && my>y1 - 3) { G.move[0] = w; G.move[1] = 1; }
                if (mx<x2 + 3 && mx>x2 - 3 && my<y2 + 3 && my>y2 - 3) { G.move[2] = w; G.move[3] = 2; }
            }
        }
    }

    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) { dark = 0; }
}

void mouseMoving(int x, int y)
{
    if (x<580 && G.addSect == 0 && G.move[0]>-1)
    {
        int Aw = G.move[0], Ax = G.move[1];
        int Bw = G.move[2], Bx = G.move[3];
        if (Ax == 1) { W[Aw].x1 = ((x + 16) >> 5) << 5; W[Aw].y1 = ((GLSH - y + 16) >> 5) << 5; }
        if (Ax == 2) { W[Aw].x2 = ((x + 16) >> 5) << 5; W[Aw].y2 = ((GLSH - y + 16) >> 5) << 5; }
        if (Bx == 1) { W[Bw].x1 = ((x + 16) >> 5) << 5; W[Bw].y1 = ((GLSH - y + 16) >> 5) << 5; }
        if (Bx == 2) { W[Bw].x2 = ((x + 16) >> 5) << 5; W[Bw].y2 = ((GLSH - y + 16) >> 5) << 5; }
    }
}

void KeysDown(unsigned char key, int x, int y)
{
    if (key == 'w') { K.w = 1; }
    if (key == 's') { K.s = 1; }
    if (key == 'a') { K.a = 1; }
    if (key == 'd') { K.d = 1; }
    if (key == 'm') { K.m = 1; }
    if (key == ',') { K.sr = 1; }
    if (key == '.') { K.sl = 1; }
    if (key == 'p')
    {
        if (previewWindow == 0) { createPreviewWindow(); }
        else { destroyPreviewWindow(); }
    }
}
void KeysUp(unsigned char key, int x, int y)
{
    if (key == 'w') { K.w = 0; }
    if (key == 's') { K.s = 0; }
    if (key == 'a') { K.a = 0; }
    if (key == 'd') { K.d = 0; }
    if (key == 'm') { K.m = 0; }
    if (key == ',') { K.sr = 0; }
    if (key == '.') { K.sl = 0; }
}

void movePlayer()
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

void display()
{
    int x, y;
    if (T.fr1 - T.fr2 >= 50)                        //only draw 20 frames/second
    {
        movePlayer();
        draw2D();
        darken();

        T.fr2 = T.fr1;
        glutSwapBuffers();
        glutReshapeWindow(GLSW, GLSH);             //prevent window scaling
    }

    T.fr1 = glutGet(GLUT_ELAPSED_TIME);          //1000 Milliseconds per second
    glutPostRedisplay();
}

int shade(int w)    //automatic shading
{
    float ang = atan2f(W[w].y2 - W[w].y1, W[w].x2 - W[w].x1);
    ang = (ang * 180) / M_PI;      //radians to degrees
    if (ang < 0) { ang += 360; }    //correct negative
    int shade = ang;           //shading goes from 0-90-0-90-0
    if (shade > 180) { shade = 180 - (shade - 180); }
    if (shade > 90) { shade = 90 - (shade - 90); }
    return shade * 0.75;
}

void init()
{
    int x;
    initGlobals();

    //init player
    P.x = 32 * 9; P.y = 48; P.z = 30; P.a = 0; P.l = 0;    //init player variables

    //store sin/cos in degrees
    for (x = 0; x < 360; x++)                         //precalulate sin cos in degrees
    {
        M.cos[x] = cos(x / 180.0 * M_PI);
        M.sin[x] = sin(x / 180.0 * M_PI);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

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

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(GLSW / 2, GLSH / 2);
    glutInitWindowSize(GLSW, GLSH);
    mainWindow = glutCreateWindow("Oracular Map Edit"); // single main window
    glPointSize(pixelScale);
    gluOrtho2D(0, GLSW, 0, GLSH);
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(KeysDown);
    glutKeyboardUpFunc(KeysUp);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMoving);
    glutMainLoop();
    return 0;
}
