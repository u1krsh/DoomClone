#ifndef AUTOMAP_H
#define AUTOMAP_H

// Automap Module
// Provides minimap/automap functionality for navigation

// Forward declarations for external dependencies
typedef struct {
	int x, y, z;
	int a;
	int l;
} PlayerState;

typedef struct {
	int x1, y1;
	int x2, y2;
	int c;
	int wt, u, v;
	int shade;
} WallData;

#define AUTOMAP_SW 320

typedef struct {
	int ws, we;
	int z1, z2;
	int x, y;
	int d;
	int c1, c2;
	int surf[AUTOMAP_SW];
	int surface;
	int ss, st;
} SectorData;

typedef struct {
	float cos[360];
	float sin[360];
} MathTable;

// Initialize automap system
void initAutomap(void);

// Update automap animation
void updateAutomap(void);

// Draw automap on screen
void drawAutomap(
	void (*pixelFunc)(int, int, int, int, int),
	int screenWidth,
	int screenHeight,
	const PlayerState* player,
	const WallData* walls,
	const SectorData* sectors,
	int numSectors,
	const MathTable* mathTable
);

// Toggle automap visibility
void toggleAutomap(void);

// Check if automap is active
int isAutomapActive(void);

// Check if automap is animating
int isAutomapAnimating(void);

#endif // AUTOMAP_H
