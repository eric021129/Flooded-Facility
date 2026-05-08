#ifndef PUZZLEINTERFACE_H
#define PUZZLEINTERFACE_H

#include <string>
#include <vector>

class PuzzleInterface {
public:
    static bool solveRandomPuzzle();

private:
    static std::vector<std::string> codes;
    static std::vector<std::vector<std::string>> all_options;
    static std::vector<std::vector<int>> answers;
    static std::vector<bool> used;
    static void waitForEnter();
};

#endif

