// ===========================================================================
// src/image_gen.cpp — ImageGenerator: deterministic filename generation
// ===========================================================================
#include "image_gen.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <sstream>
#include <string>

// ---- filename generation (original) ----
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

// ---- dimension helpers ----
bool ImageGenerator::valid_dimensions(int width, int height) {
    return width >= MIN_DIM && width <= MAX_DIM &&
           height >= MIN_DIM && height <= MAX_DIM;
}

int ImageGenerator::clamp_dimension(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int ImageGenerator::clamp_width(int w) {
    return clamp_dimension(w, MIN_DIM, MAX_DIM);
}

int ImageGenerator::clamp_height(int h) {
    return clamp_dimension(h, MIN_DIM, MAX_DIM);
}

// ---- prompt helpers ----
std::string ImageGenerator::sanitize_prompt(std::string s) {
    // Trim leading/trailing whitespace
    auto start = std::cbegin(s);
    auto end = std::cend(s);
    while (start != end && std::isspace(static_cast<unsigned char>(*start)))
        ++start;
    while (end != start && std::isspace(static_cast<unsigned char>(*(end - 1))))
        --end;
    s.assign(start, end);

    // Collapse consecutive whitespace into a single space
    std::string out;
    out.reserve(s.size());
    bool last_space = false;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_space) {
                out += ' ';
                last_space = true;
            }
        } else {
            out += c;
            last_space = false;
        }
    }
    return out;
}

std::string ImageGenerator::default_seed() {
    return "0";
}

// ---- path helpers ----
std::string ImageGenerator::build_full_path(const std::string& output_dir,
                                             const std::string& filename) {
    if (output_dir.empty()) return filename;
    std::string sep = "/";
    if (output_dir.back() == '/' || output_dir.back() == '\\')
        sep.clear();
    return output_dir + sep + filename;
}
