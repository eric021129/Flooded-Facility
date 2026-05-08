#ifndef GAMEPLAY_FLOOD_H
#define GAMEPLAY_FLOOD_H

#include <vector>
#include <string>
#include <ctime>

// ---------------------------------------------------------
// Tile Types
// ---------------------------------------------------------
enum Tile {
    TILE_EMPTY,
    TILE_WALL,
    TILE_WATER,
    TILE_MACHINE
};

// ---------------------------------------------------------
// Machine Struct
// ---------------------------------------------------------
struct Machine {
    int x;
    int y;
};

// ---------------------------------------------------------
// FloodLevel Class
// ---------------------------------------------------------
class FloodLevel {
public:
    FloodLevel(int difficulty_, int stage_);
    bool run();

private:
    int difficulty;     // 1 = Easy, 2 = Medium, 3 = Hard
    int stage;          // stage 1–5 → map design

    int playerX;
    int playerY;

    int waterFrontX;    // current water boundary
    time_t lastFloodTime;

    std::vector<std::string> world;            // raw map text
    std::vector<std::vector<Tile>> tileMap;    // parsed map
    std::vector<Machine> machines;

    std::string statusMessage;

private:
    // -----------------------------------------------------
    // Map Setup
    // -----------------------------------------------------
    void initWorld();                // load map based on stage
    void initTileMapFromWorld();     // convert char → Tile

    // NEW: spawn player INSIDE wall + reachable
    void choosePlayerSpawnOnWall();

    // Machine placement (inside walls)
    void placeRandomMachines(int count);

    // -----------------------------------------------------
    // Drawing & Input
    // -----------------------------------------------------
    void draw(bool canInteract);
    void handleInput(bool canInteract, bool &gameOver, bool &needRedraw);

    // -----------------------------------------------------
    // Flood Logic
    // -----------------------------------------------------
    bool updateFlood(bool &needRedraw);
    bool playerInWater() const;

    bool isWalkable(Tile t) const;

    // -----------------------------------------------------
    // Machine Interaction
    // -----------------------------------------------------
    int findMachineForInteraction() const;

    // -----------------------------------------------------
    // Terminal Helpers
    // -----------------------------------------------------
    void clearScreen();
    void getTerminalSize(int &cols, int &rows);
    char getchNonBlocking(double timeoutSeconds);
};

#endif
