// ===========================================================================
// src/main.cpp — SDL3 window with 30-second image refresh from JSON
// ===========================================================================
#define _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#define _USE_MATH_DEFINES
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <cstring>
#include <optional>

#include "image_gen.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::json;

// ---- Forward declaration of test runner ----
void run_tests();

int main(int argc, char* argv[]) {
    bool test_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--test") == 0 ||
            std::strcmp(argv[i], "-t") == 0 ||
            std::strcmp(argv[i], "--run-tests") == 0) {
            test_mode = true;
            break;
        }
    }

    std::cout << "[App] Starting DiffusionApp...\n";
    std::cout << std::flush;

    if (test_mode) {
        run_tests();
        return 0;
    }

    // ---- Read config.json ----
    std::string config_path = "config.json";
    std::cout << "[App] Checking config.json...\n";
    std::cout << std::flush;

    if (!fs::exists(config_path)) {
        std::cout << "[IO] Config not found, generating default...\n";
        json default_config = {
            {"image_prompts", json::array({
                {{"prompt", "a serene landscape with mountains and a lake at sunset"},
                 {"width", 800}, {"height", 600}, {"model", "flux"}, {"seed", "1001"}},
                {{"prompt", "a futuristic cityscape with neon lights at night"},
                 {"width", 800}, {"height", 600}, {"model", "flux"}, {"seed", "1002"}}
            })}
        };
        std::ofstream out(config_path);
        out << default_config.dump(4);
    }
    std::cout << "[App] Config ready.\n";
    std::cout << std::flush;

    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "[Error] Cannot open " << config_path << "\n";
        return 1;
    }

    json parsed_json;
    config_file >> parsed_json;
    std::cout << "[App] JSON parsed.\n";
    std::cout << std::flush;

    std::string output_dir = "images";
    std::cout << "[App] Creating output dir...\n";
    std::cout << std::flush;
    fs::create_directories(output_dir);
    std::cout << "[App] Output dir ready.\n";
    std::cout << std::flush;

    // ---- Generate initial images ----
    std::cout << "\n[ImageGen] Generating images from config...\n";
    std::cout << std::flush;
    ImageGenerator generator;
    std::cout << "[App] Generator created.\n";
    std::cout << std::flush;
    auto results = generator.generate_from_json_config(parsed_json, output_dir);
    std::cout << "[App] generate_from_json_config returned.\n";
    std::cout << std::flush;

    int generated = 0;
    for (const auto& r : results) {
        if (r.has_value()) {
            std::cout << "[ImageGen] OK: " << *r << "\n";
            ++generated;
        } else {
            std::cout << "[ImageGen] FAILED\n";
        }
    }
    std::cout << "[ImageGen] Done: " << generated << " generated\n";
    std::cout << std::flush;

    // ---- SDL3 window ----
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[SDL] Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "DiffusionApp - Image Generator", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "[SDL] Window failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "[SDL] Renderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    // ---- Load latest image ----
    auto find_latest = [&](const std::string& dir) -> std::string {
        std::string latest;
        for (const auto& entry : fs::directory_iterator(dir)) {
            std::string ext = entry.path().extension().string();
            if (ext == ".ppm" || ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                std::string path = entry.path().string();
                if (latest.empty() || fs::last_write_time(path) > fs::last_write_time(latest))
                    latest = path;
            }
        }
        return latest;
    };

    std::string latest_image_path = find_latest(output_dir);
    SDL_Texture* image_texture = nullptr;
    if (!latest_image_path.empty()) {
        SDL_Surface* surface = SDL_LoadSurface(latest_image_path.c_str());
        if (surface) {
            image_texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (!image_texture) {
                std::cerr << "[SDL] CreateTexture failed: " << SDL_GetError() << "\n";
            } else {
                std::cout << "[SDL] Loaded: " << latest_image_path << " (" << surface->w << "x" << surface->h << ")\n";
            }
            SDL_DestroySurface(surface);
        } else {
            std::cerr << "[SDL] Load failed: " << SDL_GetError() << " for " << latest_image_path << "\n";
        }
    }

    // ---- Render loop with 30-second refresh ----
    bool running = true;
    bool show_text = true;
    SDL_Event event;
    Uint32 last_regen = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) running = false;
                else if (event.key.key == SDLK_SPACE) show_text = !show_text;
            }
        }

        Uint32 now = SDL_GetTicks();
        if (now - last_regen >= 30000) {
            last_regen = now;
            std::cout << "[ImageGen] Refreshing images (30s)...\n";
            std::cout << std::flush;

            ImageGenerator gen;
            auto refresh_results = gen.generate_from_json_config(parsed_json, output_dir);

            int new_count = 0;
            for (const auto& r : refresh_results) {
                if (r.has_value()) {
                    std::cout << "[ImageGen] Refreshed: " << *r << "\n";
                    ++new_count;
                }
            }
            generated += new_count;
            std::cout << "[ImageGen] Refresh done: +" << new_count << "\n";
            std::cout << std::flush;

            if (image_texture) SDL_DestroyTexture(image_texture);
            image_texture = nullptr;
            latest_image_path = find_latest(output_dir);
            if (!latest_image_path.empty()) {
                SDL_Surface* surface = SDL_LoadSurface(latest_image_path.c_str());
                if (surface) {
                    image_texture = SDL_CreateTextureFromSurface(renderer, surface);
                    if (!image_texture) {
                        std::cerr << "[SDL] Reload texture failed: " << SDL_GetError() << "\n";
                    } else {
                        std::cout << "[SDL] Reloaded: " << latest_image_path << " (" << surface->w << "x" << surface->h << ")\n";
                    }
                    SDL_DestroySurface(surface);
                } else {
                    std::cerr << "[SDL] Reload load failed: " << SDL_GetError() << " for " << latest_image_path << "\n";
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_RenderClear(renderer);

        if (image_texture) {
            SDL_FRect dest = {0, 0, 800.0f, 600.0f};
            SDL_RenderTexture(renderer, image_texture, nullptr, &dest);
        }

        if (show_text) {
            SDL_FRect overlay = {10.0f, 10.0f, 380.0f, 50.0f};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_RenderFillRect(renderer, &overlay);

            std::string info = "Generated: " + std::to_string(generated) +
                               " | SPACE=toggle | ESC=quit";
            SDL_Surface* surf = SDL_LoadSurface(info.c_str());
            if (surf) {
                SDL_Texture* txt = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FRect rect = {15.0f, 15.0f, 0, 0};
                SDL_GetTextureSize(txt, &rect.w, &rect.h);
                SDL_RenderTexture(renderer, txt, nullptr, &rect);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (image_texture) SDL_DestroyTexture(image_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "\n[App] Exited.\n";
    return 0;
}
