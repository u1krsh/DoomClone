@echo off
echo Starting Oracular Map Editor and DoomTest...

REM Change to the output directory
cd /d "%~dp0x64\Debug"

REM Start both executables
start "Oracular Map Editor" Oracular_map_editor.exe
start "DoomTest" DoomTest.exe

echo Both applications started!
pause
