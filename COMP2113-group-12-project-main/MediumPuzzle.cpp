#include "MediumPuzzle.h"
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <unistd.h>      // sleep
#include <termios.h>
#include <sys/select.h>  // select, fd_set, timeval

using namespace std;

vector<string> MediumPuzzle::codes;
vector<vector<string>> MediumPuzzle::options;
vector<vector<int>> MediumPuzzle::answers;
vector<bool> MediumPuzzle::used;

void MediumPuzzle::waitForEnter() {
    cout << "\nPress any key to continue...\n";
    cin.get();
}

void MediumPuzzle::initPuzzles() {
    if (!codes.empty()) return;

    codes = {
        "sort(v.[BLANK1](), v.[BLANK2](), [BLANK3]);",
        "auto it = find(v.[BLANK1](), v.[BLANK2](), [BLANK3]);",
        "map<string,int> m; m.[BLANK1](\"key\") = [BLANK2]; cout << m.[BLANK3](\"key\");",
        "vector<int> v = {1,2,3}; v.[BLANK1](v.[BLANK2](), [BLANK3]);",
        "string s = \"hello\"; transform(s.[BLANK1](), s.[BLANK2](), s.[BLANK3](), ::toupper);"
    };

    options = {
        {"end", "begin", "greater<int>()"},
        {"999", "end", "begin"},
        {"operator[]", "50", "at"},
        {"insert", "begin", "42"},
        {"begin", "begin", "end"}
    };

    answers = {
        {1, 0, 2},
        {2, 1, 0},
        {0, 1, 2},
        {0, 1, 2},
        {0, 2, 1}
    };

    used.assign(codes.size(), false);
}

bool MediumPuzzle::solve() {
    initPuzzles();

    // choose a not-yet-used puzzle
    vector<int> avail;
    for (int i = 0; i < (int)codes.size(); ++i) {
        if (!used[i]) avail.push_back(i);
    }

    // if all puzzles were used already, reset so we can reuse them
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


    string code = codes[idx];
    auto opts = options[idx];
    auto ans  = answers[idx];

    const int blank_count = 3;
    string filled[blank_count] = {"", "", ""};
    int cur_blank = 0;
    int cur_opt   = 0;

    // raw terminal
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    char key = '\0';
    while (true) {
        system("clear");
        cout << "========================================\n";
        cout << "             MEDIUM MODE\n";
        cout << "========================================\n\n";

        // build display and find current blank position
        string display = code;
        size_t cur_pos = 0;

        for (int i = 0; i < blank_count; ++i) {
            string tag = "[BLANK" + to_string(i + 1) + "]";
            size_t pos = display.find(tag);
            if (pos != string::npos) {
                if (i == cur_blank) {
                    cur_pos = pos;      // arrow goes here
                }
                string content = filled[i].empty() ? tag : filled[i];
                display.replace(pos, tag.size(), content);
            }
        }

        // print code line + arrow under active blank
        cout << "  " << display << "\n  ";
        for (size_t i = 0; i < cur_pos; ++i) {
            cout << ' ';
        }
        cout << "^^\n";

        // options
        cout << "Options:\n";
        for (int i = 0; i < blank_count; ++i) {
            cout << "   " << (i == cur_opt ? ">> " : "   ")
                 << (i + 1) << ". " << opts[i] << "\n";
        }

        cout << "\nW/S = Move Option   A/D = Move Blank   Space/Enter = Insert   R = Check   Q = Quit\n";

        // non-blocking read
        struct timeval tv = {0, 100000};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(1, &fds, NULL, NULL, &tv) > 0) {
            key = getchar();
        } else {
            key = '\0';
        }

        if (key == 'q' || key == 'Q') {
            break;
        }

        if (key == 'r' || key == 'R') {
            bool ok = true;
            for (int i = 0; i < blank_count; ++i) {
                if (filled[i] != opts[ans[i]]) {
                    ok = false;
                    break;
                }
            }
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

        // move between blanks
        if (key == 'a' || key == 'A') cur_blank = (cur_blank + blank_count - 1) % blank_count;
        if (key == 'd' || key == 'D') cur_blank = (cur_blank + 1) % blank_count;

        // move between options
        if (key == 'w' || key == 'W') cur_opt = (cur_opt + blank_count - 1) % blank_count;
        if (key == 's' || key == 'S') cur_opt = (cur_opt + 1) % blank_count;

        // insert option into current blank
        if (key == ' ' || key == '\n' || key == '\r') {
            filled[cur_blank] = opts[cur_opt];
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    waitForEnter();
    return false;
}
