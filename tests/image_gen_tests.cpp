// ===========================================================================
// tests/image_gen_tests.cpp — ImageGenerator tests via GoogleTest (50+)
// Filename format: img_<32 hex chars from SHA-256>.png
// ===========================================================================
#include "image_gen.hpp"
#include <gtest/gtest.h>

// ===========================================================================
// Helper
// ===========================================================================

// ===========================================================================
// Filename determinism / uniqueness
// ===========================================================================
class FilenameTest : public ::testing::Test {
protected:
    ImageGenerator gen;
};

TEST_F(FilenameTest, SameInputsSameOutput) {
    auto f1 = gen.image_filename("cat", "42", 512, 512);
    auto f2 = gen.image_filename("cat", "42", 512, 512);
    ASSERT_EQ(f1, f2);
}

TEST_F(FilenameTest, DifferentPromptsDistinguish) {
    auto f1 = gen.image_filename("cat", "42", 512, 512);
    auto f2 = gen.image_filename("dog", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(FilenameTest, DifferentSeedsDistinguish) {
    auto f1 = gen.image_filename("cat", "1", 512, 512);
    auto f2 = gen.image_filename("cat", "2", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(FilenameTest, DifferentWidthsDistinguish) {
    auto f1 = gen.image_filename("cat", "42", 256, 512);
    auto f2 = gen.image_filename("cat", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(FilenameTest, DifferentHeightsDistinguish) {
    auto f1 = gen.image_filename("cat", "42", 512, 256);
    auto f2 = gen.image_filename("cat", "42", 512, 512);
    EXPECT_NE(f1, f2);
}

TEST_F(FilenameTest, StartsWithImgUnderscore) {
    auto fn = gen.image_filename("a", "b", 100, 200);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
}

TEST_F(FilenameTest, EndsWithPng) {
    auto fn = gen.image_filename("x", "y", 1, 1);
    EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
}

TEST_F(FilenameTest, Has32HexCharsInMiddle) {
    auto fn = gen.image_filename("a", "b", 100, 200);
    // img_ + 32 hex + .png = 4 + 32 + 4 = 40
    EXPECT_EQ(fn.size(), 40u);
    auto hex = fn.substr(4, 32);
    EXPECT_EQ(hex.size(), 32u);
    for (char c : hex) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

TEST_F(FilenameTest, EmptyPromptStillWorks) {
    auto fn = gen.image_filename("", "s", 100, 100);
    EXPECT_EQ(fn.size(), 40u);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
    EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
}

TEST_F(FilenameTest, EmptySeedStillWorks) {
    auto fn = gen.image_filename("p", "", 100, 100);
    EXPECT_EQ(fn.size(), 40u);
}

TEST_F(FilenameTest, BothEmptyStillWorks) {
    auto fn = gen.image_filename("", "", 100, 100);
    EXPECT_EQ(fn.size(), 40u);
}

TEST_F(FilenameTest, ZeroDimensionsStillWorks) {
    auto fn = gen.image_filename("z", "0", 0, 0);
    EXPECT_EQ(fn.size(), 40u);
}

TEST_F(FilenameTest, MaxDimensionsStillWorks) {
    auto fn = gen.image_filename("x", "0", 10000, 10000);
    EXPECT_EQ(fn.size(), 40u);
}

TEST_F(FilenameTest, LongPromptStillWorks) {
    std::string lp(1000, 'x');
    auto fn = gen.image_filename(lp, "s", 1, 1);
    EXPECT_EQ(fn.size(), 40u);
}

TEST_F(FilenameTest, UnicodePromptStillWorks) {
    auto fn = gen.image_filename("\u00E9\u00E0\u00FC", "s", 1, 1);
    EXPECT_EQ(fn.size(), 40u);
}

// ===========================================================================
// valid_dimensions
// ===========================================================================
TEST(ImageGeneratorValidDim, min_min_valid) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(1, 1));
}

TEST(ImageGeneratorValidDim, max_max_valid) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(10000, 10000));
}

TEST(ImageGeneratorValidDim, typical) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(800, 600));
    EXPECT_TRUE(ImageGenerator::valid_dimensions(512, 512));
}

TEST(ImageGeneratorValidDim, zero_invalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(0, 512));
    EXPECT_FALSE(ImageGenerator::valid_dimensions(512, 0));
    EXPECT_FALSE(ImageGenerator::valid_dimensions(0, 0));
}

TEST(ImageGeneratorValidDim, negative_invalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(-1, 512));
    EXPECT_FALSE(ImageGenerator::valid_dimensions(512, -1));
    EXPECT_FALSE(ImageGenerator::valid_dimensions(-1, -1));
    EXPECT_FALSE(ImageGenerator::valid_dimensions(-1000, -1000));
}

