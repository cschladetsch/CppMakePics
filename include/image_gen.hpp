// ===========================================================================
// include/image_gen.hpp — ImageGenerator (no external deps beyond STL)
// ===========================================================================
#pragma once

#include <optional>
#include <string>
#include <vector>

class ImageGenerator {
public:
    // Generate a deterministic filename for a given prompt/seed/size.
    std::string image_filename(
        const std::string& prompt,
        const std::string& seed,
        int width,
        int height);

    // --- dimension helpers ---
    static bool valid_dimensions(int width, int height);
    static int clamp_dimension(int v, int lo, int hi);
    static int clamp_width(int w);
    static int clamp_height(int h);
    static constexpr int MIN_DIM = 1;
    static constexpr int MAX_DIM = 10000;

    // --- prompt helpers ---
    static std::string sanitize_prompt(std::string s);
    static std::string default_seed();

    // --- path helpers ---
    static std::string build_full_path(const std::string& output_dir,
                                       const std::string& filename);
};
