#ifndef SOUND_H
#define SOUND_H

// Sound System Module
// Simple sound effects using Windows API

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

// Sound file paths
#define SOUND_PISTOL "sounds/guns/pistol/DSPISTOL.wav"
#define SOUND_SHOTGUN "sounds/DSHOTGN.wav"
#define SOUND_CHAINGUN "sounds/DCHGUN.wav"
#define SOUND_PUNCH "sounds/DPUNCH.wav"

// Sound enabled flag
static int soundEnabled = 1;

// Initialize sound system
void initSound(void) {
    soundEnabled = 1;
}

// Toggle sound on/off
void toggleSound(void) {
    soundEnabled = !soundEnabled;
}

// Check if sound is enabled
int isSoundEnabled(void) {
    return soundEnabled;
}

// Play a sound effect (non-blocking, async)
void playSound(const char* filename) {
#ifdef _WIN32
    if (!soundEnabled) return;
    
    // SND_ASYNC: Play asynchronously (don't block)
    // SND_NODEFAULT: Don't play default sound if file not found
    PlaySoundA(filename, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
#endif
}

// Play pistol shot sound
void playSoundPistol(void) {
    playSound(SOUND_PISTOL);
}

// Play shotgun sound
void playSoundShotgun(void) {
    playSound(SOUND_SHOTGUN);
}

// Play chaingun sound
void playSoundChaingun(void) {
    playSound(SOUND_CHAINGUN);
}

// Play punch sound
void playSoundPunch(void) {
    playSound(SOUND_PUNCH);
}

// Play weapon sound based on weapon type
void playWeaponSound(int weaponType) {
    switch (weaponType) {
        case 0:  // WEAPON_FIST
            playSoundPunch();
            break;
        case 1:  // WEAPON_PISTOL
            playSoundPistol();
            break;
        case 2:  // WEAPON_SHOTGUN
            playSoundShotgun();
            break;
        case 3:  // WEAPON_CHAINGUN
            playSoundChaingun();
            break;
        default:
            playSoundPistol();
            break;
    }
}

// Stop all sounds
void stopAllSounds(void) {
#ifdef _WIN32
    PlaySoundA(NULL, NULL, 0);
#endif
}

#endif // SOUND_H
