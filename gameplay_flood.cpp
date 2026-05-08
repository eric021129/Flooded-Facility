#include "gameplay_flood.h"
#include "PuzzleInterface.h" 
#include "MediumPuzzle.h"
#include "HardPuzzle.h"

#include <iostream>
#include <cstdlib>
#include <unistd.h>      // read, usleep
#include <sys/ioctl.h>   // TIOCGWINSZ
#include <sys/select.h>  // select, fd_set, timeval
#include <cstdio>        // printf
#include <cmath>
#include <sstream>
#include <ctime>

using namespace std;

// ----------------------------------------------------------
// Constructor
// ----------------------------------------------------------
FloodLevel::FloodLevel(int difficulty_, int stage_)
    : difficulty(difficulty_),
      stage(stage_),
      playerX(1),
      playerY(1),
      waterFrontX(-1),
      lastFloodTime(0)
{
    initWorld();
    initTileMapFromWorld();

    // Fixed spawn positions by stage
    switch (stage) {
        case 1:
            playerX = 1;
            playerY = 1;
            break;
        case 2:
            playerX = 3;
            playerY = 1;
            break;
        case 3:
            playerX = 10;
            playerY = 1;
            break;
        case 4:
            playerX = 1;
            playerY = 1;
            break;
        case 5:
            playerX = 1;
            playerY = 1;
            break;
        default:
            playerX = 1;
            playerY = 1;
            break;
    }

    // Number of machines depends on difficulty
    int machineCount = 0;
    if (difficulty == 1)      machineCount = 2; // Easy
    else if (difficulty == 2) machineCount = 3; // Medium
    else if (difficulty == 3) machineCount = 4; // Hard
    else                      machineCount = 1; // fallback

    placeRandomMachines(machineCount);
    lastFloodTime = time(nullptr);
}

// ----------------------------------------------------------
// Stage-based maps
// ----------------------------------------------------------
void FloodLevel::initWorld() {
    int s = stage;
    if (s < 1) s = 1;
    if (s > 5) s = 5;

    if (s == 1) {
        world = {
            "##################################################",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "##################################################"
        };
    }
    else if (s == 2) {
        world = {
            "##################################################",
            " #                                               ~",
            "  #                                              ~",
            "   #                                             ~",
            "    #                                            ~",
            "     #                                           ~",
            "      #                                          ~",
            "       #                                         ~",
            "        #                                        ~",
            "         #########################################"
        };
    }
    else if (s == 3) {
        world = {
            "         #########################################",
            "        #                                        ~",
            "       #                                         ~",
            "      #                                          ~",
            "     #                                           ~",
            "    #                                            ~",
            "   #                                             ~",
            "  #                                              ~",
            " #                                               ~",
            "##################################################"
        };
    }
    else if (s == 4) {
        world = {
            "######                                      ######",
            "#    ######                            ######    ~",
            "#         ##############################         ~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#         ##############################         ~",
            "#    ######                            ######    ~",
            "######                                      ######"
        };
    }
    else { // s == 5
        world = {
            "#################################################~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#                                                ~",
            "#################################################~"
        };
    }
}

