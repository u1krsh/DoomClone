cmake_minimum_required(VERSION 3.16)
project(DoomClone C)

set(CMAKE_C_STANDARD 99)

# ================= COMMON INCLUDES =================
include_directories(
        .
        textures
        tools/textures
)

find_package(OpenGL REQUIRED)

# ================= DOOM GAME =================
add_executable(DoomClone
        DoomTest.c
        automap.c
        console.c
        console_font.c
        fps_counter.c
        hud.c
        lighting.c
        screen_melt.c
)

target_link_libraries(DoomClone
        OpenGL::GL
        OpenGL::GLU
        glut
        openal
        m
)

# ================= COPY RUNTIME DATA FILES =================
# level.h is loaded at runtime via fopen(), not compiled
file(COPY ${CMAKE_SOURCE_DIR}/level.h DESTINATION ${CMAKE_BINARY_DIR})

# Copy sounds directory for OpenAL audio
file(COPY ${CMAKE_SOURCE_DIR}/sounds DESTINATION ${CMAKE_BINARY_DIR})

# Copy textures directory (if loaded at runtime)
file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})

# ================= MAP EDITOR (TOOL) =================
add_executable(MapEditor
        Oracular_map_editor.c
)

target_link_libraries(MapEditor
        OpenGL::GL
        OpenGL::GLU
        glut
        m
)

# ================= TEXTURE EDITOR (TOOL) =================
add_executable(TextureEditor
        Texture_Editor/texture_editor.c
)

target_link_libraries(TextureEditor
        OpenGL::GL
        OpenGL::GLU
        glut
        m
)
