#ifndef WALL57_ANIM_H
#define WALL57_ANIM_H

#include "WALL57_1.h"
#include "WALL57_2.h"
#include "WALL57_3.h"
#include "WALL57_4.h"

// Animation configuration
#define WALL57_ANIM_AVAILABLE 1
#define WALL57_FRAME_COUNT 4  // Ping-pong: 2->3->4->3 (WALL57_1 excluded from loop)
#define WALL57_FRAME_MS 150  // 150ms per frame (~6.7 FPS)
#define WALL57_FRAME_WIDTH 64
#define WALL57_FRAME_HEIGHT 128

// Array of frame pointers - ping-pong animation: 2->3->4->3->repeat
// WALL57_1 is only shown initially when texture is first loaded
static const unsigned char* WALL57_frames[WALL57_FRAME_COUNT] = {
    (const unsigned char*)WALL57_2,  // Frame 0: Start with frame 2
    (const unsigned char*)WALL57_3,  // Frame 1: Forward to 3
    (const unsigned char*)WALL57_4,  // Frame 2: Peak at 4
    (const unsigned char*)WALL57_3   // Frame 3: Back to 3 (loops to frame 0)
};

#endif // WALL57_ANIM_H
