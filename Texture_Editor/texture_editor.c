#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <math.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_TEXTURE_SIZE 256
#define MAX_FRAMES 32
#define MAX_FILENAME 256
#define PALETTE_SIZE 32

// Tool types
typedef enum {
    TOOL_PENCIL,
    TOOL_ERASER,
    TOOL_FILL,
    TOOL_PICKER,
    TOOL_LINE,
    TOOL_RECT,
    TOOL_CIRCLE
} ToolType;

// Color structure
typedef struct {
    unsigned char r, g, b;
} Color;

// Texture frame structure
typedef struct {
    unsigned char* data;  // RGB data
    int width;
    int height;
    int duration;  // Duration in milliseconds for animation
} TextureFrame;

// Animation structure
typedef struct {
    TextureFrame frames[MAX_FRAMES];
    int frameCount;
    int currentFrame;
    int isAnimated;
    char name[MAX_FILENAME];
} Texture;

// Editor state
typedef struct {
    Texture texture;
    Color currentColor;
    Color palette[PALETTE_SIZE];
    ToolType currentTool;
    int brushSize;
    int gridEnabled;
    int previewEnabled;
    int zoom;
    int canvasOffsetX;
    int canvasOffsetY;
    int isDragging;
    int lastX, lastY;
    int lineStartX, lineStartY;
    int isDrawingShape;
    int textureWidth;
    int textureHeight;
    int selectedPaletteIndex;
    int playingAnimation;
    int animationTimer;
} EditorState;

EditorState editor;

// UI regions
#define CANVAS_X 200
#define CANVAS_Y 100
#define CANVAS_WIDTH 600
#define CANVAS_HEIGHT 500
#define TOOLBAR_WIDTH 180
#define PALETTE_Y 620

// Function declarations
void init();
void display();
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keyboard(unsigned char key, int x, int y);
void timer(int value);
void drawPixel(int px, int py, Color color);
void floodFill(int x, int y, Color targetColor, Color fillColor);
void drawLine(int x0, int y0, int x1, int y1, Color color);
void drawRectangle(int x0, int y0, int x1, int y1, Color color);
void drawCircle(int cx, int cy, int radius, Color color);
void drawUI();
void drawCanvas();
void drawToolbar();
void drawPalette();
void drawFrameTimeline();
void saveTexture(const char* filename);
void loadTexture(const char* filename);
void exportCHeader(const char* filename);
void newTexture(int width, int height);
void addFrame();
void deleteFrame(int index);
void duplicateFrame(int index);
void clearCanvas();
Color getPixelColor(int x, int y);
void setPixelColor(int x, int y, Color color);
int isColorEqual(Color a, Color b);
void drawText(float x, float y, const char* text);
void screenToCanvas(int sx, int sy, int* cx, int* cy);
int isInCanvas(int x, int y);
int isInPalette(int x, int y);
int isInToolbar(int x, int y);

void init() {
    // Initialize editor state
    editor.textureWidth = 64;
    editor.textureHeight = 64;
    editor.currentTool = TOOL_PENCIL;
    editor.brushSize = 1;
    editor.gridEnabled = 1;
    editor.previewEnabled = 1;
    editor.zoom = 8;
    editor.canvasOffsetX = 0;
    editor.canvasOffsetY = 0;
    editor.isDragging = 0;
    editor.isDrawingShape = 0;
    editor.selectedPaletteIndex = 0;
    editor.playingAnimation = 0;
    editor.animationTimer = 0;
    
    // Initialize current color (white)
    editor.currentColor.r = 255;
    editor.currentColor.g = 255;
    editor.currentColor.b = 255;
    
    // Initialize default palette (doom-like colors)
    Color defaultPalette[] = {
        {255, 255, 255}, {0, 0, 0}, {255, 0, 0}, {0, 255, 0},
        {0, 0, 255}, {255, 255, 0}, {255, 0, 255}, {0, 255, 255},
        {128, 128, 128}, {192, 192, 192}, {128, 0, 0}, {0, 128, 0},
        {0, 0, 128}, {128, 128, 0}, {128, 0, 128}, {0, 128, 128},
        {64, 64, 64}, {255, 128, 0}, {255, 128, 128}, {128, 255, 128},
        {128, 128, 255}, {200, 200, 200}, {100, 100, 100}, {150, 75, 0},
        {75, 0, 130}, {255, 165, 0}, {255, 192, 203}, {165, 42, 42},
        {210, 105, 30}, {139, 69, 19}, {85, 107, 47}, {47, 79, 79}
    };
    
    for (int i = 0; i < PALETTE_SIZE; i++) {
        editor.palette[i] = defaultPalette[i];
    }
    
    // Initialize texture
    strcpy(editor.texture.name, "untitled");
    editor.texture.frameCount = 1;
    editor.texture.currentFrame = 0;
    editor.texture.isAnimated = 0;
    
    // Create first frame
    newTexture(editor.textureWidth, editor.textureHeight);
}

