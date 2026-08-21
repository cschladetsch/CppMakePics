// ===========================================================================
// src/main.cpp — CppMakePics: SDL3 + ImGui window with config.json-driven
// image prompts, curl image download, 30s refresh, text input field.
// ===========================================================================
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_surface.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

// Forward declaration
int run_tests();

using json = nlohmann::json;
namespace fs = std::filesystem;

// ---- config ----
struct ConfigEntry {
    std::string prompt;
    int width  = 800;
    int height = 600;
    std::string model  = "flux";
    std::string seed   = "0";
};

struct AppConfig {
    std::vector<ConfigEntry> prompts;
    int window_width  = 800;
    int window_height = 600;
    int refresh_ms    = 30000;
    std::string output_dir = "images";
};

static AppConfig load_config(const char* path) {
    AppConfig cfg;
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        // defaults
        cfg.prompts = {
            {"a serene landscape with mountains and a lake at sunset", 800, 600, "flux", "1001"},
            {"a futuristic cityscape with neon lights at night", 800, 600, "flux", "1002"},
            {"an Egyptian Goddess", 800, 600, "flux", "1003"},
        };
        return cfg;
    }
    json j;
    ifs >> j;
    ifs.close();

    if (j.contains("image_prompts") && j["image_prompts"].is_array()) {
        for (auto& p : j["image_prompts"]) {
            ConfigEntry e;
            e.prompt  = p.value("prompt", "");
            e.width   = p.value("width", 800);
            e.height  = p.value("height", 600);
            e.model   = p.value("model", "flux");
            e.seed    = p.value("seed", "0");
            if (!e.prompt.empty()) cfg.prompts.push_back(e);
        }
    }
    if (cfg.prompts.empty()) {
        cfg.prompts = {
            {"a serene landscape with mountains and a lake at sunset", 800, 600, "flux", "1001"},
            {"a futuristic cityscape with neon lights at night", 800, 600, "flux", "1002"},
        };
    }
    cfg.window_width  = j.value("window_width", 800);
    cfg.window_height = j.value("window_height", 600);
    cfg.refresh_ms    = j.value("refresh_ms", 30000);
    cfg.output_dir    = j.value("output_dir", "images");
    return cfg;
}

// ---- image download via curl (saved to output_dir) ----
struct DownloadEntry {
    std::string url;
    std::string local_path;
    bool success = false;
    std::string error;
    size_t byte_count = 0;
};

static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* stream) {
    auto* out = static_cast<std::string*>(stream);
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

static DownloadEntry download_image(const std::string& url, const std::string& local_path) {
    DownloadEntry de;
    de.url = url;
    de.local_path = local_path;
    de.success = false;

    CURL* curl = curl_easy_init();
    if (!curl) {
        de.error = "curl_easy_init failed";
        return de;
    }

    std::string data;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        de.error = std::string("curl error: ") + curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        return de;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code < 200 || http_code >= 300) {
        de.error = "HTTP " + std::to_string(http_code);
        return de;
    }

    de.byte_count = data.size();
    if (data.empty()) {
        de.error = "empty response";
        return de;
    }

    // Write to file
    FILE* f = fopen(local_path.c_str(), "wb");
    if (!f) {
        de.error = "fopen failed";
        return de;
    }
    fwrite(data.data(), 1, data.size(), f);
    fclose(f);
    de.success = true;
    return de;
}

// Build Pollinations.ai URL for a given config entry
// Format: https://image.pollinations.ai/prompt/<prompt>?width=<w>&height=<h>&seed=<seed>&model=<model>
static std::string make_pollinations_url(const ConfigEntry& e) {
    std::string encoded_prompt;
    for (char c : e.prompt) {
        if (c == ' ') encoded_prompt += '%';
        else encoded_prompt += c;
    }
    // Simple encoding: space -> %20 (URL requires %20 not literal space)
    // Proper URL encode would be more robust, but Pollinations accepts %20 for spaces.
    // Actually use %20 for spaces and keep rest as-is for simplicity.
    std::string url = "https://image.pollinations.ai/prompt/";
    for (char c : e.prompt) {
        if (c == ' ') url += "%20";
        else url += c;
    }
    url += "?width=" + std::to_string(e.width) +
           "&height=" + std::to_string(e.height) +
           "&seed=" + e.seed +
           "&model=" + e.model;
    return url;
}

// ---- image loading as SDL texture ----
static SDL_Texture* load_image_as_texture(SDL_Renderer* renderer, const std::string& path) {
    if (path.empty() || !fs::exists(path)) return nullptr;
    SDL_Surface* surf = SDL_LoadSurface(path.c_str());
    if (!surf) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    return tex;
}

