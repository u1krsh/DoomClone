@echo off
REM ============================================================
REM DoomClone Complete Build & Package Script
REM 
REM This script does everything in one go:
REM   1. Checks/installs Python dependencies
REM   2. Builds OracularEditor.exe from oracular_editor.py
REM   3. Builds TextureEditorPro.exe from texture_editor_pro.py
REM   4. Converts level.h to level.sau (with embedded textures)
REM   5. Copies DoomClone.exe, freeglut.dll, and bundles everything
REM
REM Usage: Just double-click build_package.bat
REM ============================================================

setlocal enabledelayedexpansion
cd /d "%~dp0"

echo.
echo ================================================================
echo     ____                        ______ __                    
echo    / __ \____  ____  ____ ___  / ____// /___  ____  ___      
echo   / / / / __ \/ __ \/ __ `__ \/ /    / / __ \/ __ \/ _ \     
echo  / /_/ / /_/ / /_/ / / / / / / /___ / / /_/ / / / /  __/     
echo /_____/\____/\____/_/ /_/ /_/\____//_/\____/_/ /_/\___/      
echo.
echo                  BUILD ^& PACKAGE SYSTEM                       
echo ================================================================
echo.

set "PROJECT_DIR=%~dp0"
set "TOOLS_DIR=%PROJECT_DIR%tools"
set "TEXTURES_DIR=%PROJECT_DIR%textures"
set "OUTPUT_DIR=%PROJECT_DIR%build\package"
set "TEMP_DIR=%PROJECT_DIR%build\temp"
set "SPEC_DIR=%PROJECT_DIR%build\spec"

REM ================================================================
echo [STEP 1/7] Checking Python installation...
echo ----------------------------------------------------------------

where python >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Python not found in PATH!
    echo.
    echo Please install Python 3.x from https://python.org
    echo Make sure to check "Add Python to PATH" during installation.
    echo.
    pause
    exit /b 1
)

for /f "tokens=*" %%i in ('python --version 2^>^&1') do set PYTHON_VER=%%i
echo [OK] Found: %PYTHON_VER%

REM ================================================================
echo.
echo [STEP 2/7] Installing/checking dependencies...
echo ----------------------------------------------------------------

echo Checking PyInstaller...
python -c "import PyInstaller" 2>nul
if %ERRORLEVEL% neq 0 (
    echo Installing PyInstaller...
    pip install pyinstaller --quiet
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Failed to install PyInstaller
        pause
        exit /b 1
    )
)
echo [OK] PyInstaller ready

echo Checking Pillow...
python -c "from PIL import Image" 2>nul
if %ERRORLEVEL% neq 0 (
    echo Installing Pillow...
    pip install Pillow --quiet
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Failed to install Pillow
        pause
        exit /b 1
    )
)
echo [OK] Pillow ready

echo Checking pygame...
python -c "import pygame" 2>nul
if %ERRORLEVEL% neq 0 (
    echo Installing pygame...
    pip install pygame --quiet
)
echo [OK] pygame ready

REM ================================================================
echo.
echo [STEP 3/7] Creating output directories...
echo ----------------------------------------------------------------

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%OUTPUT_DIR%\tools" mkdir "%OUTPUT_DIR%\tools"
if not exist "%TEMP_DIR%" mkdir "%TEMP_DIR%"
if not exist "%SPEC_DIR%" mkdir "%SPEC_DIR%"
echo [OK] Directories created: %OUTPUT_DIR%

REM ================================================================
echo.
echo [STEP 4/7] Building Python tools as EXE...
echo ----------------------------------------------------------------

REM Build Oracular Editor (Map Editor)
set "EDITOR_PY="
if exist "%TOOLS_DIR%\oracular_editor.py" (
    set "EDITOR_PY=%TOOLS_DIR%\oracular_editor.py"
    set "EDITOR_NAME=oracular_editor.py"
) else if exist "%TOOLS_DIR%\hammer_editor.py" (
    set "EDITOR_PY=%TOOLS_DIR%\hammer_editor.py"
    set "EDITOR_NAME=hammer_editor.py"
)

if defined EDITOR_PY (
    echo Building OracularEditor.exe from %EDITOR_NAME%...
    
    cd /d "%TOOLS_DIR%"
    pyinstaller --onefile --windowed --name "OracularEditor" ^
        --distpath "%OUTPUT_DIR%\tools" ^
        --workpath "%TEMP_DIR%" ^
        --specpath "%SPEC_DIR%" ^
        --noconfirm ^
        --clean ^
        --log-level WARN ^
        --hidden-import pygame ^
        --hidden-import tkinter ^
        --hidden-import tkinter.filedialog ^
        "%EDITOR_PY%" 2>nul
    
    cd /d "%PROJECT_DIR%"
    
    if exist "%OUTPUT_DIR%\tools\OracularEditor.exe" (
        for %%A in ("%OUTPUT_DIR%\tools\OracularEditor.exe") do set "SIZE=%%~zA"
        set /a "SIZE_MB=!SIZE! / 1048576"
        echo [OK] OracularEditor.exe created ^(!SIZE_MB! MB^)
    ) else (
        echo [WARNING] OracularEditor.exe build may have failed
    )
) else (
    echo [WARNING] No map editor Python file found
)

REM Build Texture Editor
if exist "%TOOLS_DIR%\texture_editor_pro.py" (
    echo Building TextureEditorPro.exe from texture_editor_pro.py...
    
    cd /d "%TOOLS_DIR%"
    pyinstaller --onefile --windowed --name "TextureEditorPro" ^
        --distpath "%OUTPUT_DIR%\tools" ^
        --workpath "%TEMP_DIR%" ^
        --specpath "%SPEC_DIR%" ^
        --noconfirm ^
        --clean ^
        --log-level WARN ^
        --hidden-import PIL ^
        --hidden-import PIL.Image ^
        --hidden-import PIL.ImageDraw ^
        --hidden-import PIL.ImageTk ^
        --hidden-import PIL.ImageFont ^
        --hidden-import tkinter ^
        "%TOOLS_DIR%\texture_editor_pro.py" 2>nul
    
    cd /d "%PROJECT_DIR%"
    
    if exist "%OUTPUT_DIR%\tools\TextureEditorPro.exe" (
        for %%A in ("%OUTPUT_DIR%\tools\TextureEditorPro.exe") do set "SIZE=%%~zA"
        set /a "SIZE_MB=!SIZE! / 1048576"
        echo [OK] TextureEditorPro.exe created ^(!SIZE_MB! MB^)
    ) else (
        echo [WARNING] TextureEditorPro.exe build may have failed
    )
) else (
    echo [WARNING] texture_editor_pro.py not found
)

REM ================================================================
echo.
echo [STEP 5/7] Creating SAU file with embedded textures...
echo ----------------------------------------------------------------

if exist "%PROJECT_DIR%level.h" (
    if exist "%TOOLS_DIR%\sau_builder.py" (
        echo Converting level.h to level.sau with textures...
        
        REM Check if textures directory exists
        if exist "%TEXTURES_DIR%" (
            python "%TOOLS_DIR%\sau_builder.py" "%PROJECT_DIR%level.h" "%OUTPUT_DIR%\level.sau" --textures "%TEXTURES_DIR%"
        ) else (
            echo [INFO] No textures directory found, creating SAU without textures
            python "%TOOLS_DIR%\sau_builder.py" "%PROJECT_DIR%level.h" "%OUTPUT_DIR%\level.sau"
        )
        
        if exist "%OUTPUT_DIR%\level.sau" (
            for %%A in ("%OUTPUT_DIR%\level.sau") do set "SIZE=%%~zA"
            echo [OK] level.sau created ^(!SIZE! bytes^)
        ) else (
            echo [WARNING] SAU creation may have failed
        )
    ) else (
        echo [WARNING] sau_builder.py not found, skipping SAU creation
    )
    
    REM Also copy text version as backup
    copy /Y "%PROJECT_DIR%level.h" "%OUTPUT_DIR%\level.h" >nul
    echo [OK] level.h copied as backup
) else (
    echo [WARNING] level.h not found
)

REM ================================================================
echo.
echo [STEP 6/7] Copying game executable and DLLs...
echo ----------------------------------------------------------------

set "GAME_EXE="

REM Check various build output locations
if exist "%PROJECT_DIR%x64\Release\DoomClone.exe" (
    set "GAME_EXE=%PROJECT_DIR%x64\Release\DoomClone.exe"
    set "EXE_CONFIG=x64 Release"
) else if exist "%PROJECT_DIR%x64\Debug\DoomClone.exe" (
    set "GAME_EXE=%PROJECT_DIR%x64\Debug\DoomClone.exe"
    set "EXE_CONFIG=x64 Debug"
) else if exist "%PROJECT_DIR%Release\DoomClone.exe" (
    set "GAME_EXE=%PROJECT_DIR%Release\DoomClone.exe"
    set "EXE_CONFIG=x86 Release"
) else if exist "%PROJECT_DIR%Debug\DoomClone.exe" (
    set "GAME_EXE=%PROJECT_DIR%Debug\DoomClone.exe"
    set "EXE_CONFIG=x86 Debug"
)

if defined GAME_EXE (
    copy /Y "!GAME_EXE!" "%OUTPUT_DIR%\DoomClone.exe" >nul
    echo [OK] DoomClone.exe copied ^(!EXE_CONFIG!^)
) else (
    echo [WARNING] DoomClone.exe not found
    echo          Build the solution in Visual Studio first!
)

REM Copy GLUT DLLs - check multiple locations
set "DLL_COUNT=0"

REM Check project directory
for %%D in (freeglut.dll glut32.dll freeglut32.dll) do (
    if exist "%PROJECT_DIR%%%D" (
        copy /Y "%PROJECT_DIR%%%D" "%OUTPUT_DIR%\" >nul
        set /a "DLL_COUNT+=1"
        echo [OK] %%D copied from project directory
    )
)

REM Check x64 directories
if exist "%PROJECT_DIR%x64\Release\freeglut.dll" (
    copy /Y "%PROJECT_DIR%x64\Release\freeglut.dll" "%OUTPUT_DIR%\" >nul
    set /a "DLL_COUNT+=1"
    echo [OK] freeglut.dll copied from x64\Release
)
if exist "%PROJECT_DIR%x64\Debug\freeglut.dll" (
    if not exist "%OUTPUT_DIR%\freeglut.dll" (
        copy /Y "%PROJECT_DIR%x64\Debug\freeglut.dll" "%OUTPUT_DIR%\" >nul
        set /a "DLL_COUNT+=1"
        echo [OK] freeglut.dll copied from x64\Debug
    )
)

REM Check common freeglut installation paths
if %DLL_COUNT% equ 0 (
    if exist "C:\freeglut\bin\x64\freeglut.dll" (
        copy /Y "C:\freeglut\bin\x64\freeglut.dll" "%OUTPUT_DIR%\" >nul
        set /a "DLL_COUNT+=1"
        echo [OK] freeglut.dll copied from C:\freeglut\bin\x64
    ) else if exist "C:\freeglut\bin\freeglut.dll" (
        copy /Y "C:\freeglut\bin\freeglut.dll" "%OUTPUT_DIR%\" >nul
        set /a "DLL_COUNT+=1"
        echo [OK] freeglut.dll copied from C:\freeglut\bin
    )
)

if %DLL_COUNT% equ 0 (
    echo [WARNING] freeglut.dll not found! Game may not run.
    echo          Please copy freeglut.dll to the output directory manually.
)

REM ================================================================
echo.
echo [STEP 7/7] Creating launchers and finalizing...
echo ----------------------------------------------------------------

REM Create launcher batch files
(
echo @echo off
echo cd /d "%%~dp0"
echo start "" "DoomClone.exe"
) > "%OUTPUT_DIR%\Play_Game.bat"

(
echo @echo off
echo cd /d "%%~dp0"
echo start "" "tools\OracularEditor.exe"
) > "%OUTPUT_DIR%\Launch_MapEditor.bat"

(
echo @echo off
echo cd /d "%%~dp0"
echo start "" "tools\TextureEditorPro.exe"
) > "%OUTPUT_DIR%\Launch_TextureEditor.bat"

echo [OK] Launcher scripts created

REM Copy sau_builder.py to tools for future use
if exist "%TOOLS_DIR%\sau_builder.py" (
    copy /Y "%TOOLS_DIR%\sau_builder.py" "%OUTPUT_DIR%\tools\" >nul
    echo [OK] sau_builder.py copied to tools
)

REM ================================================================
REM Cleanup temp files
echo.
echo Cleaning up temporary files...
if exist "%TEMP_DIR%" rd /s /q "%TEMP_DIR%" 2>nul
if exist "%SPEC_DIR%" rd /s /q "%SPEC_DIR%" 2>nul
echo [OK] Cleanup complete

REM ================================================================
echo.
echo ================================================================
echo                    BUILD COMPLETE!
echo ================================================================
echo.
echo  Package Location: %OUTPUT_DIR%
echo.
echo  Package Contents:
echo  -----------------
if exist "%OUTPUT_DIR%\DoomClone.exe" (
    echo    [GAME]    DoomClone.exe
) else (
    echo    [MISSING] DoomClone.exe - Build in Visual Studio first!
)
if exist "%OUTPUT_DIR%\freeglut.dll" (
    echo    [DLL]     freeglut.dll
) else (
    echo    [MISSING] freeglut.dll - Copy manually!
)
if exist "%OUTPUT_DIR%\level.sau" (
    echo    [DATA]    level.sau ^(binary level + textures^)
)
if exist "%OUTPUT_DIR%\level.h" (
    echo    [BACKUP]  level.h ^(text format backup^)
)
if exist "%OUTPUT_DIR%\tools\OracularEditor.exe" (
    echo    [TOOL]    tools\OracularEditor.exe ^(Map Editor^)
)
if exist "%OUTPUT_DIR%\tools\TextureEditorPro.exe" (
    echo    [TOOL]    tools\TextureEditorPro.exe ^(Texture Editor^)
)
if exist "%OUTPUT_DIR%\tools\sau_builder.py" (
    echo    [TOOL]    tools\sau_builder.py ^(SAU converter^)
)
echo.
echo  Launchers:
echo    - Play_Game.bat
echo    - Launch_MapEditor.bat  
echo    - Launch_TextureEditor.bat
echo.
echo ================================================================
echo.

REM Open the output folder
explorer "%OUTPUT_DIR%"

pause
