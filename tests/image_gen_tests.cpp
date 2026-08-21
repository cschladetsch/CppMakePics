// ===========================================================================
// tests/image_gen_tests.cpp — ImageGenerator tests via GoogleTest (50+ cases)
// ===========================================================================
#include "image_gen.hpp"
#include <gtest/gtest.h>
#include <string>

// ===========================================================================
// Filename consistency / uniqueness (replaces original 5 tests, expanded)
// ===========================================================================
class FilenameTest : public ::testing::Test {
protected:
    ImageGenerator gen;
};

TEST_F(FilenameTest, SamePromptSeedSizeProducesSameFilename) {
    auto f1 = gen.image_filename("cat", "42", 512, 512);
    auto f2 = gen.image_filename("cat", "42", 512, 512);
    ASSERT_EQ(f1, f2);
}

TEST_F(FilenameTest, SameHashTwiceDeterministic) {
    auto f1 = gen.image_filename("hello", "1", 100, 200);
    auto f2 = gen.image_filename("hello", "1", 100, 200);
    ASSERT_EQ(f1, f2);
    ASSERT_EQ(f1, gen.image_filename("hello", "1", 100, 200));
}

TEST_F(FilenameTest, DifferentPromptsProduceDifferentFilenames) {
    auto f1 = gen.image_filename("cat", "42", 512, 512);
    auto f2 = gen.image_filename("dog", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(FilenameTest, DifferentSeedsProduceDifferentFilenames) {
    auto f1 = gen.image_filename("cat", "1", 512, 512);
    auto f2 = gen.image_filename("cat", "2", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(FilenameTest, DifferentSizesProduceDifferentFilenames) {
    auto f1 = gen.image_filename("cat", "42", 256, 256);
    auto f2 = gen.image_filename("cat", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(FilenameTest, FilenameStartsWithImg) {
    auto fn = gen.image_filename("a", "b", 100, 200);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
}

TEST_F(FilenameTest, FilenameEndsWithPng) {
    auto fn = gen.image_filename("x", "y", 1, 1);
    EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
}

TEST_F(FilenameTest, FilenameLongEnough) {
    auto fn = gen.image_filename("test", "seed", 32, 32);
    EXPECT_GT(fn.size(), 10u);
}

TEST_F(FilenameTest, FilenameContainsHashDigits) {
    auto fn = gen.image_filename("z", "1", 50, 50);
    EXPECT_TRUE(fn.find("img_") == 0);
    EXPECT_GT(fn.size(), 5u);
}

TEST_F(FilenameTest, EmptyPromptStillProducesFilename) {
    auto fn = gen.image_filename("", "s", 10, 10);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
    EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
}

TEST_F(FilenameTest, EmptySeedStillProducesFilename) {
    auto fn = gen.image_filename("p", "", 10, 10);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
}

TEST_F(FilenameTest, LargeDimensionsProduceValidFilename) {
    auto fn = gen.image_filename("big", "1", 5000, 5000);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
    EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
}

TEST_F(FilenameTest, OneByOneProducesFilename) {
    auto fn = gen.image_filename("single", "s", 1, 1);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
}

TEST_F(FilenameTest, UnicodePromptProducesFilename) {
    auto fn = gen.image_filename("\u00e9\u00e0\u00fc", "1", 50, 50);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
    EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
}

TEST_F(FilenameTest, VeryLongPromptStillProducesFilename) {
    std::string long_prompt(1000, 'A');
    auto fn = gen.image_filename(long_prompt, "s", 80, 80);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
    EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
}

// ===========================================================================
// valid_dimensions
// ===========================================================================
class ValidDimensionsTest : public ::testing::Test {};

TEST(ValidDimensionsTest, EqualValidPair) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(500, 500));
}

TEST(ValidDimensionsTest, Typical1) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(1, 1));
}

TEST(ValidDimensionsTest, Typical2) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(10000, 10000));
}

TEST(ValidDimensionsTest, RectangularValid) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(800, 600));
}

TEST(ValidDimensionsTest, MinMin) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(1, 1));
}

TEST(ValidDimensionsTest, MaxMax) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(10000, 10000));
}

