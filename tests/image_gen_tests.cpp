// ===========================================================================
// tests/image_gen_tests.cpp — ImageGenerator tests (no GTest dependency)
// ===========================================================================
#include "image_gen.hpp"
#include <cassert>
#include <iostream>
#include <string>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { ++tests_passed; } \
    else { ++tests_failed; std::cerr << "FAIL: " << msg << "\n"; } \
} while(0)

#define CHECK_EQ(a, b, msg) CHECK((a) == (b), msg)
#define CHECK_NE(a, b, msg) CHECK((a) != (b), msg)
#define CHECK_TRUE(cond, msg) CHECK(cond, msg)
#define CHECK_STREQ(a, b, msg) CHECK((a) == (b), msg)

// ---- Filename consistency ----
void test_filename_consistent() {
    ImageGenerator gen;
    std::string f1 = gen.image_filename("cat", "42", 512, 512);
    std::string f2 = gen.image_filename("cat", "42", 512, 512);
    CHECK_EQ(f1, f2, "Same prompt/seed/size should produce same filename");
    CHECK_TRUE(f1.find("img_") == 0, "Filename should start with img_");
    CHECK_TRUE(f1.size() > 10, "Filename should be >10 chars");
    CHECK_TRUE(f1.substr(f1.size() - 4) == ".png", "Filename should end with .png");
}

// ---- Filename unique per prompt ----
void test_filename_unique_per_prompt() {
    ImageGenerator gen;
    std::string f1 = gen.image_filename("cat", "42", 512, 512);
    std::string f2 = gen.image_filename("dog", "42", 512, 512);
    CHECK_NE(f1, f2, "Different prompts should produce different filenames");
}

// ---- Filename unique per seed ----
void test_filename_unique_per_seed() {
    ImageGenerator gen;
    std::string f1 = gen.image_filename("cat", "1", 512, 512);
    std::string f2 = gen.image_filename("cat", "2", 512, 512);
    CHECK_NE(f1, f2, "Different seeds should produce different filenames");
}

// ---- Filename unique per size ----
void test_filename_unique_per_size() {
    ImageGenerator gen;
    std::string f1 = gen.image_filename("cat", "42", 256, 256);
    std::string f2 = gen.image_filename("cat", "42", 512, 512);
    CHECK_NE(f1, f2, "Different sizes should produce different filenames");
}

// ---- Image filename format ----
void test_filename_format() {
    ImageGenerator gen;
    std::string fn = gen.image_filename("a", "b", 100, 200);
    CHECK_TRUE(fn.rfind("img_", 0) == 0, "Should start with img_");
    size_t dot = fn.rfind('.');
    CHECK_TRUE(dot != std::string::npos && dot > 4, "Should have extension after position 4");
    CHECK_STREQ(fn.substr(dot), ".png", "Extension should be .png");
}

int main() {
    std::cout << "[Tests] Running ImageGenerator tests...\n";
    test_filename_consistent();
    test_filename_unique_per_prompt();
    test_filename_unique_per_seed();
    test_filename_unique_per_size();
    test_filename_format();
    std::cout << "[Tests] " << tests_passed << " passed, " << tests_failed << " failed\n";
    return tests_failed > 0 ? 1 : 0;
}