TEST(ImageGeneratorValidDim, over_max_invalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(10001, 512));
    EXPECT_FALSE(ImageGenerator::valid_dimensions(512, 10001));
    EXPECT_FALSE(ImageGenerator::valid_dimensions(10001, 10001));
}

TEST(ImageGeneratorValidDim, boundary_min) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(1, 10000));
    EXPECT_TRUE(ImageGenerator::valid_dimensions(10000, 1));
}

TEST(ImageGeneratorValidDim, boundary_max) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(10000, 1));
    EXPECT_TRUE(ImageGenerator::valid_dimensions(1, 10000));
}

TEST(ImageGeneratorValidDim, boundary_plus_one_invalid) {
    EXPECT_FALSE(ImageGenerator::valid_dimensions(10001, 1));
    EXPECT_FALSE(ImageGenerator::valid_dimensions(1, 10001));
}

// ===========================================================================
// clamp_dimension / clamp_width / clamp_height
// ===========================================================================
TEST(ImageGeneratorClamp, clamp_dimension_basic) {
    EXPECT_EQ(ImageGenerator::clamp_dimension(500, 1, 10000), 500);
    EXPECT_EQ(ImageGenerator::clamp_dimension(0, 1, 10000), 1);
    EXPECT_EQ(ImageGenerator::clamp_dimension(-100, 1, 10000), 1);
    EXPECT_EQ(ImageGenerator::clamp_dimension(10001, 1, 10000), 10000);
    EXPECT_EQ(ImageGenerator::clamp_dimension(9999999, 1, 10000), 10000);
}

TEST(ImageGeneratorClamp, clamp_dimension_boundaries) {
    EXPECT_EQ(ImageGenerator::clamp_dimension(1, 1, 10000), 1);
    EXPECT_EQ(ImageGenerator::clamp_dimension(10000, 1, 10000), 10000);
    EXPECT_EQ(ImageGenerator::clamp_dimension(2, 1, 10000), 2);
    EXPECT_EQ(ImageGenerator::clamp_dimension(9999, 1, 10000), 9999);
}

TEST(ImageGeneratorClamp, clamp_dimension_lo_eq_hi) {
    EXPECT_EQ(ImageGenerator::clamp_dimension(0, 5, 5), 5);
    EXPECT_EQ(ImageGenerator::clamp_dimension(10, 5, 5), 5);
    EXPECT_EQ(ImageGenerator::clamp_dimension(5, 5, 5), 5);
}

TEST(ImageGeneratorClamp, clamp_dimension_negative_range) {
    EXPECT_EQ(ImageGenerator::clamp_dimension(-100, -50, -10), -50);
    EXPECT_EQ(ImageGenerator::clamp_dimension(0, -50, -10), -10);
    EXPECT_EQ(ImageGenerator::clamp_dimension(-30, -50, -10), -30);
}

TEST(ImageGeneratorClamp, clamp_width_uses_defaults) {
    EXPECT_EQ(ImageGenerator::clamp_width(0), 1);
    EXPECT_EQ(ImageGenerator::clamp_width(10001), 10000);
    EXPECT_EQ(ImageGenerator::clamp_width(512), 512);
}

TEST(ImageGeneratorClamp, clamp_height_uses_defaults) {
    EXPECT_EQ(ImageGenerator::clamp_height(0), 1);
    EXPECT_EQ(ImageGenerator::clamp_height(10001), 10000);
    EXPECT_EQ(ImageGenerator::clamp_height(512), 512);
}

TEST(ImageGeneratorClamp, clamp_width_matches_clamp_dimension_with_imagegen_range) {
    for (int v : {0, 1, 512, 10000, 10001})
        EXPECT_EQ(ImageGenerator::clamp_width(v),
                  ImageGenerator::clamp_dimension(v, ImageGenerator::MIN_DIM, ImageGenerator::MAX_DIM));
}

TEST(ImageGeneratorClamp, clamp_height_matches_clamp_dimension_with_imagegen_range) {
    for (int v : {0, 1, 512, 10000, 10001})
        EXPECT_EQ(ImageGenerator::clamp_height(v),
                  ImageGenerator::clamp_dimension(v, ImageGenerator::MIN_DIM, ImageGenerator::MAX_DIM));
}

TEST(ImageGeneratorClamp, MIN_DIM_is_1) {
    EXPECT_EQ(ImageGenerator::MIN_DIM, 1);
}

TEST(ImageGeneratorClamp, MAX_DIM_is_10000) {
    EXPECT_EQ(ImageGenerator::MAX_DIM, 10000);
}

