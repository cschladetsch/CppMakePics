// ===========================================================================
// src/main.cpp — Entry point: runs tests or prints help
// ===========================================================================
#include <cstring>
#include <iostream>

// Forward declaration
int run_tests();

int main(int argc, char* argv[]) {
    bool test_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--test") == 0 ||
            std::strcmp(argv[i], "-t") == 0 ||
            std::strcmp(argv[i], "--run-tests") == 0) {
            test_mode = true;
            break;
        }
    }

    if (test_mode) {
        return run_tests();
    }

    std::cout << "DiffusionApp — use --test to run the test suite.\n";
    return 0;
}
