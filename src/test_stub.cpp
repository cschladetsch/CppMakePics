// src/test_stub.cpp — provides run_tests() so main.cpp compiles standalone.
// Real tests: bin/tests/TestRunner.exe (GTest, built from tests/CMakeLists.txt).
#include <iostream>

void run_tests() {
    std::cout << "[Tests] Stub: run `bin/tests/TestRunner.exe` for full GTest suite.\n";
}