// ---- ImGui backends ----
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

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
    if (test_mode) {
        return run_tests();
    }

    AppConfig cfg = load_config("config.json");
    if (cfg.prompts.empty()) {
        std::cerr << "No config entries found.\n";
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        curl_global_cleanup();
        return 1;
    }

    int win_w = cfg.window_width;
    int win_h = cfg.window_height;
    if (win_w <= 0) win_w = 800;
    if (win_h <= 0) win_h = 600;

    // Create output dir
    fs::create_directories(cfg.output_dir);

    SDL_Window* window = SDL_CreateWindow("CppMakePics",
        win_w, win_h, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        curl_global_cleanup();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        curl_global_cleanup();
        return 1;
    }

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // State
    int cur_entry = 0;
    SDL_Texture* image_tex = nullptr;
    Uint32 last_refresh = SDL_GetTicks();
    DownloadEntry last_dl;

    bool show_text_input = false;
    char text_input_buf[256] = "";

    bool running = true;
    SDL_Event ev;
    while (running) {
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_KEY_DOWN) {
                if (ev.key.key == SDLK_SPACE) {
                    cur_entry = (cur_entry + 1) % cfg.prompts.size();
                    last_refresh = 0;
                } else if (ev.key.key == SDLK_ESCAPE) {
                    if (show_text_input) {
                        show_text_input = false;
                    } else {
                        running = false;
                    }
                } else if (ev.key.key == SDLK_T) {
                    show_text_input = !show_text_input;
                    if (show_text_input) {
                        strncpy(text_input_buf, cfg.prompts[cur_entry].prompt.c_str(), 255);
                        text_input_buf[255] = '\0';
                    }
                }
            } else if (ev.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Refresh image periodically
        Uint32 now = SDL_GetTicks();
        if (now - last_refresh >= static_cast<Uint32>(cfg.refresh_ms) || !image_tex) {
            if (image_tex) {
                SDL_DestroyTexture(image_tex);
                image_tex = nullptr;
            }
            const auto& entry = cfg.prompts[cur_entry];

            // Ensure output dir exists
            fs::create_directories(cfg.output_dir);

            // Build local path
            std::string basename = "img_" + std::to_string(cur_entry) + ".png";
            std::string local_path = cfg.output_dir + "/" + basename;

            // Download
            std::string url = make_pollinations_url(entry);
            last_dl = download_image(url, local_path);

            if (last_dl.success) {
                image_tex = load_image_as_texture(renderer, local_path);
                if (!image_tex) {
                    // fallback: create red placeholder
                    SDL_Surface* ph = SDL_CreateSurface(win_w, win_h, SDL_PIXELFORMAT_RGBA8888);
                    if (ph) {
                        SDL_FillSurfaceRect(ph, nullptr, 0xFF0000FF);
                        image_tex = SDL_CreateTextureFromSurface(renderer, ph);
                        SDL_DestroySurface(ph);
                    }
                }
            } else {
                // fallback: create red placeholder
                SDL_Surface* ph = SDL_CreateSurface(win_w, win_h, SDL_PIXELFORMAT_RGBA8888);
                if (ph) {
                    SDL_FillSurfaceRect(ph, nullptr, 0xFF0000FF);
                    image_tex = SDL_CreateTextureFromSurface(renderer, ph);
                    SDL_DestroySurface(ph);
                }
            }
            last_refresh = now;
        }

        // Start ImGui frame
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        // Image window
        ImGui::Begin("CppMakePics");
        ImGui::Text("Prompt %d/%d", cur_entry + 1, static_cast<int>(cfg.prompts.size()));
        ImGui::Separator();

        if (image_tex) {
            float tex_w = 0.0f, tex_h = 0.0f;
            SDL_GetTextureSize(image_tex, &tex_w, &tex_h);
            float tex_ratio = tex_w > 0.0f && tex_h > 0.0f ? tex_w / tex_h : 1.0f;
            float avail_w = static_cast<float>(win_w) - 20.0f;
            float avail_h = static_cast<float>(win_h) - 100.0f;
            ImVec2 img_size;
            if (avail_w / avail_h > tex_ratio) {
                img_size.y = avail_h;
                img_size.x = img_size.y * tex_ratio;
            } else {
                img_size.x = avail_w;
                img_size.y = img_size.x / tex_ratio;
            }
            ImGui::Image(image_tex, img_size);
        } else {
            ImGui::TextColored(ImVec4(1, 0.3, 0.3, 1), "No image loaded");
        }

        // Status line
        if (last_dl.success) {
            ImGui::Text("Loaded: %s (%zu bytes)", last_dl.local_path.c_str(), last_dl.byte_count);
        } else if (!last_dl.error.empty()) {
            ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "Download error: %s", last_dl.error.c_str());
        }

        ImGui::Separator();

        // Text input field
        if (show_text_input) {
            ImGui::InputText("Prompt", text_input_buf, 256, ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if (ImGui::Button("Apply")) {
                cfg.prompts[cur_entry].prompt = text_input_buf;
                show_text_input = false;
                last_refresh = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                show_text_input = false;
            }
        }

        ImGui::End();

        // Bottom bar with controls
        ImGui::SetNextWindowPos(ImVec2(0, static_cast<float>(win_h) - 30), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(win_w), 30), ImGuiCond_Always);
        ImGui::Begin("BottomBar", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
        ImGui::Text("Space: next | T: edit prompt | Esc: quit | %s",
                    cfg.prompts[cur_entry].seed.c_str());
        ImGui::End();

        // Render
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0x1a, 0x1a, 0x1a, 0xFF);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (image_tex) SDL_DestroyTexture(image_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    curl_global_cleanup();
    return 0;
}

