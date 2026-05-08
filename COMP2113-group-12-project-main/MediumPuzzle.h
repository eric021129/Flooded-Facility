#ifndef MEDIUMPuzzle_H
#define MEDIUMPuzzle_H

#include <string>
#include <vector>

class MediumPuzzle {
public:
    static bool solve();

private:
    static std::vector<std::string> codes;
    static std::vector<std::vector<std::string>> options;
    static std::vector<std::vector<int>> answers;
    static std::vector<bool> used;

    static void waitForEnter();
    static void initPuzzles();
};

#endif