// ----------------------------------------------------------
// Convert char world to tile map
// ----------------------------------------------------------
void FloodLevel::initTileMapFromWorld() {
    int mapHeight = (int)world.size();
    if (mapHeight == 0) {
        tileMap.clear();
        machines.clear();
        waterFrontX = 0;
        return;
    }

    // 1) Compute effective width per row (last non-space char)
    std::vector<int> rowWidth(mapHeight, 0);
    int mapWidth = 0;

    for (int y = 0; y < mapHeight; ++y) {
        const std::string &row = world[y];
        int last = -1;
        for (int x = 0; x < (int)row.size(); ++x) {
            if (row[x] != ' ') {
                last = x;
            }
        }
        if (last >= 0) {
            rowWidth[y] = last + 1;
            if (rowWidth[y] > mapWidth) {
                mapWidth = rowWidth[y];
            }
        }
    }

    if (mapWidth == 0) {
        tileMap.clear();
        machines.clear();
        waterFrontX = 0;
        return;
    }

    // 2) Build tileMap using trimmed widths
    tileMap.assign(mapHeight, std::vector<Tile>(mapWidth, TILE_EMPTY));
    machines.clear();

    for (int y = 0; y < mapHeight; ++y) {
        const std::string &row = world[y];
        int effW = rowWidth[y];

        for (int x = 0; x < mapWidth; ++x) {
            char c = ' ';
            if (x < effW && x < (int)row.size()) {
                c = row[x];
            }

            if (c == '#') {
                tileMap[y][x] = TILE_WALL;
            } else if (c == '~') {
                tileMap[y][x] = TILE_WATER;
            } else {
                tileMap[y][x] = TILE_EMPTY;
            }
        }
    }

    // 3) Set the flood front to the rightmost existing water column.
    int rightmostWater = -1;
    for (int x = mapWidth - 1; x >= 0 && rightmostWater == -1; --x) {
        for (int y = 0; y < mapHeight; ++y) {
            if (tileMap[y][x] == TILE_WATER) {
                rightmostWater = x;
                break;
            }
        }
    }

    if (rightmostWater != -1) {
        waterFrontX = rightmostWater;
    } else {
        // fallback: no water in map, start from far right
        waterFrontX = mapWidth - 1;
    }
}

// ----------------------------------------------------------
// Player spawn inside wall '#', reachable
// ----------------------------------------------------------
void FloodLevel::choosePlayerSpawnOnWall() {
    int H = (int)tileMap.size();
    if (H == 0) return;
    int W = (int)tileMap[0].size();

    std::vector<std::pair<int,int>> candidates;

    for (int y = 1; y < H - 1; ++y) {
        for (int x = 1; x < W - 1; ++x) {
            if (tileMap[y][x] != TILE_WALL) continue;

            bool reachable = false;
            const int dx[4] = { 0,  0, -1,  1};
            const int dy[4] = {-1,  1,  0,  0};

            for (int k = 0; k < 4; ++k) {
                int nx = x + dx[k];
                int ny = y + dy[k];
                if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
                if (tileMap[ny][nx] == TILE_EMPTY) {
                    reachable = true;
                    break;
                }
            }

            if (reachable) {
                candidates.push_back({x, y});
            }
        }
    }

    if (!candidates.empty()) {
        auto p = candidates[rand() % candidates.size()];
        playerX = p.first;
        playerY = p.second;
    }
}

// ----------------------------------------------------------
// Machines spawn on FLOOR (empty) tiles, not on walls
// ----------------------------------------------------------
// Machines spawn on FLOOR (empty) tiles, not on walls,
// and ONLY if each machine tile is vertically between two
// walls ('#') in that column.
// ----------------------------------------------------------
void FloodLevel::placeRandomMachines(int count) {
    int H = (int)tileMap.size();
    if (H == 0) return;
    int W = (int)tileMap[0].size();

    if (W < 6 || H < 3) return;

    // Helper: check if (x, y) is between a wall above and a wall below
    auto isBetweenWalls = [&](int x, int y) -> bool {
        int above = -1;
        int below = -1;

        // search upward for the first wall
        for (int yy = y; yy >= 0; --yy) {
            if (tileMap[yy][x] == TILE_WALL) {
                above = yy;
                break;
            }
        }

        // search downward for the first wall
        for (int yy = y; yy < H; ++yy) {
            if (tileMap[yy][x] == TILE_WALL) {
                below = yy;
                break;
            }
        }

        // must have walls both above and below and y strictly between them
        return (above != -1 && below != -1 && above < y && y < below);
    };

    int attempts = 0;
    const int MAX_ATTEMPTS = 2000;

    while ((int)machines.size() < count && attempts < MAX_ATTEMPTS) {
        attempts++;

        int ry = 1 + rand() % (H - 2);           // avoid very top/bottom
        int rx = 1 + rand() % (W - 4 - 1);       // x in [1, W-5]

        bool canPlace = true;

        // 1) Must be three consecutive EMPTY tiles (floor)
        //    and each must be between two walls in its column.
        for (int i = 0; i < 3; ++i) {
            int mx = rx + i;
            if (mx < 0 || mx >= W) { canPlace = false; break; }

            if (tileMap[ry][mx] != TILE_EMPTY) {
                canPlace = false;
                break;
            }

            // Do not place on player
            if (mx == playerX && ry == playerY) {
                canPlace = false;
                break;
            }

            // NEW CONDITION: the tile must be between two walls
            if (!isBetweenWalls(mx, ry)) {
                canPlace = false;
                break;
            }
        }
        if (!canPlace) continue;

        // 2) Do not overlap an existing machine
        for (const auto &m : machines) {
            for (int i = 0; i < 3; ++i) {
                int mx1 = rx + i;
                if (m.y == ry && mx1 >= m.x && mx1 <= m.x + 2) {
                    canPlace = false;
                    break;
                }
            }
            if (!canPlace) break;
        }
        if (!canPlace) continue;

        // 3) Ensure at least one adjacent EMPTY tile so player can reach it
        bool reachable = false;
        for (int i = 0; i < 3 && !reachable; ++i) {
            int mx = rx + i;
            int my = ry;

            const int dx[4] = { 0,  0, -1,  1};
            const int dy[4] = {-1,  1,  0,  0};

            for (int k = 0; k < 4; ++k) {
                int nx = mx + dx[k];
                int ny = my + dy[k];
                if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
                if (tileMap[ny][nx] == TILE_EMPTY) {
                    reachable = true;
                    break;
                }
            }
        }
        if (!reachable) continue;

        // 4) Place machine on floor tiles
        machines.push_back({rx, ry});
        for (int i = 0; i < 3; ++i) {
            tileMap[ry][rx + i] = TILE_MACHINE;
        }
    }
}

