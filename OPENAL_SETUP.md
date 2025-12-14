# OpenAL Setup Instructions

## Download OpenAL Soft

1. Download **openal-soft** binaries from the official source:
   - https://openal-soft.org/
   - Download `openal-soft-1.23.1-bin.zip` (or newer)

## Installation

### For MinGW (Visual Studio Code / gcc):
1. Extract the downloaded zip.
2. Locate the `include` folder in the zip. Copy the `AL` folder (containing `al.h`, `alc.h`, etc.) to your MinGW include directory:
   - `C:\mingw\include\AL\`
3. Locate `libs\Win64\libOpenAL32.dll.a` (if on 64-bit) or `libs\Win32\libOpenAL32.dll.a` (for 32-bit).
   - Copy this to your lib directory: `C:\mingw\lib\`
   - Rename explicitly to `libOpenAL32.a` if needed, or link as `-lOpenAL32`.
4. **Crucial**: Copy `bin\Win64\soft_oal.dll` (or Win32 version) to your project folder (`d:\PROGRAM\VSPRO\DoomClone\`).
   - Rename it to `OpenAL32.dll` so the application finds it as the default OpenAL driver.

### For Visual Studio (IDE):
1. Extract the zip.
2. In Project Properties -> C/C++ -> General -> Additional Include Directories:
   - Add path to `include` folder.
3. In Linker -> General -> Additional Library Directories:
   - Add path to `libs\Win64` (or Win32).
4. In Linker -> Input -> Additional Dependencies:
   - Add `OpenAL32.lib`.
5. Copy `bin\Win64\soft_oal.dll` to the folder containing your `.exe` (Debug/Release folder) and rename it to `OpenAL32.dll`.

## Compilation Command (gcc)

```bash
gcc -o DoomTest.exe DoomTest.c -lopengl32 -lglu32 -lglut32 -lwinmm -lOpenAL32 -Wall
```

## Features
- Hardware accelerated 3D audio (emulated via OpenAL Soft)
- Simultaneous sound playback (Music + SFX)
- Independent volume control
- No pausing of music when SFX plays