void newTexture(int width, int height) {
    editor.textureWidth = width;
    editor.textureHeight = height;
    
    // Clear existing frames
    for (int i = 0; i < editor.texture.frameCount; i++) {
        if (editor.texture.frames[i].data) {
            free(editor.texture.frames[i].data);
        }
    }
    
    // Create new frame
    editor.texture.frameCount = 1;
    editor.texture.currentFrame = 0;
    editor.texture.frames[0].width = width;
    editor.texture.frames[0].height = height;
    editor.texture.frames[0].duration = 100; // 100ms default
    editor.texture.frames[0].data = (unsigned char*)calloc(width * height * 3, sizeof(unsigned char));
    
    // Fill with black
    memset(editor.texture.frames[0].data, 0, width * height * 3);
}

void drawPixel(int px, int py, Color color) {
    TextureFrame* frame = &editor.texture.frames[editor.texture.currentFrame];
    
    if (px < 0 || px >= frame->width || py < 0 || py >= frame->height) return;
    
    int index = (py * frame->width + px) * 3;
    frame->data[index] = color.r;
    frame->data[index + 1] = color.g;
    frame->data[index + 2] = color.b;
}

Color getPixelColor(int x, int y) {
    TextureFrame* frame = &editor.texture.frames[editor.texture.currentFrame];
    Color color = {0, 0, 0};
    
    if (x < 0 || x >= frame->width || y < 0 || y >= frame->height) return color;
    
    int index = (y * frame->width + x) * 3;
    color.r = frame->data[index];
    color.g = frame->data[index + 1];
    color.b = frame->data[index + 2];
    
    return color;
}

void setPixelColor(int x, int y, Color color) {
    drawPixel(x, y, color);
}

int isColorEqual(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

void floodFill(int x, int y, Color targetColor, Color fillColor) {
    TextureFrame* frame = &editor.texture.frames[editor.texture.currentFrame];
    
    if (x < 0 || x >= frame->width || y < 0 || y >= frame->height) return;
    if (isColorEqual(targetColor, fillColor)) return;
    
    Color currentColor = getPixelColor(x, y);
    if (!isColorEqual(currentColor, targetColor)) return;
    
    // Simple recursive flood fill (for small textures)
    setPixelColor(x, y, fillColor);
    
    floodFill(x + 1, y, targetColor, fillColor);
    floodFill(x - 1, y, targetColor, fillColor);
    floodFill(x, y + 1, targetColor, fillColor);
    floodFill(x, y - 1, targetColor, fillColor);
}

void drawLine(int x0, int y0, int x1, int y1, Color color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        drawPixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void drawRectangle(int x0, int y0, int x1, int y1, Color color) {
    int minX = x0 < x1 ? x0 : x1;
    int maxX = x0 > x1 ? x0 : x1;
    int minY = y0 < y1 ? y0 : y1;
    int maxY = y0 > y1 ? y0 : y1;
    
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            drawPixel(x, y, color);
        }
    }
}

void drawCircle(int cx, int cy, int radius, Color color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                drawPixel(cx + x, cy + y, color);
            }
        }
    }
}

void clearCanvas() {
    TextureFrame* frame = &editor.texture.frames[editor.texture.currentFrame];
    memset(frame->data, 0, frame->width * frame->height * 3);
}

