// ===========================================================================
// src/image_gen.cpp — ImageGenerator: deterministic filename generation
// ===========================================================================
#include "image_gen.hpp"

#include <functional>
#include <sstream>
#include <string>

std::string ImageGenerator::image_filename(
    const std::string& prompt,
    const std::string& seed,
    int width,
    int height)
{
    std::hash<std::string> hasher;
    std::size_t h = hasher(prompt + "|" + seed + "|" +
                            std::to_string(width) + "x" + std::to_string(height));
    std::ostringstream oss;
    oss << "img_" << h << ".png";
    return oss.str();
}
