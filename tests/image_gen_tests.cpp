// ===========================================================================
// tests/image_gen_tests.cpp — ImageGenerator tests via GoogleTest
// ===========================================================================
#include "image_gen.hpp"
#include <gtest/gtest.h>

class ImageGeneratorTest : public ::testing::Test {
protected:
    ImageGenerator gen;
};

TEST_F(ImageGeneratorTest, SamePromptSeedSizeProducesSameFilename) {
    auto f1 = gen.image_filename("cat", "42", 512, 512);
    auto f2 = gen.image_filename("cat", "42", 512, 512);
    ASSERT_EQ(f1, f2);
    EXPECT_TRUE(f1.rfind("img_", 0) == 0);
    EXPECT_TRUE(f1.size() > 10);
    EXPECT_EQ(f1.substr(f1.size() - 4), ".png");
}

TEST_F(ImageGeneratorTest, DifferentPromptsProduceDifferentFilenames) {
    auto f1 = gen.image_filename("cat", "42", 512, 512);
    auto f2 = gen.image_filename("dog", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(ImageGeneratorTest, DifferentSeedsProduceDifferentFilenames) {
    auto f1 = gen.image_filename("cat", "1", 512, 512);
    auto f2 = gen.image_filename("cat", "2", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(ImageGeneratorTest, DifferentSizesProduceDifferentFilenames) {
    auto f1 = gen.image_filename("cat", "42", 256, 256);
    auto f2 = gen.image_filename("cat", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(ImageGeneratorTest, FilenameFormat) {
    auto fn = gen.image_filename("a", "b", 100, 200);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
    auto dot = fn.rfind('.');
    EXPECT_TRUE(dot != std::string::npos && dot > 4);
    EXPECT_EQ(fn.substr(dot), ".png");
}