void addFrame() {
    if (editor.texture.frameCount >= MAX_FRAMES) return;
    
    int newIndex = editor.texture.frameCount;
    editor.texture.frames[newIndex].width = editor.textureWidth;
    editor.texture.frames[newIndex].height = editor.textureHeight;
    editor.texture.frames[newIndex].duration = 100;
    editor.texture.frames[newIndex].data = (unsigned char*)calloc(
        editor.textureWidth * editor.textureHeight * 3, sizeof(unsigned char));
    
    editor.texture.frameCount++;
    editor.texture.isAnimated = editor.texture.frameCount > 1;
}

void deleteFrame(int index) {
    if (editor.texture.frameCount <= 1) return;
    if (index < 0 || index >= editor.texture.frameCount) return;
    
    free(editor.texture.frames[index].data);
    
    // Shift frames
    for (int i = index; i < editor.texture.frameCount - 1; i++) {
        editor.texture.frames[i] = editor.texture.frames[i + 1];
    }
    
    editor.texture.frameCount--;
    if (editor.texture.currentFrame >= editor.texture.frameCount) {
        editor.texture.currentFrame = editor.texture.frameCount - 1;
    }
    
    editor.texture.isAnimated = editor.texture.frameCount > 1;
}

void duplicateFrame(int index) {
    if (editor.texture.frameCount >= MAX_FRAMES) return;
    if (index < 0 || index >= editor.texture.frameCount) return;
    
    int newIndex = editor.texture.frameCount;
    TextureFrame* src = &editor.texture.frames[index];
    TextureFrame* dst = &editor.texture.frames[newIndex];
    
    dst->width = src->width;
    dst->height = src->height;
    dst->duration = src->duration;
    dst->data = (unsigned char*)malloc(src->width * src->height * 3);
    memcpy(dst->data, src->data, src->width * src->height * 3);
    
    editor.texture.frameCount++;
    editor.texture.isAnimated = editor.texture.frameCount > 1;
}

void screenToCanvas(int sx, int sy, int* cx, int* cy) {
    *cx = (sx - CANVAS_X - editor.canvasOffsetX) / editor.zoom;
    *cy = (sy - CANVAS_Y - editor.canvasOffsetY) / editor.zoom;
}

int isInCanvas(int x, int y) {
    return x >= CANVAS_X && x < CANVAS_X + CANVAS_WIDTH &&
           y >= CANVAS_Y && y < CANVAS_Y + CANVAS_HEIGHT;
}

int isInPalette(int x, int y) {
    return x >= 20 && x < 20 + PALETTE_SIZE * 20 &&
           y >= PALETTE_Y && y < PALETTE_Y + 40;
}

int isInToolbar(int x, int y) {
    return x >= 10 && x < 10 + TOOLBAR_WIDTH &&
           y >= 20 && y < 600;
}

void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *text);
        text++;
    }
}

void drawCanvas() {
    TextureFrame* frame = &editor.texture.frames[editor.texture.currentFrame];
    
    // Draw canvas background
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(CANVAS_X, CANVAS_Y);
    glVertex2f(CANVAS_X + CANVAS_WIDTH, CANVAS_Y);
    glVertex2f(CANVAS_X + CANVAS_WIDTH, CANVAS_Y + CANVAS_HEIGHT);
    glVertex2f(CANVAS_X, CANVAS_Y + CANVAS_HEIGHT);
    glEnd();
    
    // Draw texture pixels
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_2D);
    
    for (int y = 0; y < frame->height; y++) {
        for (int x = 0; x < frame->width; x++) {
            int index = (y * frame->width + x) * 3;
            float r = frame->data[index] / 255.0f;
            float g = frame->data[index + 1] / 255.0f;
            float b = frame->data[index + 2] / 255.0f;
            
            int px = CANVAS_X + editor.canvasOffsetX + x * editor.zoom;
            int py = CANVAS_Y + editor.canvasOffsetY + y * editor.zoom;
            
            glColor3f(r, g, b);
            glBegin(GL_QUADS);
            glVertex2f(px, py);
            glVertex2f(px + editor.zoom, py);
            glVertex2f(px + editor.zoom, py + editor.zoom);
            glVertex2f(px, py + editor.zoom);
            glEnd();
        }
    }
    
    // Draw grid
    if (editor.gridEnabled) {
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINES);
        for (int x = 0; x <= frame->width; x++) {
            int px = CANVAS_X + editor.canvasOffsetX + x * editor.zoom;
            glVertex2f(px, CANVAS_Y + editor.canvasOffsetY);
            glVertex2f(px, CANVAS_Y + editor.canvasOffsetY + frame->height * editor.zoom);
        }
        for (int y = 0; y <= frame->height; y++) {
            int py = CANVAS_Y + editor.canvasOffsetY + y * editor.zoom;
            glVertex2f(CANVAS_X + editor.canvasOffsetX, py);
            glVertex2f(CANVAS_X + editor.canvasOffsetX + frame->width * editor.zoom, py);
        }
        glEnd();
    }
    
    // Draw canvas border
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(CANVAS_X, CANVAS_Y);
    glVertex2f(CANVAS_X + CANVAS_WIDTH, CANVAS_Y);
    glVertex2f(CANVAS_X + CANVAS_WIDTH, CANVAS_Y + CANVAS_HEIGHT);
    glVertex2f(CANVAS_X, CANVAS_Y + CANVAS_HEIGHT);
    glEnd();
    glLineWidth(1);
}

