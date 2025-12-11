#define _CRT_SECURE_NO_WARNINGS
#include "console.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Global console instance
Console console;

// Global game state variables
int godMode = 0;
int noclip = 0;

// Enemy system external reference
extern int enemiesEnabled;
extern int playerHealth;
extern int playerMaxHealth;
extern int playerArmor;
extern int playerMaxArmor;
extern int playerDead;
extern int enemiesKilled;
extern int totalEnemiesSpawned;

// Forward declarations for external functions
extern void healPlayer(int amount);
extern void addArmor(int amount);

void initConsole(int screenWidth, int screenHeight) {
    console.active = 0;
    console.slidePos = 0.0f;
    console.animating = 0;
    console.screenWidth = screenWidth;
    console.screenHeight = screenHeight;
    console.input[0] = '\0';
    console.inputPos = 0;
    console.messageCount = 0;
    
    // Clear message history
    for (int i = 0; i < CONSOLE_MESSAGE_LINES; i++) {
        console.messages[i][0] = '\0';
    }
    
    consolePrint("Console initialized. Type 'help' for commands.");
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
        strncpy(console.messages[console.messageCount], message, MAX_CONSOLE_INPUT);
        console.messageCount++;
    } else {
        // Shift messages up and add new one at bottom
        for (int i = 0; i < CONSOLE_MESSAGE_LINES - 1; i++) {
            strncpy(console.messages[i], console.messages[i + 1], MAX_CONSOLE_INPUT);
        }
        strncpy(console.messages[CONSOLE_MESSAGE_LINES - 1], message, MAX_CONSOLE_INPUT);
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

// Parse command and argument
void parseCommand(const char* input, char* cmd, char* arg) {
    cmd[0] = '\0';
    arg[0] = '\0';
    
    // Skip leading whitespace
    while (*input && isspace(*input)) input++;
    
    // Copy command (until space or end)
    int i = 0;
    while (*input && !isspace(*input) && i < MAX_CONSOLE_INPUT - 1) {
        cmd[i++] = *input++;
    }
    cmd[i] = '\0';
    
    // Skip whitespace between command and argument
    while (*input && isspace(*input)) input++;
    
    // Copy argument (rest of string)
    i = 0;
    while (*input && i < MAX_CONSOLE_INPUT - 1) {
        arg[i++] = *input++;
    }
    arg[i] = '\0';
}

void consoleExecuteCommand() {
    if (console.inputPos == 0) return;
    
    // Add to history
    if (console.historyCount < CONSOLE_HISTORY_SIZE) {
        strncpy(console.history[console.historyCount], console.input, MAX_CONSOLE_INPUT);
        console.historyCount++;
    } else {
        // Shift history up and add new command at end
        for (int i = 0; i < CONSOLE_HISTORY_SIZE - 1; i++) {
            strncpy(console.history[i], console.history[i + 1], MAX_CONSOLE_INPUT);
        }
        strncpy(console.history[CONSOLE_HISTORY_SIZE - 1], console.input, MAX_CONSOLE_INPUT);
    }
    
    // Process command
    char command[MAX_CONSOLE_INPUT];
    char argument[MAX_CONSOLE_INPUT];
    
    // Make a copy for processing
    char inputCopy[MAX_CONSOLE_INPUT];
    strncpy(inputCopy, console.input, MAX_CONSOLE_INPUT);
    trim(inputCopy);
    
    // Parse command and argument
    parseCommand(inputCopy, command, argument);
    toLowerCase(command);
    
    char response[MAX_CONSOLE_INPUT];
    
    // Execute command
    if (strcmp(command, "godmode") == 0 || strcmp(command, "god") == 0) {
        godMode = !godMode;
        if (godMode) {
            consolePrint("God mode ENABLED - You are invincible!");
        } else {
            consolePrint("God mode DISABLED");
        }
    }
    else if (strcmp(command, "noclip") == 0) {
        noclip = !noclip;
        if (noclip) {
            consolePrint("Noclip ENABLED - Walk through walls");
        }   
        else {
            consolePrint("Noclip DISABLED");
        }
    }
    else if (strcmp(command, "noenemies") == 0 || strcmp(command, "nomonsters") == 0 || 
             strcmp(command, "notarget") == 0) {
        enemiesEnabled = !enemiesEnabled;
        if (!enemiesEnabled) {
            consolePrint("Enemies DISABLED");
        }   
        else {
            consolePrint("Enemies ENABLED");
        }
    }
    else if (strcmp(command, "health") == 0) {
        if (strlen(argument) > 0) {
            int amount = atoi(argument);
            if (amount > 0) {
                playerHealth = amount;
                if (playerHealth > 200) playerHealth = 200;
                playerMaxHealth = playerHealth > 100 ? playerHealth : 100;
                playerDead = 0;
                snprintf(response, sizeof(response), "Health set to %d", playerHealth);
                consolePrint(response);
            } else {
                consolePrint("Usage: health <amount> (1-200)");
            }
        } else {
            snprintf(response, sizeof(response), "Current health: %d/%d", playerHealth, playerMaxHealth);
            consolePrint(response);
        }
    }
    else if (strcmp(command, "armor") == 0) {
        if (strlen(argument) > 0) {
            int amount = atoi(argument);
            if (amount >= 0) {
                playerArmor = amount;
                if (playerArmor > 200) playerArmor = 200;
                playerMaxArmor = playerArmor > 100 ? playerArmor : 100;
                snprintf(response, sizeof(response), "Armor set to %d", playerArmor);
                consolePrint(response);
            } else {
                consolePrint("Usage: armor <amount> (0-200)");
            }
        } else {
            snprintf(response, sizeof(response), "Current armor: %d/%d", playerArmor, playerMaxArmor);
            consolePrint(response);
        }
    }
    else if (strcmp(command, "give") == 0) {
        toLowerCase(argument);
        if (strcmp(argument, "health") == 0 || strcmp(argument, "h") == 0) {
            healPlayer(100);
            consolePrint("Gave 100 health");
        }
        else if (strcmp(argument, "armor") == 0 || strcmp(argument, "a") == 0) {
            addArmor(100);
            consolePrint("Gave 100 armor");
        }
        else if (strcmp(argument, "weapons") == 0 || strcmp(argument, "w") == 0) {
            extern void giveAllWeapons(void);
            giveAllWeapons();
            consolePrint("Gave all weapons and ammo");
        }
        else if (strcmp(argument, "ammo") == 0) {
            extern void giveAllWeapons(void);
            giveAllWeapons();
            consolePrint("Max ammo for all weapons");
        }
        else if (strcmp(argument, "all") == 0) {
            extern void giveAllWeapons(void);
            playerHealth = 200;
            playerMaxHealth = 200;
            playerArmor = 200;
            playerMaxArmor = 200;
            playerDead = 0;
            giveAllWeapons();
            consolePrint("Gave all items - 200 health, 200 armor, all weapons");
        }
        else {
            consolePrint("Usage: give <health|armor|weapons|ammo|all>");
        }
    }
    else if (strcmp(command, "kill") == 0) {
        toLowerCase(argument);
        if (strcmp(argument, "enemies") == 0 || strcmp(argument, "all") == 0) {
            // Kill all enemies using extern function
            extern void killAllEnemies(int currentTime);
            killAllEnemies(0);
            consolePrint("All enemies killed!");
        }
        else if (strcmp(argument, "me") == 0 || strcmp(argument, "self") == 0) {
            if (!godMode) {
                playerHealth = 0;
                playerDead = 1;
                consolePrint("You killed yourself");
            } else {
                consolePrint("Cannot die in god mode");
            }
        }
        else {
            consolePrint("Usage: kill <enemies|me>");
        }
    }
    else if (strcmp(command, "stats") == 0) {
        snprintf(response, sizeof(response), "Health: %d/%d  Armor: %d/%d", 
                 playerHealth, playerMaxHealth, playerArmor, playerMaxArmor);
        consolePrint(response);
        snprintf(response, sizeof(response), "Enemies killed: %d/%d", 
                 enemiesKilled, totalEnemiesSpawned);
        consolePrint(response);
    }
    else if (strcmp(command, "resurrect") == 0 || strcmp(command, "respawn") == 0) {
        if (playerDead) {
            playerHealth = 100;
            playerMaxHealth = 100;
            playerDead = 0;
            consolePrint("You have been resurrected");
        } else {
            consolePrint("You are not dead");
        }
    }
    else if (strcmp(command, "help") == 0) {
        consolePrint("=== CHEAT COMMANDS ===");
        consolePrint("  god/godmode - Toggle invincibility");
        consolePrint("  noclip - Walk through walls");
        consolePrint("  notarget - Toggle enemy AI");
        consolePrint("  give <health|armor|all> - Get items");
        consolePrint("  health [amount] - Set/view health");
        consolePrint("  armor [amount] - Set/view armor");
        consolePrint("  kill <enemies|me> - Kill targets");
        consolePrint("  resurrect - Revive after death");
        consolePrint("  stats - View game statistics");
        consolePrint("=== UTILITY COMMANDS ===");
        consolePrint("  clear - Clear console");
        consolePrint("  text_edit - Launch texture editor");
        consolePrint("  map_edit - Launch map editor");
    }
    else if (strcmp(command, "clear") == 0) {
        // Clear all messages
        for (int i = 0; i < CONSOLE_MESSAGE_LINES; i++) {
            console.messages[i][0] = '\0';
        }
        console.messageCount = 0;
        consolePrint("Console cleared");
    }
    else if (strcmp(command, "text_edit") == 0 || strcmp(command, "textedit") == 0) {
        consolePrint("Launching Texture Editor...");
        #ifdef _WIN32
        // Launch the Python texture editor
        system("start python tools\\texture_editor_pro.py");
        #else
        system("python tools/texture_editor_pro.py &");
        #endif
    }
    else if (strcmp(command, "map_edit") == 0 || strcmp(command, "mapedit") == 0) {
        consolePrint("Launching Map Editor...");
        #ifdef _WIN32
        // Launch the Python map editor
        system("start python tools\\oracular_editor.py");
        #else
        system("python tools/oracular_editor.py &");
        #endif
    }
    else if (strlen(command) > 0) {
        snprintf(response, sizeof(response), "Unknown command: %s", command);
        consolePrint(response);
        consolePrint("Type 'help' for available commands");
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