TEST(ValidDimensionsTest, MixedWithinRange) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(100, 9999));
    EXPECT_TRUE(ImageGenerator::valid_dimensions(9999, 100));
}

TEST(ValidDimensionsTest, ZeroWidthInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(0, 500));
}

TEST(ValidDimensionsTest, ZeroHeightInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(500, 0));
}

TEST(ValidDimensionsTest, BothZeroInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(0, 0));
}

TEST(ValidDimensionsTest, NegativeWidthInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(-1, 500));
}

TEST(ValidDimensionsTest, NegativeHeightInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(500, -1));
}

TEST(ValidDimensionsTest, BothNegativeInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(-10, -10));
}

TEST(ValidDimensionsTest, WidthTooLargeInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(10001, 500));
}

TEST(ValidDimensionsTest, HeightTooLargeInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(500, 10001));
}

TEST(ValidDimensionsTest, BothTooLargeInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(10001, 10001));
}

TEST(ValidDimensionsTest, MinMinusOneInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(0, 1));
}

TEST(ValidDimensionsTest, MaxPlusOneInvalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(10001, 10000));
}

TEST(ValidDimensionsTest, WideButValid) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(10000, 1));
    EXPECT_TRUE(ImageGenerator::valid_dimensions(1, 10000));
}

// ===========================================================================
// clamp_dimension
// ===========================================================================
class ClampDimensionTest : public ::testing::Test {
protected:
    static int clamp(int v) { return ImageGenerator::clamp_dimension(v, 1, 10000); }
};

TEST_F(ClampDimensionTest, InRangeUnchanged) {
    EXPECT_EQ(clamp(500), 500);
    EXPECT_EQ(clamp(1), 1);
    EXPECT_EQ(clamp(10000), 10000);
    EXPECT_EQ(clamp(9999), 9999);
}

TEST_F(ClampDimensionTest, BelowMinClampsToMin) {
    EXPECT_EQ(clamp(0), 1);
    EXPECT_EQ(clamp(-1), 1);
    EXPECT_EQ(clamp(-100), 1);
    EXPECT_EQ(clamp(-99999), 1);
}

TEST_F(ClampDimensionTest, AboveMaxClampsToMax) {
    EXPECT_EQ(clamp(10001), 10000);
    EXPECT_EQ(clamp(50000), 10000);
    EXPECT_EQ(clamp(999999999), 10000);
}

TEST_F(ClampDimensionTest, MixedValuesClampCorrectly) {
    EXPECT_EQ(clamp(-10), 1);
    EXPECT_EQ(clamp(0), 1);
    EXPECT_EQ(clamp(1), 1);
    EXPECT_EQ(clamp(50), 50);
    EXPECT_EQ(clamp(9999), 9999);
    EXPECT_EQ(clamp(10000), 10000);
    EXPECT_EQ(clamp(10001), 10000);
    EXPECT_EQ(clamp(100000), 10000);
}

// ===========================================================================
// clamp_width / clamp_height
// ===========================================================================
class ClampWidthHeightTest : public ::testing::Test {};

TEST_F(ClampWidthHeightTest, ClampWidthInRange) {
    EXPECT_EQ(ImageGenerator::clamp_width(100), 100);
    EXPECT_EQ(ImageGenerator::clamp_width(1), 1);
    EXPECT_EQ(ImageGenerator::clamp_width(10000), 10000);
}

TEST_F(ClampWidthHeightTest, ClampWidthBelowMin) {
    EXPECT_EQ(ImageGenerator::clamp_width(0), 1);
    EXPECT_EQ(ImageGenerator::clamp_width(-5), 1);
}

TEST_F(ClampWidthHeightTest, ClampWidthAboveMax) {
    EXPECT_EQ(ImageGenerator::clamp_width(10001), 10000);
    EXPECT_EQ(ImageGenerator::clamp_width(99999), 10000);
}

TEST_F(ClampWidthHeightTest, ClampHeightInRange) {
    EXPECT_EQ(ImageGenerator::clamp_height(200), 200);
    EXPECT_EQ(ImageGenerator::clamp_height(1), 1);
    EXPECT_EQ(ImageGenerator::clamp_height(10000), 10000);
}

