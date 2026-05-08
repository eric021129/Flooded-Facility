#include "PuzzleInterface.h"
#include <iostream>
#include <string>

int main() {
    std::cout << "=== C++ Machine Repair Game ===\n\n";
    while (true) {
        std::cout << "Approach broken machine? (Y/N): ";
        std::string c;
        std::getline(std::cin, c);
        if (c != "Y" && c != "y") break;
        PuzzleInterface::solveRandomPuzzle();
    }
    std::cout << "\nGame over! Thanks for playing!\n";
    return 0;
}

