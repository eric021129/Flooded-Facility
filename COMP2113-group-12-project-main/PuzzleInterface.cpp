#include "PuzzleInterface.h"
#include <iostream>
#include <random>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <limits>

using namespace std;

vector<string>               PuzzleInterface::codes;
vector<vector<string>>       PuzzleInterface::all_options;
vector<vector<int>>          PuzzleInterface::answers;
vector<bool>                 PuzzleInterface::used;

// -----------------------------------------------------------
// Wait for Enter after puzzle result
// -----------------------------------------------------------
void PuzzleInterface::waitForEnter() {
    cout << "\nPress any key to return to the map...\n";
    cin.get();
}

// -----------------------------------------------------------
// Solve a random EASY puzzle
// -----------------------------------------------------------
bool PuzzleInterface::solveRandomPuzzle() {
    // -------------------------------
    // 1. Initialize puzzle data once
    // -------------------------------
    if (codes.empty()) {
        codes = {
            // 0: for-loop
            "for (int i = 0; i < [BLANK1]; i += [BLANK2]) sum += v[i];",

            // 1: vector reserve / push_back (FIXED: add [BLANK1], [BLANK2])
            "vector<int> v; v.[BLANK1]; v.[BLANK2];",

            // 2: string push_back / size
            "string s = \"Hi\"; s.[BLANK1]('!'); cout << s.[BLANK2]();",

            // 3: if statement
            "if (score [BLANK1] 90) cout << [BLANK2];",

            // 4: new int, then assign
            "int* p = [BLANK1]; *p = [BLANK2];"
        };

        all_options = {
            {"1", "v.size()"},          // for-loop
            {"reserve", "push_back"},   // vector reserve/push_back
            {"push_back", "size"},      // string push_back/size
            {">=", "\"A\""},            // if (score >= 90) cout << "A";
            {"new int", "42"}           // int* p = new int; *p = 42;
        };

        // answers[i][0] -> index for BLANK1, answers[i][1] -> BLANK2
        answers = {
            {1, 0}, // 0: v.size(), 1
            {0, 1}, // 1: reserve, push_back
            {0, 1}, // 2: push_back, size
            {0, 1}, // 3: >=, "A"
            {0, 1}  // 4: new int, 42
        };

        used.assign(codes.size(), false);
    }

    // choose a not-yet-used puzzle  
    vector<int> avail;
    for (int i = 0; i < (int)codes.size(); ++i) {
        if (!used[i]) avail.push_back(i);
    }

    // if every puzzle has been used, reset and allow reuse
    if (avail.empty()) {
        used.assign(codes.size(), false);
        for (int i = 0; i < (int)codes.size(); ++i) {
            avail.push_back(i);
        }
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, (int)avail.size() - 1);
    int idx = avail[dis(gen)];
    used[idx] = true;

    string code      = codes[idx];
    auto   options   = all_options[idx];
    auto   correct   = answers[idx];

    string filled[2] = {"", ""};  // BLANK1, BLANK2
    int    cur_blank = 0;         // which blank is active (0 or 1)
    int    cur_opt   = 0;         // which option is highlighted (0 or 1)

     struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    char key = '\0';

    while (true) {
        system("clear");
        cout << "========================================\n";
        cout << "               EASY MODE\n";
        cout << "========================================\n\n";

        string display = code;
        size_t cur_pos = 0;
        for (int i = 0; i < 2; ++i) {
            string tag = "[BLANK" + to_string(i+1) + "]";
            size_t pos = display.find(tag);
            if (pos != string::npos) {
                string content = filled[i].empty() ? tag : filled[i];
                if (i == cur_blank) cur_pos = display.find(tag);
                display.replace(pos, tag.size(), content);
            }
        }
        cout << "  " << display << "\n  ";
        for (int i = 0; i < cur_pos; i++) {
            cout << " ";
        }
        cout << "^^\n";

        cout << "Options:\n";
        for (int i = 0; i < 2; ++i)
            cout << "   " << (i == cur_opt ? ">> " : "   ") << (i+1) << ". " << options[i] << "\n";

        cout << "\nW/S = Move Option   A/D = Move Blank   Enter = Insert   R = Check   Q = Quit\n";

        struct timeval tv = {0, 100000};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(1, &fds, NULL, NULL, &tv) > 0) {
            key = getchar();
        } else {
            key = '\0';
        }

        // handle commands
        if (key == 'q' || key == 'Q') {
            // Quit puzzle -> not solved
            tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
            waitForEnter();
            return false;
        }

        if (key == 'r' || key == 'R') {
            bool ok = (filled[0] == options[correct[0]] &&
                       filled[1] == options[correct[1]]);
            if (ok) {
                system("clear");
                cout << "\n\n\nCORRECT!!! Machine repaired!\n";
                tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
                waitForEnter();
                return true;
            } else {
                cout << "\nNot yet... Keep trying!\n";
                sleep(1);
            }
        }

        // Move between blanks (only 2, so they just toggle)
        if (key == 'a' || key == 'A') {
            cur_blank = (cur_blank + 1) % 2;
        }
        if (key == 'd' || key == 'D') {
            cur_blank = (cur_blank + 1) % 2;
        }

        // Move between options
        if (key == 'w' || key == 'W') {
            cur_opt = (cur_opt + 1) % 2;
        }
        if (key == 's' || key == 'S') {
            cur_opt = (cur_opt + 1) % 2;
        }

        // Insert selected option into current blank
        if (key == ' ' || key == '\n') {
            filled[cur_blank] = options[cur_opt];
        }
    }
  
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    waitForEnter();
    return false;
}
