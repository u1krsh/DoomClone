#ifndef SOUND_H
#define SOUND_H

// Sound System Module - OpenAL implementation
// Supports multiple simultaneous sounds with volume control and 3D positioning capabilities

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AL/al.h>
#include <AL/alc.h>

// Sound file paths
#define SOUND_PISTOL "sounds/guns/pistol/DSPISTOL.wav"
#define SOUND_SHOTGUN "sounds/guns/shotgun/DSSHOTGN.wav"
#define SOUND_CHAINGUN "sounds/guns/chaingun/DCHGUN.wav"
#define SOUND_PUNCH "sounds/guns/DSOOF.wav"
#define SOUND_PLASMA "sounds/guns/plasma/DSPLASMA.wav"
#define SOUND_MUSIC_E1M1 "sounds/BG/D_E1M1.wav"

// Volume levels (0.0 - 1.0)
#define MAX_MUSIC_VOLUME 0.35f
#define MAX_SFX_VOLUME 1.0f

// Maximum simultaneous sound effects
#define Max_SFX_SOURCES 16

// OpenAL Objects
static ALCdevice* device = NULL;
static ALCcontext* context = NULL;

// Buffers hold audio data
static ALuint bufferPistol = 0;
static ALuint bufferShotgun = 0;
static ALuint bufferChaingun = 0;
static ALuint bufferPunch = 0;
static ALuint bufferPlasma = 0;
static ALuint bufferMusic = 0;

// Sources emit sound
static ALuint sourceMusic = 0;
static ALuint sourcesSFX[Max_SFX_SOURCES];
static int nextSFXSource = 0;

static int soundEnabled = 1;

// --- Minimal WAV Loader Helper ---
// Returns AL buffer ID on success, 0 on failure
static ALuint loadWavFile(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Failed to open WAV file: %s\n", filename);
        return 0;
    }

    // Read header parts
    char type[4];
    unsigned int size, chunkSize;
    short formatType, channels;
    unsigned int sampleRate, avgBytesPerSec;
    short bytesPerSample, bitsPerSample;
    unsigned int dataSize;
    unsigned char* data;

    // RIFF chunk
    if (fread(type, 1, 4, fp) != 4 || strncmp(type, "RIFF", 4) != 0) { fclose(fp); return 0; }
    fread(&size, 4, 1, fp);
    if (fread(type, 1, 4, fp) != 4 || strncmp(type, "WAVE", 4) != 0) { fclose(fp); return 0; }

    // fmt chunk
    if (fread(type, 1, 4, fp) != 4 || strncmp(type, "fmt ", 4) != 0) { fclose(fp); return 0; }
    fread(&chunkSize, 4, 1, fp);
    fread(&formatType, 2, 1, fp);
    fread(&channels, 2, 1, fp);
    fread(&sampleRate, 4, 1, fp);
    fread(&avgBytesPerSec, 4, 1, fp);
    fread(&bytesPerSample, 2, 1, fp);
    fread(&bitsPerSample, 2, 1, fp);
    
    // Skip any extra format bytes if chunk is larger than 16
    if (chunkSize > 16) {
        fseek(fp, chunkSize - 16, SEEK_CUR);
    }

    // data chunk search (skip other chunks like LIST)
    while (1) {
        if (fread(type, 1, 4, fp) != 4) { fclose(fp); return 0; }
        fread(&dataSize, 4, 1, fp);
        if (strncmp(type, "data", 4) == 0) break;
        fseek(fp, dataSize, SEEK_CUR);
    }

    data = (unsigned char*)malloc(dataSize);
    if (fread(data, 1, dataSize, fp) != dataSize) {
        free(data);
        fclose(fp);
        return 0;
    }
    fclose(fp);

    ALenum format;
    if (channels == 1)
        format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    else
        format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;

    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, data, dataSize, sampleRate);
    
    free(data);
    
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        printf("OpenAL Error loading %s: %d\n", filename, error);
        return 0;
    }

    return buffer;
}

