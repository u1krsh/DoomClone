# Quake-Style Console System

## Overview
A Quake-style dropdown console system for the DoomClone project. Press the backtick (`) key to toggle the console.

## Features
- **Slide-down animation**: The console smoothly slides down from the top of the screen
- **Command input**: Type commands directly into the console
- **Command history**: Previous commands are stored (up to 10 commands)
- **In-console messages**: Command responses and messages displayed within the console (not system console)
- **Message history**: Displays up to 5 lines of recent messages
- **Resolution-aware scaling**: Automatically scales font and console size based on screen resolution
- **8x8 Bitmap Font**: Clean, retro-style font rendering (8514OEM style) with automatic scaling
- **Visual feedback**: Yellow prompt (>) and blinking cursor

## Console Display
The console displays:
- **Top section**: Recent messages (up to 5 lines) showing command responses and feedback
- **Bottom section**: Input line with `>` prompt where you type commands
- **Messages scroll up** as new messages are added

## Resolution Scaling
The console automatically adapts to different resolutions:
- **Low Resolution** (< 480p): Font scale = 1x
- **Medium Resolution** (480p - 719p): Font scale = 2x  
- **High Resolution** (720p - 1079p): Font scale = 3x
- **Very High Resolution** (1080p+): Font scale = 4x

Console height is dynamically set to 25% of screen height, ensuring consistent appearance across all resolutions.

## Controls
- **`** (backtick): Toggle console on/off
- **Enter**: Execute the current command
- **Backspace**: Delete the last character

## Available Commands

### godmode
Toggles god mode on/off. When god mode is enabled, you can fly (move on the Z-axis).
- **Default**: OFF (cannot fly)
- **When enabled**: Can use W/S with M key held to fly up/down
- **Usage**: Type `godmode` and press Enter
- **Response**: "God mode ENABLED" or "God mode DISABLED" (displayed in console)

### help
Displays a list of available commands.
- **Usage**: Type `help` and press Enter
- **Response**: Shows list of available commands in the console

### clear
Clears all messages from the console display.
- **Usage**: Type `clear` and press Enter
- **Response**: "Console cleared" (displayed in console)

### Invalid Commands
If you type anything that isn't a recognized command, the console will respond with:
- **Response**: "That is not a command, bitch" (displayed in console)

## Message System
All command responses and feedback are displayed directly in the console window:
- Messages appear in white text
- Up to 5 message lines are displayed at once
- Older messages scroll off the top as new ones arrive
- The `clear` command removes all displayed messages
- Messages are persistent until cleared or pushed off by new messages

## How It Works

### God Mode Behavior
- **God Mode OFF (default)**: 
  - Normal movement with W/A/S/D
  - Can look up/down with M + A/D
  - Cannot change Z position (fly)
  
- **God Mode ON**:
  - All normal movement controls work
  - M + W: Fly up
  - M + S: Fly down
  - M + A/D: Look up/down

### File Structure
- **console.h**: Header file with console structure and function declarations
- **console.c**: Implementation of console functionality with message system
- **console_font.h**: 8x8 bitmap font definitions (ASCII 32-126) with scaling support
- **console_font.c**: Font rendering functions with resolution-aware scaling
- **DoomTest.c**: Main game file, integrated with console

## Font System
The console uses an 8x8 bitmap font system inspired by the classic 8514OEM font, with automatic scaling support. Each character:
- Base size: 8 pixels wide and 8 pixels tall
- Scales automatically: 1x, 2x, 3x, or 4x based on resolution
- Supports all printable ASCII characters (32-126)
- Renders cleanly at all resolutions
- Uses bitmap data for crisp, retro appearance
- Correctly oriented (fixed from initial inverted display)

### Texture Rendering Improvements
At higher resolutions, texture coordinate clamping has been improved to prevent:
- Out-of-bounds texture access
- Visual artifacts when close to walls
- Texture warping or glitches
- Memory access violations

## Integration Details
The console module is loaded into `DoomTest.c` with `#include "console.h"`. 

Key integration points:
1. **Input handling**: Keyboard input is routed to the console when active
2. **Movement blocking**: Player movement is disabled when console is open
3. **Rendering**: Console is drawn on top of the game view with resolution-scaled bitmap font
4. **Message system**: `consolePrint()` function displays messages in-console instead of system console
5. **State management**: `godMode` global variable controls flying ability
6. **Resolution awareness**: Console and font automatically scale with screen resolution

## Adding Custom Messages
To display messages in the console from your code:

```c
// Include the console header
#include "console.h"

// Display a message
consolePrint("Your message here");
```

## Future Enhancements
You can easily add more commands by modifying the `consoleExecuteCommand()` function in `console.c`:

```c
else if (strcmp(command, "yourcommand") == 0) {
    // Your command logic here
    consolePrint("Command executed");
}
```

## Technical Notes
- Console height: 25% of screen height
- Max input length: 128 characters
- History size: 10 commands
- Message lines displayed: 5
- Animation speed: 0.15 units per frame
- Console slides from top of screen
- Font: 8x8 bitmap font (8514OEM style) with automatic scaling
- Character spacing: 8 pixels * scale factor
- Supported resolutions: All resolutions from 160x120 to 1920x1080+
- Messages are displayed in-console, not in system/debug console
