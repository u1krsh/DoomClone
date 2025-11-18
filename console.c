#include "console.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Global console instance
Console console;

// Global game state variables
int godMode = 0;

void initConsole(int screenWidth, int screenHeight) {
    console.active = 0;
    console.animating = 0;
    console.slidePos = 0.0f;
    console.input[0] = '\0';
    console.inputPos = 0;
    console.historyCount = 0;
    console.historyIndex = -1;
    console.screenWidth = screenWidth;
    console.screenHeight = screenHeight;
    console.messageCount = 0;
    
    // Initialize history
    for (int i = 0; i < CONSOLE_HISTORY_SIZE; i++) {
        console.history[i][0] = '\0';
    }
    
    // Initialize messages
    for (int i = 0; i < CONSOLE_MESSAGE_LINES; i++) {
        console.messages[i][0] = '\0';
    }
}

void toggleConsole() {
    console.active = !console.active;
    console.animating = 1;
    console.historyIndex = -1; // Reset history browsing when toggling
}

void updateConsole() {
    if (console.animating) {
        if (console.active) {
            console.slidePos += 0.15f;
            if (console.slidePos >= 1.0f) {
                console.slidePos = 1.0f;
                console.animating = 0;
            }
        } else {
            console.slidePos -= 0.15f;
            if (console.slidePos <= 0.0f) {
                console.slidePos = 0.0f;
                console.animating = 0;
            }
        }
    }
}

void drawConsole(int screenWidth, int screenHeight) {
    if (console.slidePos <= 0.0f) return;
    
    // This will be called from your main render function
    // You'll need to pass the pixel drawing function to this
}

void consoleAddChar(char c) {
    if (console.inputPos < MAX_CONSOLE_INPUT - 1 && isprint(c)) {
        console.input[console.inputPos] = c;
        console.inputPos++;
        console.input[console.inputPos] = '\0';
    }
}

void consoleBackspace() {
    if (console.inputPos > 0) {
        console.inputPos--;
        console.input[console.inputPos] = '\0';
    }
}

// Print message to console
void consolePrint(const char* message) {
    if (console.messageCount < CONSOLE_MESSAGE_LINES) {
        // Add new message
        strcpy_s(console.messages[console.messageCount], MAX_CONSOLE_INPUT, message);
        console.messageCount++;
    } else {
        // Shift messages up and add new one at bottom
        for (int i = 0; i < CONSOLE_MESSAGE_LINES - 1; i++) {
            strcpy_s(console.messages[i], MAX_CONSOLE_INPUT, console.messages[i + 1]);
        }
        strcpy_s(console.messages[CONSOLE_MESSAGE_LINES - 1], MAX_CONSOLE_INPUT, message);
    }
}

// Trim whitespace from string
void trim(char* str) {
    int start = 0;
    int end = strlen(str) - 1;
    
    // Trim leading whitespace
    while (isspace(str[start])) start++;
    
    // Trim trailing whitespace
    while (end >= start && isspace(str[end])) end--;
    
    // Move trimmed string to beginning
    int i;
    for (i = 0; i <= end - start; i++) {
        str[i] = str[start + i];
    }
    str[i] = '\0';
}

// Convert string to lowercase
void toLowerCase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void consoleExecuteCommand() {
    if (console.inputPos == 0) return;
    
    // Add to history
    if (console.historyCount < CONSOLE_HISTORY_SIZE) {
        strcpy_s(console.history[console.historyCount], MAX_CONSOLE_INPUT, console.input);
        console.historyCount++;
    } else {
        // Shift history up and add new command at end
        for (int i = 0; i < CONSOLE_HISTORY_SIZE - 1; i++) {
            strcpy_s(console.history[i], MAX_CONSOLE_INPUT, console.history[i + 1]);
        }
        strcpy_s(console.history[CONSOLE_HISTORY_SIZE - 1], MAX_CONSOLE_INPUT, console.input);
    }
    
    // Process command
    char command[MAX_CONSOLE_INPUT];
    strcpy_s(command, MAX_CONSOLE_INPUT, console.input);
    trim(command);
    toLowerCase(command);
    
    // Execute command
    if (strcmp(command, "godmode") == 0) {
        godMode = !godMode;
        if (godMode) {
            consolePrint("God mode ENABLED");
        } else {
            consolePrint("God mode DISABLED");
        }
    }
    else if (strcmp(command, "help") == 0) {
        consolePrint("Available commands:");
        consolePrint("  godmode - Toggle god mode");
        consolePrint("  help - Show this help");
        consolePrint("  clear - Clear console");
    }
    else if (strcmp(command, "clear") == 0) {
        // Clear all messages
        for (int i = 0; i < CONSOLE_MESSAGE_LINES; i++) {
            console.messages[i][0] = '\0';
        }
        console.messageCount = 0;
        consolePrint("Console cleared");
    }
    else if (strlen(command) > 0) {
        consolePrint("That is not a command, bitch");
    }
    
    // Clear input
    console.input[0] = '\0';
    console.inputPos = 0;
}

void consoleHandleKey(unsigned char key) {
    if (key == 13) { // Enter key
        consoleExecuteCommand();
    }
    else if (key == 8 || key == 127) { // Backspace
        consoleBackspace();
    }
    else {
        consoleAddChar(key);
    }
}