void drawToolbar() {
    // Background
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(10, 20);
    glVertex2f(10 + TOOLBAR_WIDTH, 20);
    glVertex2f(10 + TOOLBAR_WIDTH, 600);
    glVertex2f(10, 600);
    glEnd();
    
    // Title
    glColor3f(1, 1, 1);
    drawText(15, 590, "TEXTURE EDITOR");
    
    // Tool buttons
    const char* tools[] = {"Pencil", "Eraser", "Fill", "Picker", "Line", "Rect", "Circle"};
    for (int i = 0; i < 7; i++) {
        if (editor.currentTool == i) {
            glColor3f(0.4f, 0.6f, 0.9f);
        } else {
            glColor3f(0.25f, 0.25f, 0.25f);
        }
        
        glBegin(GL_QUADS);
        glVertex2f(15, 560 - i * 30);
        glVertex2f(175, 560 - i * 30);
        glVertex2f(175, 540 - i * 30);
        glVertex2f(15, 540 - i * 30);
        glEnd();
        
        glColor3f(1, 1, 1);
        drawText(20, 548 - i * 30, tools[i]);
    }
    
    // Brush size
    glColor3f(1, 1, 1);
    char brushText[32];
    sprintf(brushText, "Brush: %d", editor.brushSize);
    drawText(15, 330, brushText);
    
    // Zoom
    char zoomText[32];
    sprintf(zoomText, "Zoom: %dx", editor.zoom);
    drawText(15, 310, zoomText);
    
    // Current color
    glColor3f(1, 1, 1);
    drawText(15, 290, "Current Color:");
    glColor3f(editor.currentColor.r / 255.0f,
              editor.currentColor.g / 255.0f,
              editor.currentColor.b / 255.0f);
    glBegin(GL_QUADS);
    glVertex2f(15, 250);
    glVertex2f(175, 250);
    glVertex2f(175, 270);
    glVertex2f(15, 270);
    glEnd();
    
    // Frame info
    glColor3f(1, 1, 1);
    char frameText[64];
    sprintf(frameText, "Frame: %d/%d", editor.texture.currentFrame + 1, editor.texture.frameCount);
    drawText(15, 230, frameText);
    
    // Controls
    glColor3f(0.7f, 0.7f, 0.7f);
    drawText(15, 200, "Controls:");
    drawText(15, 185, "N - New");
    drawText(15, 170, "S - Save");
    drawText(15, 155, "L - Load");
    drawText(15, 140, "E - Export .h");
    drawText(15, 125, "A - Add Frame");
    drawText(15, 110, "D - Delete Frame");
    drawText(15, 95, "C - Duplicate Frame");
    drawText(15, 80, "Space - Play/Pause");
    drawText(15, 65, "G - Toggle Grid");
    drawText(15, 50, "+/- - Zoom");
    drawText(15, 35, "[ / ] - Brush Size");
}