// Initialize sound system
void initSound(void) {
    soundEnabled = 1;

    // Open default device
    device = alcOpenDevice(NULL);
    if (!device) {
        printf("Failed to open OpenAL device\n");
        soundEnabled = 0;
        return;
    }

    // Create context
    context = alcCreateContext(device, NULL);
    if (!context || !alcMakeContextCurrent(context)) {
        printf("Failed to create/set OpenAL context\n");
        if(context) alcDestroyContext(context);
        alcCloseDevice(device);
        soundEnabled = 0;
        return;
    }

    // Generate Buffers
    bufferPistol = loadWavFile(SOUND_PISTOL);
    bufferShotgun = loadWavFile(SOUND_SHOTGUN);
    bufferChaingun = loadWavFile(SOUND_CHAINGUN);
    bufferPunch = loadWavFile(SOUND_PUNCH);
    bufferPlasma = loadWavFile(SOUND_PLASMA);
    bufferMusic = loadWavFile(SOUND_MUSIC_E1M1);

    // Generate Sources
    alGenSources(1, &sourceMusic);
    alSourcef(sourceMusic, AL_GAIN, MAX_MUSIC_VOLUME);
    
    alGenSources(Max_SFX_SOURCES, sourcesSFX);
    for(int i=0; i<Max_SFX_SOURCES; i++) {
        alSourcef(sourcesSFX[i], AL_GAIN, MAX_SFX_VOLUME);
    }
}

// Check if sound is enabled
int isSoundEnabled(void) {
    return soundEnabled;
}

void toggleSound(void) {
    soundEnabled = !soundEnabled;
    if (soundEnabled) {
        alcMakeContextCurrent(context);
    } else {
        alcMakeContextCurrent(NULL);
    }
}

// Play a sound buffer on next available channel
void playBuffer(ALuint buffer) {
    if (!soundEnabled || buffer == 0) return;

    // Find a non-playing source or pick the next one round-robin
    ALuint source = sourcesSFX[nextSFXSource];
    
    alSourceStop(source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    nextSFXSource = (nextSFXSource + 1) % Max_SFX_SOURCES;
}

void playSoundPistol(void) { playBuffer(bufferPistol); }
void playSoundShotgun(void) { playBuffer(bufferShotgun); }
void playSoundChaingun(void) { playBuffer(bufferChaingun); }
void playSoundPunch(void) { playBuffer(bufferPunch); }
void playSoundPlasma(void) { playBuffer(bufferPlasma); }

// Generic play for compatibility
void playSound(const char* filename) {
    // Note: In a real engine, we'd look up the buffer by filename hash/map.
    // Here we map known files to preloaded buffers.
    if (strstr(filename, "PISTOL")) playSoundPistol();
    else if (strstr(filename, "SHOTGN")) playSoundShotgun();
    else if (strstr(filename, "CHGUN")) playSoundChaingun();
    else if (strstr(filename, "PUNCH") || strstr(filename, "OOF")) playSoundPunch();
    else if (strstr(filename, "PLASMA")) playSoundPlasma();
}

void playWeaponSound(int weaponType) {
    switch (weaponType) {
        case 0: playSoundPunch(); break;      // WEAPON_FIST
        case 1: playSoundPistol(); break;     // WEAPON_PISTOL
        case 2: playSoundShotgun(); break;    // WEAPON_SHOTGUN
        case 3: playSoundChaingun(); break;   // WEAPON_CHAINGUN
        case 4: playSoundPlasma(); break;     // WEAPON_PLASMA
        default: playSoundPistol(); break;
    }
}

void playBackgroundMusic(void) {
    if (!soundEnabled || bufferMusic == 0) return;

    alSourcei(sourceMusic, AL_BUFFER, bufferMusic);
    alSourcei(sourceMusic, AL_LOOPING, AL_TRUE);
    alSourcePlay(sourceMusic);
}

void stopBackgroundMusic(void) {
    if (soundEnabled) {
        alSourceStop(sourceMusic);
    }
}

void stopAllSounds(void) {
    if (!soundEnabled) return;
    alSourceStop(sourceMusic);
    for(int i=0; i<Max_SFX_SOURCES; i++) {
        alSourceStop(sourcesSFX[i]);
    }
}

void cleanupSound(void) {
    stopAllSounds();
    
    alDeleteSources(1, &sourceMusic);
    alDeleteSources(Max_SFX_SOURCES, sourcesSFX);
    
    alDeleteBuffers(1, &bufferPistol);
    alDeleteBuffers(1, &bufferShotgun);
    alDeleteBuffers(1, &bufferChaingun);
    alDeleteBuffers(1, &bufferPunch);
    alDeleteBuffers(1, &bufferPlasma);
    alDeleteBuffers(1, &bufferMusic);

    alcMakeContextCurrent(NULL);
    if(context) alcDestroyContext(context);
    if(device) alcCloseDevice(device);
}

#endif // SOUND_H
