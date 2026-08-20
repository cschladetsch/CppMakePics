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
    int band_height = std::max(1, height / 6);

    for (int y = 0; y < height; ++y) {
        int band = std::min(y / band_height, 5);
        RGB base = colors[band];

        float t = static_cast<float>(y % band_height) / band_height;

        for (int x = 0; x < width; ++x) {
            float tx = static_cast<float>(x) / width;
            int r = static_cast<int>(base.r * (1.0f - t * 0.3f) + 50 * tx);
            int g = static_cast<int>(base.g * (1.0f - t * 0.3f) + 70 * (1.0f - tx));
            int b = static_cast<int>(base.b * (1.0f - t * 0.3f) + 90 * tx * (1.0f - tx));
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
