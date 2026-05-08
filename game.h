#ifndef GAME_H
#define GAME_H

#include <string>
#include <termios.h>

class Game {
public:
    Game();
    ~Game();

    void run();   // entry point

private:
    // ---------- terminal state ----------
    termios old_tio, new_tio;
    void setupTerminal();
    void resetTerminal();
    void enableInstantInput(); // raw mode (no enter needed)
    void enableLineInput();    // normal mode (for getline)
    void clearScreen();

    // center-print helper
    void printCenteredBlock(const std::string &block);

    // ---------- game state ----------
    int difficulty;          // 1=Easy, 2=Medium, 3=Hard
    bool first_time_play;    // for intro cutscene once
    std::string player_data_file;

    struct SaveSlot {
        bool        exists = false;
        std::string name;
        int         stage = 1;
        int         difficulty = 1;
    } saveSlot;

    // ---------- UI flow ----------
    void showTitleScreen();       // Screen 1: Flooded Facility
    void showMainMenu();          // Screen 2: [START GAME] / [QUIT]
    void showSaveLoadScreen();    // Screen 3: MyGame + [NEW GAME]
    void showNewGameFlow();       // Screen 4: Difficulty + Title
    void showDifficultySelect();  // difficulty picker
    std::string promptRecordTitle(); // Title: xxxxx

    // ---------- story / text ----------
    void typeText(const std::string &text);
    void showIntroCutscene();

    // ---------- save / load ----------
    void loadPlayerData();
    void savePlayerData();

    // ---------- gameplay ----------
    void startGameFromSave(); // uses FloodLevel
};

#endif