void drawPalette() {
    glColor3f(1, 1, 1);
    drawText(20, PALETTE_Y + 55, "Palette:");
    
    for (int i = 0; i < PALETTE_SIZE; i++) {
        Color c = editor.palette[i];
        glColor3f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
        
        glBegin(GL_QUADS);
        glVertex2f(20 + i * 20, PALETTE_Y);
        glVertex2f(40 + i * 20, PALETTE_Y);
        glVertex2f(40 + i * 20, PALETTE_Y + 20);
        glVertex2f(20 + i * 20, PALETTE_Y + 20);
        glEnd();
        
        // Highlight selected
        if (i == editor.selectedPaletteIndex) {
            glColor3f(1, 1, 0);
            glLineWidth(2);
            glBegin(GL_LINE_LOOP);
            glVertex2f(20 + i * 20, PALETTE_Y);
            glVertex2f(40 + i * 20, PALETTE_Y);
            glVertex2f(40 + i * 20, PALETTE_Y + 20);
            glVertex2f(20 + i * 20, PALETTE_Y + 20);
            glEnd();
            glLineWidth(1);
        }
    }
}

void drawFrameTimeline() {
    if (!editor.texture.isAnimated) return;
    
    glColor3f(1, 1, 1);
    drawText(820, 650, "Timeline:");
    
    for (int i = 0; i < editor.texture.frameCount; i++) {
        if (i == editor.texture.currentFrame) {
            glColor3f(0.4f, 0.6f, 0.9f);
        } else {
            glColor3f(0.3f, 0.3f, 0.3f);
        }
        
        glBegin(GL_QUADS);
        glVertex2f(820 + i * 60, 620);
        glVertex2f(870 + i * 60, 620);
        glVertex2f(870 + i * 60, 640);
        glVertex2f(820 + i * 60, 640);
        glEnd();
        
        glColor3f(1, 1, 1);
        char frameNum[4];
        sprintf(frameNum, "%d", i + 1);
        drawText(840 + i * 60, 628, frameNum);
    }
}

void drawUI() {
    drawCanvas();
    drawToolbar();
    drawPalette();
    drawFrameTimeline();
}

void display() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    drawUI();
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    y = WINDOW_HEIGHT - y; // Flip Y coordinate
    
    if (state == GLUT_DOWN) {
        // Check palette
        if (isInPalette(x, y)) {
            int index = (x - 20) / 20;
            if (index >= 0 && index < PALETTE_SIZE) {
                editor.selectedPaletteIndex = index;
                editor.currentColor = editor.palette[index];
            }
            return;
        }
        
        // Check toolbar
        if (isInToolbar(x, y)) {
            int toolY = 560;
            for (int i = 0; i < 7; i++) {
                if (y >= 540 - i * 30 && y <= 560 - i * 30) {
                    editor.currentTool = i;
                    break;
                }
            }
            return;
        }
        
        // Canvas interaction
        if (isInCanvas(x, y)) {
            int cx, cy;
            screenToCanvas(x, y, &cx, &cy);
            
            editor.isDragging = 1;
            editor.lastX = cx;
            editor.lastY = cy;
            
            switch (editor.currentTool) {
                case TOOL_PENCIL:
                    for (int dy = -editor.brushSize/2; dy <= editor.brushSize/2; dy++) {
                        for (int dx = -editor.brushSize/2; dx <= editor.brushSize/2; dx++) {
                            if (dx*dx + dy*dy <= (editor.brushSize/2)*(editor.brushSize/2)) {
                                drawPixel(cx + dx, cy + dy, editor.currentColor);
                            }
                        }
                    }
                    break;
                    
                case TOOL_ERASER:
                    {
                        Color black = {0, 0, 0};
                        for (int dy = -editor.brushSize/2; dy <= editor.brushSize/2; dy++) {
                            for (int dx = -editor.brushSize/2; dx <= editor.brushSize/2; dx++) {
                                if (dx*dx + dy*dy <= (editor.brushSize/2)*(editor.brushSize/2)) {
                                    drawPixel(cx + dx, cy + dy, black);
                                }
                            }
                        }
                    }
                    break;
                    
                case TOOL_FILL:
                    {
                        Color targetColor = getPixelColor(cx, cy);
                        floodFill(cx, cy, targetColor, editor.currentColor);
                    }
                    break;
                    
                case TOOL_PICKER:
                    editor.currentColor = getPixelColor(cx, cy);
                    break;
                    
                case TOOL_LINE:
                case TOOL_RECT:
                case TOOL_CIRCLE:
                    editor.isDrawingShape = 1;
                    editor.lineStartX = cx;
                    editor.lineStartY = cy;
                    break;
            }
        }
    } else if (state == GLUT_UP) {
        if (editor.isDrawingShape) {
            int cx, cy;
            screenToCanvas(x, y, &cx, &cy);
            
            if (editor.currentTool == TOOL_LINE) {
                drawLine(editor.lineStartX, editor.lineStartY, cx, cy, editor.currentColor);
            } else if (editor.currentTool == TOOL_RECT) {
                drawRectangle(editor.lineStartX, editor.lineStartY, cx, cy, editor.currentColor);
            } else if (editor.currentTool == TOOL_CIRCLE) {
                int radius = (int)sqrt((cx - editor.lineStartX) * (cx - editor.lineStartX) +
                                      (cy - editor.lineStartY) * (cy - editor.lineStartY));
                drawCircle(editor.lineStartX, editor.lineStartY, radius, editor.currentColor);
            }
            
            editor.isDrawingShape = 0;
        }
        editor.isDragging = 0;
    }
    
    glutPostRedisplay();
}

