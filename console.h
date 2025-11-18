#ifndef CONSOLE_H
#define CONSOLE_H

#include "console_font.h"

#define MAX_CONSOLE_INPUT 128
#define CONSOLE_HEIGHT_PERCENT 0.25f  // Console height as percentage of screen (25%)
#define CONSOLE_HISTORY_SIZE 10
#define CONSOLE_MESSAGE_LINES 5  // Number of message lines to display

typedef struct {
    int active;           // Is console visible?
    int animating;        // Is console sliding?
    float slidePos;       // Current slide position (0.0 = hidden, 1.0 = visible)
    char input[MAX_CONSOLE_INPUT];  // Current input text
    int inputPos;         // Cursor position in input
    char history[CONSOLE_HISTORY_SIZE][MAX_CONSOLE_INPUT]; // Command history
    int historyCount;     // Number of commands in history
    int historyIndex;     // Current position in history browsing
    int screenWidth;      // Store screen dimensions for scaling
    int screenHeight;
    char messages[CONSOLE_MESSAGE_LINES][MAX_CONSOLE_INPUT]; // Message buffer
    int messageCount;     // Number of messages
} Console;

// Global console instance
extern Console console;

// Global game state variables
extern int godMode;

// Console functions
void initConsole(int screenWidth, int screenHeight);
void toggleConsole();
void updateConsole();
void drawConsole(int screenWidth, int screenHeight);
void consoleAddChar(char c);
void consoleBackspace();
void consoleExecuteCommand();
void consoleHandleKey(unsigned char key);
void consolePrint(const char* message);  // Print message to console

#endif // CONSOLE_H