// ----------------------------------------------------------
// Helpers
// ----------------------------------------------------------
bool FloodLevel::isWalkable(Tile t) const {
    return t == TILE_EMPTY;
}

int FloodLevel::findMachineForInteraction() const {
    // A machine occupies three tiles horizontally
    for (int i = 0; i < (int)machines.size(); ++i) {
        const Machine &m = machines[i];

        for (int j = 0; j < 3; ++j) {
            int mx = m.x + j;
            int my = m.y;

            int dx = playerX - mx;
            int dy = playerY - my;

            if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
                return i;
            }
        }
    }
    return -1;
}

void FloodLevel::getTerminalSize(int &cols, int &rows) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    cols = w.ws_col;
    rows = w.ws_row;
}

void FloodLevel::clearScreen() {
    cout << "\033[2J\033[H";
}

// non-blocking single key read with timeout
char FloodLevel::getchNonBlocking(double timeoutSeconds) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    struct timeval tv;
    if (timeoutSeconds < 0) timeoutSeconds = 0;
    tv.tv_sec  = (int)timeoutSeconds;
    tv.tv_usec = (int)((timeoutSeconds - tv.tv_sec) * 1e6);

    int rv = select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv);
    if (rv > 0) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n == 1) return c;
    }
    return 0; // no key
}

// ----------------------------------------------------------
// drawing – centered map
// ----------------------------------------------------------
void FloodLevel::draw(bool canInteract) {
    clearScreen();

    int termCols, termRows;
    getTerminalSize(termCols, termRows);

    int mapHeight = (int)tileMap.size();
    if (mapHeight == 0) return;
    int mapWidth  = (int)tileMap[0].size();

    int offsetY = (termRows - mapHeight) / 2;
    int offsetX = (termCols - mapWidth) / 2;
    if (offsetY < 0) offsetY = 0;
    if (offsetX < 0) offsetX = 0;

    // draw map rows
    for (int y = 0; y < mapHeight; ++y) {
        printf("\033[%d;%dH", offsetY + y + 1, offsetX + 1);

        for (int x = 0; x < mapWidth; ++x) {
            if (playerX == x && playerY == y) {
                cout << '@';
                continue;
            }

            Tile t = tileMap[y][x];
            if (t == TILE_WALL) {
                cout << '#';
            } else if (t == TILE_WATER) {
                cout << '~';
            } else if (t == TILE_MACHINE) {
                cout << "[o]";
                x += 2; // skip the next two cells in display
            } else {
                cout << ' ';
            }
        }
    }

    // status / prompt line centered below map
    int uiRow = offsetY + mapHeight + 2;
    if (uiRow >= termRows) uiRow = termRows;
    string prompt;

    if (!statusMessage.empty()) {
        prompt = statusMessage + "  ";
    }

    if (canInteract) {
        prompt += "[MACHINE NEARBY] Press Enter to interact. Move with WASD, Q to quit.";
    } else {
        string secText = "10s";
        if (difficulty == 2)      secText = "8s";
        else if (difficulty == 3) secText = "5s";

        prompt += "Move with WASD, Q to quit. Water fills every " + secText + "...";
    }

    int promptCol = (termCols - (int)prompt.size()) / 2;
    if (promptCol < 1) promptCol = 1;
    printf("\033[%d;%dH%s", uiRow, promptCol, prompt.c_str());

    cout.flush();
}