TEST_F(ClampWidthHeightTest, ClampHeightBelowMin) {
    EXPECT_EQ(ImageGenerator::clamp_height(0), 1);
    EXPECT_EQ(ImageGenerator::clamp_height(-10), 1);
}

TEST_F(ClampWidthHeightTest, ClampHeightAboveMax) {
    EXPECT_EQ(ImageGenerator::clamp_height(20000), 10000);
    EXPECT_EQ(ImageGenerator::clamp_height(500000), 10000);
}

TEST_F(ClampWidthHeightTest, ClampWidthWidthZeroBecomesOne) {
    EXPECT_EQ(ImageGenerator::clamp_width(0), ImageGenerator::MIN_DIM);
}

TEST_F(ClampWidthHeightTest, ClampHeightHeightZeroBecomesOne) {
    EXPECT_EQ(ImageGenerator::clamp_height(0), ImageGenerator::MIN_DIM);
}

TEST_F(ClampWidthHeightTest, ClampWidthMaxPlusOneBecomesMax) {
    EXPECT_EQ(ImageGenerator::clamp_width(ImageGenerator::MAX_DIM + 1),
              ImageGenerator::MAX_DIM);
}

TEST_F(ClampWidthHeightTest, ClampHeightMaxPlusOneBecomesMax) {
    EXPECT_EQ(ImageGenerator::clamp_height(ImageGenerator::MAX_DIM + 1),
              ImageGenerator::MAX_DIM);
}

// ===========================================================================
// sanitize_prompt
// ===========================================================================
class SanitizePromptTest : public ::testing::Test {};

TEST_F(SanitizePromptTest, EmptyStringStaysEmpty) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt(""), "");
}

TEST_F(SanitizePromptTest, WhitespaceOnlyBecomesEmpty) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt(" "), "");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("   "), "");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("     "), "");
}

TEST_F(SanitizePromptTest, SingleWordUnchanged) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("hello"), "hello");
}

TEST_F(SanitizePromptTest, LeadingSpacesTrimmed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  hello"), "hello");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("   hello"), "hello");
}

TEST_F(SanitizePromptTest, TrailingSpacesTrimmed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("hello  "), "hello");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("hello   "), "hello");
}

TEST_F(SanitizePromptTest, BothLeadingAndTrailingTrimmed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  hello  "), "hello");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("   hello   "), "hello");
}

TEST_F(SanitizePromptTest, InternalSpacesPreserved) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("hello world"), "hello world");
}

TEST_F(SanitizePromptTest, MultipleInternalSpacesCollapsed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("hello   world"), "hello world");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a    b"), "a b");
}

TEST_F(SanitizePromptTest, MixedWhitespaceCollapsed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a \t b"), "a b");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a\n b"), "a b");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a\r\n b"), "a b");
}

TEST_F(SanitizePromptTest, WhitespaceOnlyBecomesEmptyAfterFullTrim) {
    // All-whitespace -> empty after trimming both ends
    EXPECT_EQ(ImageGenerator::sanitize_prompt("    "), "");
}

TEST_F(SanitizePromptTest, SingleSpaceIsEmptyAfterTrim) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt(" "), "");
}

TEST_F(SanitizePromptTest, TabOnlyIsEmptyAfterTrim) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\t"), "");
}

TEST_F(SanitizePromptTest, NewlineOnlyIsEmptyAfterTrim) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\n"), "");
}

TEST_F(SanitizePromptTest, LeadingTabsTrimmed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\t\thello"), "hello");
}

TEST_F(SanitizePromptTest, TrailingTabsTrimmed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("hello\t\t"), "hello");
}

TEST_F(SanitizePromptTest, NewlineTrimmed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\nhello\n"), "hello");
}

TEST_F(SanitizePromptTest, CarriageReturnTrimmed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\rhello\r"), "hello");
}

TEST_F(SanitizePromptTest, ComplexWhitespaceCollapse) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt(" a \t\n b "), "a b");
}

