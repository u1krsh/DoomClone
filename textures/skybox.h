#ifndef SKYBOX_H
#define SKYBOX_H

// Include all 5 cubemap face textures
#include "skybox_1_fr.h"  // Front
#include "skybox_1_bk.h"  // Back
#include "skybox_1_lf.h"  // Left
#include "skybox_1_rt.h"  // Right
#include "skybox_1_up.h"  // Up

// Cubemap face enum for easy reference
typedef enum {
    SKYBOX_FACE_FRONT = 0,
    SKYBOX_FACE_RIGHT = 1,
    SKYBOX_FACE_BACK = 2,
    SKYBOX_FACE_LEFT = 3,
    SKYBOX_FACE_UP = 4
} SkyboxFace;

// Structure to hold cubemap face data
typedef struct {
    const unsigned char* data;
    int width;
    int height;
} SkyboxFaceData;

// Cubemap faces array
static const SkyboxFaceData SKYBOX_FACES[5] = {
    { SKYBOX_1_FR, SKYBOX_1_FR_WIDTH, SKYBOX_1_FR_HEIGHT },  // Front
    { SKYBOX_1_RT, SKYBOX_1_RT_WIDTH, SKYBOX_1_RT_HEIGHT },  // Right
    { SKYBOX_1_BK, SKYBOX_1_BK_WIDTH, SKYBOX_1_BK_HEIGHT },  // Back
    { SKYBOX_1_LF, SKYBOX_1_LF_WIDTH, SKYBOX_1_LF_HEIGHT },  // Left
    { SKYBOX_1_UP, SKYBOX_1_UP_WIDTH, SKYBOX_1_UP_HEIGHT }   // Up
};

#endif // SKYBOX_H