// ----------------------------------------------------------
// input + logic
// ----------------------------------------------------------
void FloodLevel::handleInput(bool canInteract, bool &gameOver, bool &needRedraw) {
    char cmd = getchNonBlocking(0.1);

    int dx = 0, dy = 0;

    if (cmd != 0) {
        needRedraw = true;
    }

    if (cmd == 'w' || cmd == 'W') dy = -1;
    if (cmd == 's' || cmd == 'S') dy =  1;
    if (cmd == 'a' || cmd == 'A') dx = -1;
    if (cmd == 'd' || cmd == 'D') dx =  1;
    if (cmd == 'q' || cmd == 'Q') {
        gameOver = true; // treat as quitting level
        return;
    }

    bool isInteractKey = (cmd == 'y' || cmd == 'Y' || cmd == '\n' || cmd == '\r');

    if (isInteractKey && canInteract) {
        int midx = findMachineForInteraction();
        if (midx != -1) {
            bool solved = false;
            if (difficulty == 1) {
                solved = PuzzleInterface::solveRandomPuzzle();    // Easy
            } else if (difficulty == 2) {
                solved = MediumPuzzle::solve();                   // Medium
            } else if (difficulty == 3) {
                solved = HardPuzzle::solve();                     // Hard
            }

            if (solved) {
                const Machine m = machines[midx];
                for (int i = 0; i < 3; ++i) {
                    int tx = m.x + i;
                    if (tx >= 0 && tx < (int)tileMap[0].size()) {
                        tileMap[m.y][tx] = TILE_EMPTY;
                    }
                }
                machines.erase(machines.begin() + midx);
                statusMessage = "You repaired a machine!";
                needRedraw = true;
            } else {
                statusMessage = "You did not repair the machine.";
            }
        } else {
            statusMessage = "No machine found nearby.";
        }
    }
    else if (cmd != 0 && !isInteractKey) {
        statusMessage.clear();
    }

    int mapHeight = (int)tileMap.size();
    int mapWidth  = (mapHeight > 0 ? (int)tileMap[0].size() : 0);

    int nx = playerX + dx;
    int ny = playerY + dy;

    if (nx >= 0 && nx < mapWidth && ny >= 0 && ny < mapHeight) {
        Tile nextTile = tileMap[ny][nx];
        if (isWalkable(nextTile)) {
            if (nx != playerX || ny != playerY) {
                playerX = nx;
                playerY = ny;
                needRedraw = true;
            }
        }
    }
}

