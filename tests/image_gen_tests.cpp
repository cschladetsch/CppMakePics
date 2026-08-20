// Tests for ImageGenerator — GTest, built to bin/tests/TestRunner.exe
#include "image_gen.hpp"
#include <gtest/gtest.h>

// ---- ImageGenerator tests ----

TEST(ImageGeneratorTest, FilenameConsistent) {
    ImageGenerator gen;
    std::string f1 = gen.image_filename("cat", "42", 512, 512);
    std::string f2 = gen.image_filename("cat", "42", 512, 512);
    EXPECT_EQ(f1, f2);
    EXPECT_TRUE(f1.find("img_") == 0);
    EXPECT_TRUE(f1.size() > 10);
    EXPECT_TRUE(f1.substr(f1.size() - 4) == ".png");
}

TEST(ImageGeneratorTest, FilenameUniquePerPrompt) {
    ImageGenerator gen;
    std::string f1 = gen.image_filename("cat", "42", 512, 512);
    std::string f2 = gen.image_filename("dog", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST(ImageGeneratorTest, FilenameUniquePerSeed) {
    ImageGenerator gen;
    std::string f1 = gen.image_filename("cat", "1", 512, 512);
    std::string f2 = gen.image_filename("cat", "2", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST(ImageGeneratorTest, FilenameUniquePerSize) {
    ImageGenerator gen;
    std::string f1 = gen.image_filename("cat", "42", 256, 256);
    std::string f2 = gen.image_filename("cat", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST(ImageGeneratorTest, ImageFilenameFormat) {
    ImageGenerator gen;
    std::string fn = gen.image_filename("a", "b", 100, 200);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
    size_t dot = fn.rfind('.');
    EXPECT_TRUE(dot != std::string::npos && dot > 4);
    EXPECT_EQ(fn.substr(dot), ".png");
}