void motion(int x, int y) {
    y = WINDOW_HEIGHT - y;
    
    if (editor.isDragging && isInCanvas(x, y)) {
        int cx, cy;
        screenToCanvas(x, y, &cx, &cy);
        
        if (editor.currentTool == TOOL_PENCIL) {
            // Draw line from last position
            drawLine(editor.lastX, editor.lastY, cx, cy, editor.currentColor);
            editor.lastX = cx;
            editor.lastY = cy;
        } else if (editor.currentTool == TOOL_ERASER) {
            Color black = {0, 0, 0};
            drawLine(editor.lastX, editor.lastY, cx, cy, black);
            editor.lastX = cx;
            editor.lastY = cy;
        }
        
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'n':
        case 'N':
            newTexture(64, 64);
            printf("Created new 64x64 texture\n");
            break;
            
        case 's':
        case 'S':
            saveTexture("texture.dat");
            printf("Saved texture\n");
            break;
            
        case 'l':
        case 'L':
            loadTexture("texture.dat");
            printf("Loaded texture\n");
            break;
            
        case 'e':
        case 'E':
            exportCHeader("texture_export.h");
            printf("Exported to C header\n");
            break;
            
        case 'a':
        case 'A':
            addFrame();
            printf("Added frame\n");
            break;
            
        case 'd':
        case 'D':
            deleteFrame(editor.texture.currentFrame);
            printf("Deleted frame\n");
            break;
            
        case 'c':
        case 'C':
            duplicateFrame(editor.texture.currentFrame);
            printf("Duplicated frame\n");
            break;
            
        case ' ':
            editor.playingAnimation = !editor.playingAnimation;
            printf("Animation %s\n", editor.playingAnimation ? "playing" : "paused");
            break;
            
        case 'g':
        case 'G':
            editor.gridEnabled = !editor.gridEnabled;
            break;
            
        case '+':
        case '=':
            if (editor.zoom < 16) editor.zoom++;
            break;
            
        case '-':
        case '_':
            if (editor.zoom > 1) editor.zoom--;
            break;
            
        case '[':
            if (editor.brushSize > 1) editor.brushSize--;
            break;
            
        case ']':
            if (editor.brushSize < 20) editor.brushSize++;
            break;
            
        case ',':
        case '<':
            if (editor.texture.currentFrame > 0) {
                editor.texture.currentFrame--;
            }
            break;
            
        case '.':
        case '>':
            if (editor.texture.currentFrame < editor.texture.frameCount - 1) {
                editor.texture.currentFrame++;
            }
            break;
            
        case 27: // ESC
            exit(0);
            break;
    }
    
    glutPostRedisplay();
}