// ----------------------------------------------------------
// Flood logic: one column at a time, only between walls
// ----------------------------------------------------------
bool FloodLevel::updateFlood(bool &needRedraw) {
    time_t now = time(nullptr);

    // Water speed by difficulty
    double interval = 10.0;
    switch (difficulty) {
        case 1: interval = 10.0; break; // Easy
        case 2: interval = 8.0;  break; // Medium
        case 3: interval = 5.0;  break; // Hard
        default: interval = 10.0; break;
    }

    double elapsed = difftime(now, lastFloodTime);

    int mapHeight = (int)tileMap.size();
    if (mapHeight == 0) return false;
    int mapWidth  = (int)tileMap[0].size();

    bool anyFlooded = false;

    // Allow multiple steps if a lot of time passed
    while (elapsed >= interval && waterFrontX > 0) {
        // Move the front one column to the left
        waterFrontX--;
        if (waterFrontX < 0 || waterFrontX >= mapWidth) break;

        // Find top and bottom wall in this column
        int topWall    = -1;
        int bottomWall = -1;
        for (int y = 0; y < mapHeight; ++y) {
            if (tileMap[y][waterFrontX] == TILE_WALL) {
                if (topWall == -1) topWall = y;
                bottomWall = y;
            }
        }

        bool machineFlooded = false;

        // Only flood tiles strictly between topWall and bottomWall
        if (topWall != -1 && bottomWall != -1 && bottomWall > topWall) {
            for (int y = topWall + 1; y < bottomWall; ++y) {
                if (tileMap[y][waterFrontX] == TILE_MACHINE) {
                    machineFlooded = true;
                }
                if (tileMap[y][waterFrontX] == TILE_EMPTY) {
                    tileMap[y][waterFrontX] = TILE_WATER;
                }
            }
            anyFlooded = true;
        }

        // Advance time for this step
        lastFloodTime += (time_t)interval;
        elapsed = difftime(now, lastFloodTime);

        if (machineFlooded) {
            clearScreen();
            cout << "\n\nThe water has destroyed a critical machine... GAME OVER.\n\n";
            usleep(100000);
            cout << "Press any key to continue...";
            cout.flush();

            char c;
            read(STDIN_FILENO, &c, 1);
            return true; // level over
        }
    }

    if (anyFlooded) {
        needRedraw = true;
    }

    return false;
}

bool FloodLevel::playerInWater() const {
    int mapHeight = (int)tileMap.size();
    if (mapHeight == 0) return false;
    int mapWidth  = (int)tileMap[0].size();

    if (playerY < 0 || playerY >= mapHeight ||
        playerX < 0 || playerX >= mapWidth) return false;

    return (tileMap[playerY][playerX] == TILE_WATER);
}

// ----------------------------------------------------------
// main loop
// ----------------------------------------------------------
bool FloodLevel::run() {
    bool gameOver   = false;
    bool needRedraw = true;

    while (!gameOver) {
        // Win condition: all machines repaired
        if (machines.empty()) {
            clearScreen();
            if (stage < 5) {
                cout << "\n\nWell done!\nAll machines in this room have been repaired!\n";
                int n = 5 - stage;
                if (n == 1) {
                    cout << "But there is still 1 room left to save!\n\n";
                }
                else cout << "But there are still " << n << " rooms left to save!\n\n";
            }

            // Extra message when finishing Stage 5 (final stage)
            else if (stage == 5) {
                cout << "\n\nYOU WIN!\nAll machines have been repaired!\nThe facility is saved!\n\n";
                cout << "==============================================\n";
                cout << "CONGRATULATIONS! You have cleared STAGE 5";
                if (difficulty == 1) {
                    cout << " (EASY)";
                } else if (difficulty == 2) {
                    cout << " (MEDIUM)";
                } else if (difficulty == 3) {
                    cout << " (HARD)";
                }
                cout << "!\n";
                cout << "Thanks for playing the Flooded Facility game!\n";
                cout << "==============================================\n\n";
            }

            cout << "Press Enter to return to the menu...";
            cout.flush();

            while (true) {
                char c = getchNonBlocking(0.5);
                if (c == '\n' || c == '\r') break;
            }
            return true;
        }

        int machineIdx = findMachineForInteraction();
        bool canInteract = (machineIdx != -1);

        if (needRedraw) {
            draw(canInteract);
            needRedraw = false;
        }

        handleInput(canInteract, gameOver, needRedraw);
        if (gameOver) {
            // quit by player
            return false;
        }

        if (updateFlood(needRedraw)) {
            // machine flooded and game over
            return false;
        }

        if (playerInWater()) {
            clearScreen();
            cout << "\n\nThe water has reached you... GAME OVER.\n\n";
            usleep(100000);
            cout << "Press any key to continue...";
            cout.flush();

            char c;
            read(STDIN_FILENO, &c, 1);
            return false;
        }
    }

    return false;
}
