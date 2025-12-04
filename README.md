# DoomClone

A retro-style raycasting engine written in C using OpenGL (GLUT). This project implements core features of classic 2.5D shooters like Doom and Wolfenstein 3D.

## Features

*   **Raycasting Rendering**: Fast and efficient wall rendering with texture mapping.
*   **Texture Mapping**: Support for wall textures, including animated textures.
*   **Floor & Ceiling Rendering**: Textured floors and ceilings with depth shading.
*   **Sprite System**: Billboarded sprites for enemies and objects with depth buffering.
*   **Collision Detection**:
    *   Wall collision with sliding.
    *   Sector-based collision.
    *   Z-axis (height) collision and gravity/snapping.
*   **Automap**: Real-time top-down view of the level geometry.
*   **Console System**: Quake-style developer console for commands and debugging.
*   **Visual Effects**:
    *   Distance fog/shading.
    *   Screen melt transition effect.
    *   Head bobbing (implied by movement).
*   **Tools**:
    *   Map Editor (`tools/oracular_editor.py`)
    *   Texture Editor (`tools/texture_editor_pro.py`)

## Controls

| Key | Action |
| :--- | :--- |
| **W / S** | Move Forward / Backward |
| **A / D** | Rotate Left / Right |
| **. / ,** | Strafe Left / Right |
| **M** | Toggle Look/Fly Mode (with W/S/A/D) |
| **Tab** | Toggle Automap |
| **` / ~** | Toggle Console |
| **F1** | Toggle FPS Display |
| **Enter** | Reload Level / Trigger Screen Melt |
| **Esc** | Pause Game |

## Building and Running

### Prerequisites
*   Visual Studio (2019/2022 recommended)
*   OpenGL / GLUT libraries (usually included or easily installable via NuGet)

### Instructions
1.  Open the solution file (`.sln`) in Visual Studio.
2.  Ensure the target is set to **x86** or **x64** matching your GLUT DLLs.
3.  Build the solution (Ctrl+Shift+B).
4.  Run the application (F5).

## Tools

The project includes Python-based tools in the `tools/` directory to assist with content creation:

*   **Oracular Map Editor**: A visual editor for creating and modifying level layouts (`level.h`).
*   **Texture Editor Pro**: A tool for importing and manipulating textures for the engine.

## License

This project is for educational purposes.