TEST_F(SanitizePromptTest, MultipleWordsWithCollapse) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("one   two    three"),
              "one two three");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  one   two    three  "),
              "one two three");
}

TEST_F(SanitizePromptTest, SpecialCharsPreserved) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a!b@c#"), "a!b@c#");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("100%"), "100%");
}

TEST_F(SanitizePromptTest, UnicodePreserved) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\u00e9\u00e0"), "\u00e9\u00e0");
}

TEST_F(SanitizePromptTest, AlreadyCleanUnchanged) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("hello world"), "hello world");
}

TEST_F(SanitizePromptTest, LongStringWithWhitespace) {
    std::string in(500, 'x');
    in += "  ";
    std::string out = ImageGenerator::sanitize_prompt(in);
    EXPECT_EQ(out.size(), 500u);
    EXPECT_EQ(out, std::string(500, 'x'));
}

// ===========================================================================
// default_seed
// ===========================================================================
class DefaultSeedTest : public ::testing::Test {};

TEST_F(DefaultSeedTest, ReturnsZeroString) {
    EXPECT_EQ(ImageGenerator::default_seed(), "0");
}

TEST_F(DefaultSeedTest, ReturnsNonEmpty) {
    EXPECT_FALSE(ImageGenerator::default_seed().empty());
}

// ===========================================================================
// build_full_path
// ===========================================================================
class BuildFullPathTest : public ::testing::Test {};

TEST_F(BuildFullPathTest, EmptyDirReturnsFilenameOnly) {
    EXPECT_EQ(ImageGenerator::build_full_path("", "img_123.png"), "img_123.png");
}

TEST_F(BuildFullPathTest, DirWithoutSepAppendsSlash) {
    EXPECT_EQ(ImageGenerator::build_full_path("out", "img_123.png"),
              "out/img_123.png");
}

TEST_F(BuildFullPathTest, DirWithTrailingSlashNoDoubleSlash) {
    EXPECT_EQ(ImageGenerator::build_full_path("out/", "img_123.png"),
              "out/img_123.png");
}

TEST_F(BuildFullPathTest, DirWithTrailingBackslashNoDouble) {
    EXPECT_EQ(ImageGenerator::build_full_path("out\\", "img_123.png"),
              "out\\img_123.png");
}

TEST_F(BuildFullPathTest, NestedDir) {
    EXPECT_EQ(ImageGenerator::build_full_path("a/b", "img.png"),
              "a/b/img.png");
}

TEST_F(BuildFullPathTest, NestedDirWithSlash) {
    EXPECT_EQ(ImageGenerator::build_full_path("a/b/", "img.png"),
              "a/b/img.png");
}

TEST_F(BuildFullPathTest, FilenameOnlyNoDir) {
    EXPECT_EQ(ImageGenerator::build_full_path("", "f.png"), "f.png");
}

TEST_F(BuildFullPathTest, RootLikeDir) {
    EXPECT_EQ(ImageGenerator::build_full_path("/", "f.png"), "/f.png");
}

TEST_F(BuildFullPathTest, RootLikeDirWithSlash) {
    EXPECT_EQ(ImageGenerator::build_full_path("//", "f.png"), "//f.png");
}

// ===========================================================================
// Cross-method interaction: sanitize + filename determinism
// ===========================================================================
class InteractionTest : public ::testing::Test {
protected:
    ImageGenerator gen;
};

TEST_F(InteractionTest, SanitizedPromptSameHashForEquivalentInputs) {
    std::string a = "hello world";
    std::string b = "  hello   world  ";
    auto fn1 = gen.image_filename(ImageGenerator::sanitize_prompt(a), "1", 100, 100);
    auto fn2 = gen.image_filename(ImageGenerator::sanitize_prompt(b), "1", 100, 100);
    EXPECT_EQ(fn1, fn2);
}

TEST_F(InteractionTest, DifferentPromptsAfterSanitizeStillDifferent) {
    auto fn1 = gen.image_filename(ImageGenerator::sanitize_prompt("cat"), "1", 100, 100);
    auto fn2 = gen.image_filename(ImageGenerator::sanitize_prompt("dog"), "1", 100, 100);
    EXPECT_NE(fn1, fn2);
}