// ===========================================================================
// sanitize_prompt
// ===========================================================================
TEST(SanitizePrompt, empty) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt(""), "");
}

TEST(SanitizePrompt, spaces_only) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("   "), "");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\t\t"), "");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\n\n"), "");
}

TEST(SanitizePrompt, single_word) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("cat"), "cat");
}

TEST(SanitizePrompt, leading_trailing_spaces) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  cat  "), "cat");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\t\tdog\t\t"), "dog");
}

TEST(SanitizePrompt, internal_spaces_preserved) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a b c"), "a b c");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("one two three"), "one two three");
}

TEST(SanitizePrompt, multiple_internal_spaces_collapsed) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a  b"), "a b");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a   b   c"), "a b c");
}

TEST(SanitizePrompt, leading_and_trailing_and_internal) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  one   two  "), "one two");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\t\t\ta  b\t\t"), "a b");
}

TEST(SanitizePrompt, tabs_collapse) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a\t\tb"), "a b");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\t\ta\t\tb\t\t"), "a b");
}

TEST(SanitizePrompt, newlines_collapse) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a\nb"), "a b");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\n\none\n\ntwo\n\n"), "one two");
}

TEST(SanitizePrompt, mixed_whitespace_collapse) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt(" a \t b \n c "), "a b c");
}

TEST(SanitizePrompt, single_char) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("x"), "x");
    EXPECT_EQ(ImageGenerator::sanitize_prompt(" x "), "x");
}

TEST(SanitizePrompt, numbers_preserved) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("123 456"), "123 456");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  42  "), "42");
}

TEST(SanitizePrompt, special_chars_preserved) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("a!b@c#"), "a!b@c#");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("one,two;three"), "one,two;three");
}

TEST(SanitizePrompt, unicode_preserved) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\u00E9\u00E0"), "\u00E9\u00E0");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  \u00FC  "), "\u00FC");
}

TEST(SanitizePrompt, already_clean_noop) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("hello world"), "hello world");
}

TEST(SanitizePrompt, prompt_with_leading_number) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  42 cats"), "42 cats");
}

TEST(SanitizePrompt, prompt_with_mixed_case) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  HeLLo WoRLd  "), "HeLLo WoRLd");
}

TEST(SanitizePrompt, prompt_with_dashes) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  foo-bar  "), "foo-bar");
}

TEST(SanitizePrompt, prompt_with_underscores) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  foo_bar  "), "foo_bar");
}

TEST(SanitizePrompt, prompt_with_plus) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("  a+b  "), "a+b");
}

TEST(SanitizePrompt, long_prompt_collapse) {
    std::string lots(200, 'x');
    std::string in = "  " + lots + "  ";
    std::string out = ImageGenerator::sanitize_prompt(in);
    EXPECT_EQ(out, lots);
    EXPECT_EQ(out.size(), size_t(200));
}

TEST(SanitizePrompt, trim_empty_to_empty) {
    EXPECT_EQ(ImageGenerator::sanitize_prompt("   "), "");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\t"), "");
    EXPECT_EQ(ImageGenerator::sanitize_prompt("\n"), "");
}

// ===========================================================================
// default_seed
// ===========================================================================
TEST(DefaultSeed, returns_zero_string) {
    EXPECT_EQ(ImageGenerator::default_seed(), "0");
}

TEST(DefaultSeed, not_empty) {
    EXPECT_FALSE(ImageGenerator::default_seed().empty());
}

TEST(DefaultSeed, consistent) {
    EXPECT_EQ(ImageGenerator::default_seed(), ImageGenerator::default_seed());
}

// ===========================================================================
// build_full_path
// ===========================================================================
TEST(BuildFullPath, empty_dir) {
    EXPECT_EQ(ImageGenerator::build_full_path("", "img.png"), "img.png");
}

TEST(BuildFullPath, simple_dir) {
    EXPECT_EQ(ImageGenerator::build_full_path("out", "img.png"), "out/img.png");
}

TEST(BuildFullPath, dir_with_slash) {
    EXPECT_EQ(ImageGenerator::build_full_path("out/", "img.png"), "out/img.png");
}

TEST(BuildFullPath, dir_with_backslash) {
    EXPECT_EQ(ImageGenerator::build_full_path("out\\", "img.png"), "out\\img.png");
}

TEST(BuildFullPath, nested_dir) {
    EXPECT_EQ(ImageGenerator::build_full_path("a/b", "img.png"), "a/b/img.png");
}

TEST(BuildFullPath, nested_dir_with_slash) {
    EXPECT_EQ(ImageGenerator::build_full_path("a/b/", "img.png"), "a/b/img.png");
}

