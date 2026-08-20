// ===========================================================================
// src/image_gen.cpp — Local image generation using SDL3 (no network)
// Generates deterministic gradient images from prompt seeds.
// Uses SDL3's SDL_SavePNG to write PNG files that SDL can load.
// ===========================================================================
#include <SDL3/SDL.h>

#include "image_gen.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

// ---- Deterministic hash for filename generation ----

static std::uint64_t hash_string(std::string_view s) {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (char c : s) {
        h ^= static_cast<std::uint8_t>(c);
        h *= 0x100000001b3ULL;
    }
    return h;
}

struct RGB {
    std::uint8_t r, g, b;
};

// Create an SDL surface from pixel data and save as PNG
static bool save_png(const std::string& path, int width, int height,
                     const std::vector<RGB>& pixels) {
    SDL_Surface* surf = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGB24);
    if (!surf) {
        std::cerr << "[ImageGen] SDL_CreateSurface failed: " << SDL_GetError() << "\n";
        return false;
    }

    std::uint8_t* dst = static_cast<std::uint8_t*>(surf->pixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            size_t row = y * surf->pitch;
            dst[row + x * 3 + 0] = pixels[idx].r;
            dst[row + x * 3 + 1] = pixels[idx].g;
            dst[row + x * 3 + 2] = pixels[idx].b;
        }
    }

    bool ok = SDL_SavePNG(surf, path.c_str());
    SDL_DestroySurface(surf);
    if (!ok) {
        std::cerr << "[ImageGen] SDL_SavePNG failed: " << SDL_GetError() << "\n";
        return false;
    }
    return true;
}

// ---- Generate a deterministic color palette from the prompt hash ----

static std::vector<RGB> palette_from_hash(std::uint64_t h, int count = 6) {
    std::vector<RGB> colors;
    std::mt19937 rng(static_cast<unsigned int>(h));
    std::uniform_int_distribution<int> dist(30, 220);
    for (int i = 0; i < count; ++i) {
        colors.push_back({static_cast<std::uint8_t>(dist(rng)),
                          static_cast<std::uint8_t>(dist(rng)),
                          static_cast<std::uint8_t>(dist(rng))});
    }
    return colors;
}

std::string ImageGenerator::image_filename(
    const std::string& prompt,
    const std::string& seed,
    int width,
    int height)
{
    std::string key = prompt + "|" + seed + "|" + std::to_string(width) + "x" + std::to_string(height);
    auto h = hash_string(key);
    std::ostringstream oss;
    oss << "img_" << h << ".png";
    return oss.str();
}

// ---- Generate single image via gradient rendering ----

std::optional<std::string> ImageGenerator::generate(
    const std::string& prompt,
    const std::string& output_dir,
    int width,
    int height,
    const std::string& seed)
{
    fs::create_directories(output_dir);

    std::string filename = image_filename(prompt, seed, width, height);
    std::string full_path = (fs::path(output_dir) / filename).string();

    if (fs::exists(full_path)) {
        std::cout << "[ImageGen] Skip (exists): " << full_path << "\n";
        return full_path;
    }

    std::string seed_str = seed;
    if (seed_str.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 999999);
        seed_str = std::to_string(dist(gen));
    }

    std::cout << "[ImageGen] Generating: \"" << prompt << "\" -> "
              << full_path << " (" << width << "x" << height << ")\n";

    std::string key = prompt + "|" + seed_str;
    auto base_hash = hash_string(key);
    auto colors = palette_from_hash(base_hash, 6);

    std::vector<RGB> pixels(width * height);

    // Center-based radial layout: distances from center determine color band
    float cx = width / 2.0f;
    float cy = height / 2.0f;
    float max_dist = std::sqrt(cx * cx + cy * cy);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float dx = (x - cx) / cx;
            float dy = (y - cy) / cy;
            float dist = std::sqrt(dx * dx + dy * dy);   // 0 at center, ~1.4 at corners

            // Which color band: use angle + distance for variety
            float angle = std::atan2(dy, dx);            // -PI..PI
            float band_raw = (angle / (2.0f * 3.14159f) + 0.5f) * 5.0f + dist * 3.0f;
            int band = std::clamp(static_cast<int>(band_raw) % 6, 0, 5);

            // Blend between adjacent colors for smooth transitions
            int b0 = band % 6;
            int b1 = (band + 1) % 6;
            float blend = band_raw - static_cast<int>(band_raw);
            if (blend < 0) blend = 0;
            if (blend > 1) blend = 1;

            RGB c0 = colors[b0];
            RGB c1 = colors[b1];

            // distance-based vignette: darker at corners
            float vignette = 1.0f - dist * 0.4f;
            if (vignette < 0.3f) vignette = 0.3f;

            // Radial falloff from center: brighter in middle
            float radial = 1.0f - std::pow(dist * 0.7f, 2.0f);
            if (radial < 0.4f) radial = 0.4f;

            // Combine: base color blended + vignette + radial
            float bright = radial * vignette;

            int r = static_cast<int>((c0.r * (1.0f - blend) + c1.r * blend) * bright);
            int g = static_cast<int>((c0.g * (1.0f - blend) + c1.g * blend) * bright);
            int b = static_cast<int>((c0.b * (1.0f - blend) + c1.b * blend) * bright);

            // Add subtle noise for texture
            std::uint64_t px_hash = base_hash ^ (static_cast<std::uint64_t>(y) << 32) ^ static_cast<std::uint64_t>(x);
            std::mt19937 pix_rng(static_cast<unsigned int>(px_hash));
            std::uniform_int_distribution<int> noise_dist(-12, 12);
            r += noise_dist(pix_rng);
            g += noise_dist(pix_rng);
            b += noise_dist(pix_rng);

            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255);
            b = std::clamp(b, 0, 255);

            pixels[y * width + x] = {
                static_cast<std::uint8_t>(r),
                static_cast<std::uint8_t>(g),
                static_cast<std::uint8_t>(b)
            };
        }
    }

    if (!save_png(full_path, width, height, pixels)) {
        std::cerr << "[ImageGen] Failed to save image\n";
        return std::nullopt;
    }

    std::cout << "[ImageGen] Saved: " << full_path << "\n";
    return full_path;
}

// ---- Generate from JSON config ----

std::vector<std::optional<std::string>> ImageGenerator::generate_from_json_config(
    const json& config,
    const std::string& output_dir)
{
    std::vector<std::optional<std::string>> results;

    if (!config.contains("image_prompts") || !config["image_prompts"].is_array()) {
        std::cerr << "[ImageGen] Config has no image_prompts array\n";
        return results;
    }

    for (const auto& entry : config["image_prompts"]) {
        std::string prompt = entry.value("prompt", "");
        int width = entry.value("width", 512);
        int height = entry.value("height", 512);
        std::string seed = entry.value("seed", "");

        if (prompt.empty()) continue;

        std::cout << "[ImageGen] Prompt: \"" << prompt << "\" ("
                  << width << "x" << height << ", seed=" << seed << ")\n";
        auto result = generate(prompt, output_dir, width, height, seed);
        results.push_back(result);
        if (result.has_value()) {
            std::cout << "[ImageGen] OK: " << *result << "\n";
        } else {
            std::cout << "[ImageGen] FAILED\n";
        }
    }

    return results;
}
