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
};