TEST(BuildFullPath, dir_no_name) {
    EXPECT_EQ(ImageGenerator::build_full_path("", ""), "");
}

TEST(BuildFullPath, dir_with_spaces) {
    EXPECT_EQ(ImageGenerator::build_full_path("my images", "img.png"), "my images/img.png");
}

TEST(BuildFullPath, root_like) {
    EXPECT_EQ(ImageGenerator::build_full_path("/", "img.png"), "/img.png");
    EXPECT_EQ(ImageGenerator::build_full_path("//", "img.png"), "//img.png");
}

// ===========================================================================
// Cross-method interaction
// ===========================================================================
TEST(Interaction, sanitize_then_filename) {
    ImageGenerator gen;
    auto raw = "  hello   world  ";
    auto clean = ImageGenerator::sanitize_prompt(raw);
    EXPECT_EQ(clean, "hello world");
    auto fn = gen.image_filename(clean, "42", 512, 512);
    EXPECT_EQ(fn.size(), 40u);
    EXPECT_TRUE(fn.rfind("img_", 0) == 0);
    EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
}

TEST(Interaction, valid_dims_then_filename) {
    EXPECT_TRUE(ImageGenerator::valid_dimensions(512, 512));
    ImageGenerator gen;
    auto fn = gen.image_filename("test", "1", 512, 512);
    EXPECT_EQ(fn.size(), 40u);
}

TEST(Interaction, clamp_then_filename) {
    int w = ImageGenerator::clamp_width(0);
    int h = ImageGenerator::clamp_height(10001);
    EXPECT_EQ(w, 1);
    EXPECT_EQ(h, 10000);
    ImageGenerator gen;
    auto fn = gen.image_filename("test", "1", w, h);
    EXPECT_EQ(fn.size(), 40u);
}

// ===========================================================================
// Properties of the SHA-256 derived filename
// ===========================================================================
TEST(ShaFilenameProperty, same_hash_across_rebuilds) {
    // Just re-run deterministic
    ImageGenerator gen;
    auto a = gen.image_filename("seed_test", "123", 512, 512);
    auto b = gen.image_filename("seed_test", "123", 512, 512);
    ASSERT_EQ(a, b);
}

TEST(ShaFilenameProperty, different_input_gives_different_prefix) {
    ImageGenerator gen;
    auto a = gen.image_filename("x", "1", 100, 100);
    auto b = gen.image_filename("y", "1", 100, 100);
    EXPECT_NE(a, b);
}

TEST(ShaFilenameProperty, hash_is_lower_case_hex) {
    ImageGenerator gen;
    auto fn = gen.image_filename("test", "seed", 512, 512);
    auto hex = fn.substr(4, 32);
    for (char c : hex)
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
}

TEST(ShaFilenameProperty, length_consistent) {
    ImageGenerator gen;
    for (auto& prompt : {std::string("a"), std::string("ab"), std::string("abc"), std::string("abcdefghijklmnopqrstuvwxyz"),
                          std::string(1000, 'x')}) {
        auto fn = gen.image_filename(prompt, "1", 100, 100);
        EXPECT_EQ(fn.size(), 40u);
    }
}

TEST(ShaFilenameProperty, size_param_encoding) {
    ImageGenerator gen;
    auto f1 = gen.image_filename("p", "s", 1, 1);
    auto f2 = gen.image_filename("p", "s", 100, 100);
    EXPECT_NE(f1, f2);
}

TEST(ShaFilenameProperty, width_and_height_both_matter) {
    ImageGenerator gen;
    auto a = gen.image_filename("p", "s", 100, 200);
    auto b = gen.image_filename("p", "s", 200, 100);
    EXPECT_NE(a, b);
}

// ===========================================================================
// Fuzz-like coverage
// ===========================================================================
TEST(Fuzz, various_inputs) {
    ImageGenerator gen;
    struct Case { std::string p; std::string s; int w; int h; };
    Case cases[] = {
        {"", "", 1, 1},
        {"a", "1", 1, 1},
        {"abc", "xyz", 10, 20},
        {"\t", "\n", 0, 0},
        {"long prompt with  spaces", "seed with  spaces", 512, 512},
        {"", "seed", 10000, 10000},
        {"prompt", "", 512, 512},
        {"prompt", "seed", 1, 10000},
        {"prompt", "seed", 10000, 1},
    };
    for (auto& c : cases) {
        auto fn = gen.image_filename(c.p, c.s, c.w, c.h);
        EXPECT_EQ(fn.size(), 40u);
        EXPECT_TRUE(fn.rfind("img_", 0) == 0);
        EXPECT_EQ(fn.substr(fn.size() - 4), ".png");
    }
}