void timer(int value) {
    if (editor.playingAnimation && editor.texture.frameCount > 1) {
        editor.animationTimer += 16; // ~60fps
        
        TextureFrame* currentFrame = &editor.texture.frames[editor.texture.currentFrame];
        if (editor.animationTimer >= currentFrame->duration) {
            editor.animationTimer = 0;
            editor.texture.currentFrame = (editor.texture.currentFrame + 1) % editor.texture.frameCount;
            glutPostRedisplay();
        }
    }
    
    glutTimerFunc(16, timer, 0);
}

void saveTexture(const char* filename) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        printf("Error: Cannot open file for writing\n");
        return;
    }
    
    // Write header
    fwrite(&editor.textureWidth, sizeof(int), 1, fp);
    fwrite(&editor.textureHeight, sizeof(int), 1, fp);
    fwrite(&editor.texture.frameCount, sizeof(int), 1, fp);
    fwrite(&editor.texture.isAnimated, sizeof(int), 1, fp);
    fwrite(editor.texture.name, sizeof(char), MAX_FILENAME, fp);
    
    // Write frames
    for (int i = 0; i < editor.texture.frameCount; i++) {
        TextureFrame* frame = &editor.texture.frames[i];
        fwrite(&frame->duration, sizeof(int), 1, fp);
        fwrite(frame->data, sizeof(unsigned char), frame->width * frame->height * 3, fp);
    }
    
    fclose(fp);
}

void loadTexture(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Error: Cannot open file '%s' for reading\n", filename);
        printf("Make sure the file exists in the current directory\n");
        return;
    }
    
    // Read header
    int width, height, frameCount, isAnimated;
    size_t read;
    
    read = fread(&width, sizeof(int), 1, fp);
    if (read != 1) { printf("Error reading width\n"); fclose(fp); return; }
    
    read = fread(&height, sizeof(int), 1, fp);
    if (read != 1) { printf("Error reading height\n"); fclose(fp); return; }
    
    read = fread(&frameCount, sizeof(int), 1, fp);
    if (read != 1) { printf("Error reading frameCount\n"); fclose(fp); return; }
    
    read = fread(&isAnimated, sizeof(int), 1, fp);
    if (read != 1) { printf("Error reading isAnimated\n"); fclose(fp); return; }
    
    read = fread(editor.texture.name, sizeof(char), MAX_FILENAME, fp);
    if (read != MAX_FILENAME) { printf("Error reading texture name\n"); fclose(fp); return; }
    
    // Validate data
    if (width <= 0 || width > MAX_TEXTURE_SIZE || 
        height <= 0 || height > MAX_TEXTURE_SIZE ||
        frameCount <= 0 || frameCount > MAX_FRAMES) {
        printf("Error: Invalid texture data in file\n");
        printf("Width: %d, Height: %d, FrameCount: %d\n", width, height, frameCount);
        fclose(fp);
        return;
    }
    
    // Clear existing frames
    for (int i = 0; i < editor.texture.frameCount; i++) {
        if (editor.texture.frames[i].data) {
            free(editor.texture.frames[i].data);
            editor.texture.frames[i].data = NULL;
        }
    }
    
    editor.textureWidth = width;
    editor.textureHeight = height;
    editor.texture.frameCount = frameCount;
    editor.texture.isAnimated = isAnimated;
    editor.texture.currentFrame = 0;
    
    // Read frames
    for (int i = 0; i < frameCount; i++) {
        TextureFrame* frame = &editor.texture.frames[i];
        frame->width = width;
        frame->height = height;
        
        read = fread(&frame->duration, sizeof(int), 1, fp);
        if (read != 1) {
            printf("Error reading frame %d duration\n", i);
            fclose(fp);
            return;
        }
        
        int dataSize = width * height * 3;
        frame->data = (unsigned char*)malloc(dataSize);
        if (!frame->data) {
            printf("Error: Failed to allocate memory for frame %d\n", i);
            fclose(fp);
            return;
        }
        
        read = fread(frame->data, sizeof(unsigned char), dataSize, fp);
        if (read != dataSize) {
            printf("Error reading frame %d data (expected %d, got %zu)\n", i, dataSize, read);
            fclose(fp);
            return;
        }
    }
    
    fclose(fp);
    printf("Successfully loaded texture: %s (%dx%d, %d frames)\n", 
           editor.texture.name, width, height, frameCount);
    
    glutPostRedisplay();
}

