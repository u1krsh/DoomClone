#include "automap.h"

// Automap state
typedef struct {
	int mapActive;         // is map currently active
	int mapAnimating;      // is map animation in progress
	float mapSlidePos;     // map slide position (0.0 = hidden, 1.0 = visible)
} AutomapState;

static AutomapState automap;

// Initialize automap system
void initAutomap(void) {
	automap.mapActive = 0;
	automap.mapAnimating = 0;
	automap.mapSlidePos = 0.0f;
}

// Update automap animation
void updateAutomap(void) {
	if (!automap.mapAnimating) return;
	
	if (automap.mapActive) {
		// Slide down
		automap.mapSlidePos += 0.15f;
		if (automap.mapSlidePos >= 1.0f) {
			automap.mapSlidePos = 1.0f;
			automap.mapAnimating = 0;
		}
	} else {
		// Slide up
		automap.mapSlidePos -= 0.15f;
		if (automap.mapSlidePos <= 0.0f) {
			automap.mapSlidePos = 0.0f;
			automap.mapAnimating = 0;
		}
	}
}

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
) {
	// Don't draw if not visible
	if (automap.mapSlidePos <= 0.0f) return;
	
	// Calculate visible height based on slide position
	int visibleHeight = (int)(screenHeight * automap.mapSlidePos);
	
	// Map covers full screen width, slides from top
	int mapX = 0;
	int mapY = screenHeight - visibleHeight;
	int mapWidth = screenWidth;
	int mapHeight = visibleHeight;
	
	// Don't draw if too small
	if (mapHeight < 10) return;
	
	int mapScale = 4; // Scale down factor for the map
	
	// Draw map background (black)
	int x, y;
	for (y = mapY; y < screenHeight; y++) {
		for (x = 0; x < screenWidth; x++) {
			pixelFunc(x, y, 0, 0, 0); // Black background
		}
	}
	
	// Draw bottom border (red line - 2 pixels thick)
	for (x = 0; x < screenWidth; x++) {
		pixelFunc(x, mapY, 255, 0, 0); // Red border
		if (mapY + 1 < screenHeight) {
			pixelFunc(x, mapY + 1, 255, 0, 0);
		}
	}
	
	// Calculate map center (player position)
	int centerX = mapWidth / 2;
	int centerY = mapY + mapHeight / 2;
	
	// Draw walls
	int s, w;
	for (s = 0; s < numSectors; s++) {
		for (w = sectors[s].ws; w < sectors[s].we; w++) {
			// Transform wall coordinates relative to player
			int wx1 = (walls[w].x1 - player->x) / mapScale + centerX;
			int wy1 = (walls[w].y1 - player->y) / mapScale + centerY;
			int wx2 = (walls[w].x2 - player->x) / mapScale + centerX;
			int wy2 = (walls[w].y2 - player->y) / mapScale + centerY;
			
			// Draw line using Bresenham's algorithm
			int dx = wx2 - wx1;
			int dy = wy2 - wy1;
			int steps = (dx > 0 ? dx : -dx) > (dy > 0 ? dy : -dy) ? (dx > 0 ? dx : -dx) : (dy > 0 ? dy : -dy);
			
			if (steps > 0) {
				float xInc = dx / (float)steps;
				float yInc = dy / (float)steps;
				float currX = (float)wx1;
				float currY = (float)wy1;
				
				int i;
				for (i = 0; i <= steps; i++) {
					int px = (int)currX;
					int py = (int)currY;
					
					// Clip to map bounds
					if (px >= mapX && px < mapX + mapWidth && py >= mapY && py < mapY + mapHeight) {
						pixelFunc(px, py, 200, 200, 200); // Gray walls
					}
					
					currX += xInc;
					currY += yInc;
				}
			}
		}
	}
	
	// Draw player as upside-down cross (ankh/crucifix)
	// Vertical bar
	for (y = -6; y <= 2; y++) {
		pixelFunc(centerX, centerY + y, 255, 0, 0);
	}
	// Horizontal bar (top)
	for (x = -3; x <= 3; x++) {
		pixelFunc(centerX + x, centerY - 2, 255, 0, 0);
	}
	
	// Draw direction indicator (thicker yellow line pointing in player's direction)
	int dirLen = 12;
	int dirX = centerX + (int)(mathTable->sin[player->a] * dirLen);
	int dirY = centerY + (int)(mathTable->cos[player->a] * dirLen);
	
	// Draw direction line with thickness (3 pixels wide)
	{
		int dx = dirX - centerX;
		int dy = dirY - centerY;
		int steps = (dx > 0 ? dx : -dx) > (dy > 0 ? dy : -dy) ? (dx > 0 ? dx : -dx) : (dy > 0 ? dy : -dy);
		
		if (steps > 0) {
			float xInc = dx / (float)steps;
			float yInc = dy / (float)steps;
			float currX = (float)centerX;
			float currY = (float)centerY;
			
			int i;
			for (i = 0; i <= steps; i++) {
				int px = (int)currX;
				int py = (int)currY;
				
				// Draw 3x3 block for thickness
				int ox, oy;
				for (ox = -1; ox <= 1; ox++) {
					for (oy = -1; oy <= 1; oy++) {
						int drawX = px + ox;
						int drawY = py + oy;
						if (drawX >= mapX && drawX < mapX + mapWidth && drawY >= mapY && drawY < mapY + mapHeight) {
							pixelFunc(drawX, drawY, 255, 255, 0); // Yellow direction line
						}
					}
				}
				
				currX += xInc;
				currY += yInc;
			}
		}
	}
}

// Toggle automap visibility
void toggleAutomap(void) {
	automap.mapActive = !automap.mapActive;
	automap.mapAnimating = 1;
}

// Check if automap is active
int isAutomapActive(void) {
	return automap.mapActive;
}

// Check if automap is animating
int isAutomapAnimating(void) {
	return automap.mapAnimating;
}
