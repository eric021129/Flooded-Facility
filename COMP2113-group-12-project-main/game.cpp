#include "game.h"
#include "gameplay_flood.h"

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sys/ioctl.h>  // for TIOCGWINSZ
#include <cstdio>       // for printf
#include <sstream>
#include <vector>

using namespace std;

// -------------------------------------------------
// Helpers
// -------------------------------------------------
static string difficultyToString(int d) {
    switch (d) {
        case 1: return "Easy";
        case 2: return "Medium";
        case 3: return "Hard";
        default: return "Unknown";
    }
}

// Helper: get terminal size
static void getTerminalSize(int &cols, int &rows) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    cols = w.ws_col;
    rows = w.ws_row;
}

// -------------------------------------------------
// Game ctor / dtor
// -------------------------------------------------
Game::Game()
    : difficulty(1),
      first_time_play(true)
{
    setupTerminal();
    loadPlayerData();
}

Game::~Game() {
    resetTerminal();
}

// -------------------------------------------------
// Terminal helpers
// -------------------------------------------------
void Game::setupTerminal() {
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    // instant key mode: no line buffering, no echo
    new_tio.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void Game::resetTerminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

void Game::enableInstantInput() {
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void Game::enableLineInput() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

void Game::clearScreen() {
    cout << "\033[2J\033[H";
}

// Center-print a multi-line text block
void Game::printCenteredBlock(const string &block) {
    int cols, rows;
    getTerminalSize(cols, rows);

    vector<string> lines;
    istringstream iss(block);
    string line;
    while (getline(iss, line)) {
        lines.push_back(line);
    }

    int totalLines = (int)lines.size();
    int startRow = (rows - totalLines) / 2;
    if (startRow < 1) startRow = 1;

    clearScreen();

    for (int i = 0; i < totalLines; ++i) {
        const string &ln = lines[i];
        int len = (int)ln.size();
        int startCol = (cols - len) / 2;
        if (startCol < 1) startCol = 1;

        // Move cursor and print line
        printf("\033[%d;%dH%s", startRow + i, startCol, ln.c_str());
    }

    cout.flush();
}

// -------------------------------------------------
// Title Screen (Screen 1)
// -------------------------------------------------
void Game::showTitleScreen() {
    string block = R"(
 ________  __                        __              __  
|_   __  |[  |                      |  ]            |  ] 
  | |_ \_| | |  .--.    .--.    .--.| | .---.   .--.| |  
  |  _|    | |/ .'`\ \/ .'`\ \/ /'`\' |/ /__\\/ /'`\' |  
 _| |_     | || \__. || \__. || \__/  || \__.,| \__/  |  
|_____|   [___]'.__.'  '.__.'  '.__.;__]'.__.' '.__.;__] 
    ________                _   __    _   _              
   |_   __  |              (_) [  |  (_) / |_            
     | |_ \_|,--.   .---.  __   | |  __ `| |-' _   __    
     |  _|  `'_\ : / /'`\][  |  | | [  | | |  [ \ [  ]   
    _| |_   // | |,| \__.  | |  | |  | | | |,  \ '/ /    
   |_____|  \'-;__/'.___.'[___][___][___]\__/[\_:  /     
                                              \__.'         

PRESS ANY KEY TO CONTINUE
)";

    printCenteredBlock(block);

    char c;
    read(STDIN_FILENO, &c, 1);
}

// -------------------------------------------------
// Cutscene helpers
// -------------------------------------------------
void Game::typeText(const string& text) {
    for (char c : text) {
        cout << c << flush;
        usleep(20000);
    }
}

void Game::showIntroCutscene() {
    clearScreen();
    int cols, rows;
    getTerminalSize(cols, rows);

    int row = rows / 2 - 2;
    if (row < 1) row = 1;
    
    printf("\033[%d;%dH", row, 1);
    typeText("You are Dr. Alex Chen, a C++ programmer working late at the");
    cout << "\n";
    typeText("AquaTech Research Facility...");
    cout << "\n\n";
    usleep(100000);
    
    typeText("**WATER CONTAINMENT FAILURE!**");
    cout << "\n";
    typeText("Suddenly, rooms start flooding one by one.");
    cout << "\n\n";
    usleep(100000);
    
    typeText("You must repair the system and stop the flood before it's too late!");
    cout << "\n\n\n";
    
    cout << "Press any key to continue...";
    cout.flush();
    
    tcflush(STDIN_FILENO, TCIFLUSH);
    
    char c;
    read(STDIN_FILENO, &c, 1);
    
    first_time_play = false;
    
    // TO DO: FAILURE SCREEN TO BE LONGER (WHEN TOUCHING WATER)
    // TO DO: CHANGE GAME WITHOUT "ALREADY FILLED!"
    // TO DO: "PRESS ENTER" IN GAME TAKES TWO ENTERS
    // TO DO: "WATER FILLS EVERY 10S" CHANGE BASED ON DIFFICULTY
}

// -------------------------------------------------
// Save / Load
// -------------------------------------------------
void Game::loadPlayerData() {
    player_data_file = "player_data.txt";
    saveSlot.exists = false;

    ifstream in(player_data_file);
    if (!in) {
        // no save yet
        return;
    }

    int existsFlag;
    if (!(in >> existsFlag)) return;
    in.ignore(numeric_limits<streamsize>::max(), '\n');

    if (existsFlag == 1) {
        saveSlot.exists = true;
        getline(in, saveSlot.name);
        in >> saveSlot.stage;
        in >> saveSlot.difficulty;
        difficulty = saveSlot.difficulty;
        // player has played before
        first_time_play = false;
    }
}

void Game::savePlayerData() {
    ofstream out(player_data_file);
    if (!out) return;

    out << (saveSlot.exists ? 1 : 0) << "\n";
    if (saveSlot.exists) {
        out << saveSlot.name << "\n";
        out << saveSlot.stage << "\n";
        out << saveSlot.difficulty << "\n";
    }
}

// -------------------------------------------------
// Main Menu (Screen 2)
// -------------------------------------------------
void Game::showMainMenu() {
    int selection = 0;
    char input;

    while (true) {
        ostringstream oss;

        oss << R"( 
   _____     _        _____             
  |     |___|_|___   |     |___ ___ _ _ 
  | | | | .'| |   |  | | | | -_|   | | |
  |_|_|_|__,|_|_|_|  |_|_|_|___|_|_|___|
)";
        oss << "\n\n";
        oss << (selection == 0 ? ">" : " ") << " [START GAME]\n";
        oss << (selection == 1 ? ">" : " ") << " [QUIT]\n\n";
        oss << "Use W/S to navigate, ENTER to select";

        printCenteredBlock(oss.str());

        if (read(STDIN_FILENO, &input, 1) == 1) {
            switch (input) {
                case 'w': case 'W':
                    selection = (selection - 1 + 2) % 2;
                    break;
                case 's': case 'S':
                    selection = (selection + 1) % 2;
                    break;
                case '\n': case ' ':
                    if (selection == 0) {
                        showSaveLoadScreen();   // → Screen 3
                    } else if (selection == 1) {
                        clearScreen();
                        cout << "Thanks for playing!\n";
                        cout.flush();
                        resetTerminal();
                        exit(0);
                    }
                    break;
                case 'q': case 'Q':
                    clearScreen();
                    cout << "Thanks for playing!\n";
                    cout.flush();
                    resetTerminal();
                    exit(0);
            }
        }

        usleep(100000);
    }
}

// -------------------------------------------------
// Save/Load Screen (Screen 3)
// -------------------------------------------------
void Game::showSaveLoadScreen() {
    char input;
    int selection = (saveSlot.exists ? 0 : 1); // 0 = saved game, 1 = NEW GAME

    while (true) {
        ostringstream oss;

        oss << "Please choose a save slot.\n\n";
        if (saveSlot.exists) {
            oss << (selection == 0 ? ">" : " ")
                << " " << saveSlot.name
                << "  [stage: " << saveSlot.stage
                << " | Difficulty: " << difficultyToString(saveSlot.difficulty)
                << "]\n";
        } else {
            oss << "(No saved game)\n";
        }

        oss << (selection == 1 ? ">" : " ")
            << " [NEW GAME]\n\n";

        oss << "Use W/S to navigate, ENTER to select, Q to return to main menu";

        printCenteredBlock(oss.str());

        if (read(STDIN_FILENO, &input, 1) == 1) {
            switch (input) {
                case 'w': case 'W':
                    if (saveSlot.exists)
                        selection = (selection - 1 + 2) % 2;
                    else
                        selection = 1;
                    break;
                case 's': case 'S':
                    if (saveSlot.exists)
                        selection = (selection + 1) % 2;
                    else
                        selection = 1;
                    break;
                case 'q': case 'Q':
                    return; // back to Main Menu
                case '\n': case ' ':
                    if (selection == 0 && saveSlot.exists) {
                        startGameFromSave();
                    } else if (selection == 1) {
                        showNewGameFlow(); // Difficulty + Title
                    }
                    break;
            }
        }

        usleep(100000);
    }
}

// -------------------------------------------------
// New Game Flow (Screen 4: Difficulty + Title)
// -------------------------------------------------
void Game::showNewGameFlow() {
    // 1) Difficulty select
    showDifficultySelect();

    // 2) Title input (canonical mode, wait for ENTER)
    string title = promptRecordTitle();

    // 3) Initialize save
    saveSlot.exists     = true;
    saveSlot.name       = title;
    saveSlot.stage      = 1;              // start from Stage 1
    saveSlot.difficulty = difficulty;

    first_time_play = true;  // show intro cutscene on first play
    savePlayerData();
}

// Difficulty select (centered)
void Game::showDifficultySelect() {
    int selection = difficulty - 1;
    char input;

    while (true) {
        ostringstream oss;

        oss << "Difficulty:\n\n";
        oss << (selection == 0 ? ">" : " ") << " [EASY]\n";
        oss << (selection == 1 ? ">" : " ") << " [MEDIUM]\n";
        oss << (selection == 2 ? ">" : " ") << " [HARD]\n\n";
        oss << "Use W/S to navigate, ENTER to select";

        printCenteredBlock(oss.str());

        if (read(STDIN_FILENO, &input, 1) == 1) {
            switch (input) {
                case 'w': case 'W':
                    selection = (selection - 1 + 3) % 3;
                    break;
                case 's': case 'S':
                    selection = (selection + 1) % 3;
                    break;
                case '\n': case ' ':
                    difficulty = selection + 1;
                    return;
            }
        }

        usleep(100000);
    }
}

// Title input (centered, line mode)
string Game::promptRecordTitle() {
    // clear pending raw input
    tcflush(STDIN_FILENO, TCIFLUSH);
    enableLineInput();

    int cols, rows;
    getTerminalSize(cols, rows);

    clearScreen();

    string label = "Save Title: ";
    int row = rows / 2;
    int col = (cols - (int)label.size()) / 2;
    if (col < 1) col = 1;

    printf("\033[%d;%dH%s", row, col, label.c_str());
    cout.flush();

    string title;
    getline(cin, title);
    if (title.empty()) title = "MyGame";

    enableInstantInput();
    return title;
}

// -------------------------------------------------
// Gameplay from Save
// -------------------------------------------------
void Game::startGameFromSave() {
    // Optional: show intro cutscene only first time
    if (first_time_play) {
        showIntroCutscene();
    }

    // Pass both difficulty and stage to FloodLevel
    FloodLevel level(difficulty, saveSlot.stage);
    bool success = level.run();

    if (success) {
        // Advance stage, but cap at 5 since you have 5 stage maps
        if (saveSlot.stage < 5) {
            saveSlot.stage++;
        }
        savePlayerData();
    }
}

// -------------------------------------------------
void Game::run() {
    showTitleScreen();  // Screen 1
    showMainMenu();     // Screen 2 → flows to 3 & 4
}