void exportCHeader(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot open file for writing\n");
        return;
    }
    
    // Write header guard
    fprintf(fp, "#ifndef TEXTURE_EXPORT_H\n");
    fprintf(fp, "#define TEXTURE_EXPORT_H\n\n");
    
    if (editor.texture.isAnimated) {
        // Animated texture export
        fprintf(fp, "#define %s_FRAME_COUNT %d\n", editor.texture.name, editor.texture.frameCount);
        fprintf(fp, "#define %s_FRAME_WIDTH %d\n", editor.texture.name, editor.textureWidth);
        fprintf(fp, "#define %s_FRAME_HEIGHT %d\n", editor.texture.name, editor.textureHeight);
        fprintf(fp, "#define %s_ANIM_AVAILABLE 1\n\n", editor.texture.name);
        
        // Export each frame
        for (int f = 0; f < editor.texture.frameCount; f++) {
            TextureFrame* frame = &editor.texture.frames[f];
            fprintf(fp, "static const unsigned char %s_frame_%d[] = {\n", editor.texture.name, f);
            
            for (int i = 0; i < frame->width * frame->height * 3; i++) {
                if (i % 12 == 0) fprintf(fp, "    ");
                fprintf(fp, "%3d", frame->data[i]);
                if (i < frame->width * frame->height * 3 - 1) fprintf(fp, ",");
                if (i % 12 == 11) fprintf(fp, "\n");
            }
            fprintf(fp, "\n};\n\n");
        }
        
        // Export frame array
        fprintf(fp, "static const unsigned char* %s_frames[] = {\n", editor.texture.name);
        for (int f = 0; f < editor.texture.frameCount; f++) {
            fprintf(fp, "    %s_frame_%d", editor.texture.name, f);
            if (f < editor.texture.frameCount - 1) fprintf(fp, ",");
            fprintf(fp, "\n");
        }
        fprintf(fp, "};\n\n");
        
        // Export frame durations
        fprintf(fp, "static const int %s_frame_durations[] = {", editor.texture.name);
        for (int f = 0; f < editor.texture.frameCount; f++) {
            fprintf(fp, "%d", editor.texture.frames[f].duration);
            if (f < editor.texture.frameCount - 1) fprintf(fp, ", ");
        }
        fprintf(fp, "};\n");
        fprintf(fp, "#define %s_FRAME_MS %d\n", editor.texture.name, editor.texture.frames[0].duration);
        
    } else {
        // Static texture export
        TextureFrame* frame = &editor.texture.frames[0];
        fprintf(fp, "#define %s_WIDTH %d\n", editor.texture.name, frame->width);
        fprintf(fp, "#define %s_HEIGHT %d\n\n", editor.texture.name, frame->height);
        
        fprintf(fp, "static const unsigned char %s[] = {\n", editor.texture.name);
        for (int i = 0; i < frame->width * frame->height * 3; i++) {
            if (i % 12 == 0) fprintf(fp, "    ");
            fprintf(fp, "%3d", frame->data[i]);
            if (i < frame->width * frame->height * 3 - 1) fprintf(fp, ",");
            if (i % 12 == 11) fprintf(fp, "\n");
        }
        fprintf(fp, "\n};\n\n");
    }
    
    fprintf(fp, "#endif // TEXTURE_EXPORT_H\n");
    fclose(fp);
    
    printf("Exported to %s\n", filename);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Hammer Texture Editor");
    
    init();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);
    
    printf("=== DOOM TEXTURE EDITOR ===\n");
    printf("Controls:\n");
    printf("  N - New texture\n");
    printf("  S - Save\n");
    printf("  L - Load\n");
    printf("  E - Export to C header\n");
    printf("  A - Add frame\n");
    printf("  D - Delete frame\n");
    printf("  C - Duplicate frame\n");
    printf("  Space - Play/Pause animation\n");
    printf("  G - Toggle grid\n");
    printf("  +/- - Zoom in/out\n");
    printf("  [ / ] - Decrease/Increase brush size\n");
    printf("  < / > - Previous/Next frame\n");
    printf("  ESC - Exit\n");
    
    glutMainLoop();
    
    return 0;
}
