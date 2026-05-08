#include "HardPuzzle.h"
#include <iostream>
#include <random>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <thread>
#include <mutex>
#include <memory>
#include <future>

using namespace std;

vector<string> HardPuzzle::codes;
vector<vector<string>> HardPuzzle::options;
vector<vector<int>> HardPuzzle::answers;
vector<bool> HardPuzzle::used;

void HardPuzzle::waitForEnter() {
    cout << "\nPress any key to return to menu...\n";
    cin.get();
}

void HardPuzzle::initPuzzles() {
    if (!codes.empty()) return;

    // 0–4: puzzle templates
    codes = {
        // 0: lambda + sort
        "auto lambda = [BLANK1](int x) { return x [BLANK2] [BLANK3]; }; "
        "sort(v.begin(), v.end(), [BLANK4]);",

        // 1: thread with lambda and join
        "thread t([BLANK1] { [BLANK2]; }); t.[BLANK3](); cout << [BLANK4];",

        // 2: unique_ptr
        "unique_ptr<int> p([BLANK1] int(42)); cout << *[BLANK2] << endl; p.[BLANK3]();",

        // 3: promise / future / thread
        "promise<int> pr; future<int> ft = pr.[BLANK1](); "
        "thread t([&pr]{ pr.[BLANK2](42); });"
        "ft.[BLANK3](); t.[BLANK4]();",

        // 4: mutex + shared_ptr
        "mutex mtx; lock_guard<mutex> lock([BLANK1]); "
        "shared_ptr<int> sp([BLANK2] int(10)); "
        "cout << *sp << [BLANK3]; mtx.[BLANK4]();"
    };

    // Options for each puzzle
    options = {
        // 0
        {"0", ">=", "&", "lambda"},
        // 1
        {"&", "sleep(1)", "join", "\"done\""},
        // 2
        {"reset", "p", "new"},
        // 3
        {"get_future", "get", "set_value", "join"},
        // 4
        {"mtx", "new", "endl", "unlock"}
    };

    // answers[i][k] = index into options[i] for BLANK(k+1)
    answers = {
        {2, 1, 0, 3},  // 0: &, >=, 0, lambda
        {0, 1, 2, 3},  // 1: &, sleep(1), join, "done"
        {2, 1, 0},     // 2: new, p, reset
        {0, 2, 1, 3},  // 3: get_future, set_value, get, join
        {0, 1, 2, 3}   // 4: mtx, new, endl, unlock
    };

    used.assign(codes.size(), false);
}

bool HardPuzzle::solve() {
    initPuzzles();

    // choose a not-yet-used puzzle
    vector<int> avail;
    for (int i = 0; i < (int)codes.size(); ++i) {
        if (!used[i]) avail.push_back(i);
    }

    // if all used, reset and reuse
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
    auto ans = answers[idx];

    string filled[4] = {"", "", "", ""};
    int blank_count = (idx == 2) ? 3 : 4;  

    int    cur_blank = 0;
    int    cur_opt   = 0;

    // put terminal into raw mode
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    char key = '\0';
    while (true) {
        system("clear");
        cout << "================================================\n";
        cout << "           HARD MODE - " << blank_count << " BLANKS\n";
        cout << "================================================\n\n";

        // build display string and find position of current blank
        string display = code;
        size_t cur_pos = 0;

        for (int i = 0; i < blank_count; ++i) {
            string tag = "[BLANK" + to_string(i + 1) + "]";
            size_t pos = display.find(tag);
            if (pos != string::npos) {
                if (i == cur_blank) {
                    cur_pos = pos;  // arrow under this position
                }
                string content = filled[i].empty() ? tag : filled[i];
                display.replace(pos, tag.size(), content);
            }
        }

        // print code + cursor arrow line
        cout << "  " << display << "\n  ";
        for (size_t i = 0; i < cur_pos; ++i) cout << ' ';
        cout << "^^\n";

        // options
        cout << "Options:\n";
        for (int i = 0; i < (int)opts.size(); ++i) {
            cout << "   " << (i == cur_opt ? ">> " : "   ")
                 << (i + 1) << ". " << opts[i] << "\n";
        }

        cout << "\nW/S = Move Option   A/D = Move Blank   "
             << "Space/Enter = Insert   R = Check   Q = Quit\n";

        // non-blocking key read
        struct timeval tv = {0, 80000};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(1, &fds, NULL, NULL, &tv) > 0) {
            key = getchar();
        } else {
            key = '\0';
        }

        if (key == 'q' || key == 'Q') break;

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
                cout << "\n\n\nINCREDIBLE!!! Machine repaired!\n";
                tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
                waitForEnter();
                return true;
            } else {
                cout << "\nClose... but not yet.\n";
                sleep(1);
            }
        }

        // move between blanks
        if (key == 'a' || key == 'A')
            cur_blank = (cur_blank + blank_count - 1) % blank_count;
        if (key == 'd' || key == 'D')
            cur_blank = (cur_blank + 1) % blank_count;

        // move between options
        if (key == 'w' || key == 'W')
            cur_opt = (cur_opt + (int)opts.size() - 1) % (int)opts.size();
        if (key == 's' || key == 'S')
            cur_opt = (cur_opt + 1) % (int)opts.size();

        // insert selected option into current blank
        if (key == ' ' || key == '\n' || key == '\r') {
            filled[cur_blank] = opts[cur_opt];
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    waitForEnter();
    return false;
}
