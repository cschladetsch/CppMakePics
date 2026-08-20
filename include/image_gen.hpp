// ===========================================================================
// include/image_gen.hpp — Local image generation (no network)
// ===========================================================================
#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

class ImageGenerator {
public:
    // Generate an image from a text prompt using local Stable Diffusion.
    // Returns the path to the generated PNG, or nullopt on failure.
    std::optional<std::string> generate(
        const std::string& prompt,
        const std::string& output_dir,
        int width = 512,
        int height = 512,
        const std::string& seed = "");

    // Generate images for all prompts in a JSON config.
    std::vector<std::optional<std::string>> generate_from_json_config(
        const nlohmann::json& config,
        const std::string& output_dir);

    // Construct a deterministic filename for a given prompt/seed/size.
    std::string image_filename(
        const std::string& prompt,
        const std::string& seed,
        int width,
        int height);
};
