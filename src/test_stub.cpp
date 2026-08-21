// src/test_stub.cpp — provides run_tests() returning gtest exit code.
#include <gtest/gtest.h>

int run_tests() {
    int argc = 0;
    char** argv = nullptr;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
