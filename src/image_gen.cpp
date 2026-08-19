// ===========================================================================
// src/image_gen.cpp — HttpClient + ImageGenerator definitions
// ===========================================================================
#include "image_gen.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

// ---- HttpClient ----

void HttpClient::init_curl() {
    static bool inited = false;
    if (!inited) {
        curl_global_init(CURL_GLOBAL_ALL);
        inited = true;
    }
}

void HttpClient::cleanup_curl() {
    static bool cleaned = false;
    if (!cleaned) {
        curl_global_cleanup();
        cleaned = true;
    }
}

std::string HttpClient::build_pollinations_url(
    const std::string& prompt,
    int width,
    int height,
    const std::string& model,
    const std::string& seed)
{
    auto url_encode = [](const std::string& s) {
        std::string r;
        r.reserve(s.size() * 3);
        for (unsigned char c : s) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
                r += static_cast<char>(c);
            else {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "%%%02X", c);
                r += buf;
            }
        }
        return r;
    };
    std::string encoded = url_encode(prompt);
    std::string url = "https://image.pollinations.ai/prompt/" + encoded;
    url += "?width=" + std::to_string(width);
    url += "&height=" + std::to_string(height);
    url += "&model=" + model;
    if (!seed.empty()) url += "&seed=" + seed;
    return url;
}

// ---- ImageGenerator ----

std::optional<std::string> ImageGenerator::generate(
    const std::string& prompt,
    const std::string& output_dir,
    int width,
    int height,
    const std::string& model,
    const std::string& seed)
{
    std::string url = HttpClient::build_pollinations_url(prompt, width, height, model, seed);
    std::cout << "[ImageGenerator] Prompt: \"" << prompt << "\"\n";
    std::cout << "[ImageGenerator] URL: " << url << "\n";

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[ImageGenerator] Failed to init curl\n";
        return std::nullopt;
    }

    std::vector<uint8_t> buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        [](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            auto* buf = static_cast<std::vector<uint8_t>*>(userdata);
            buf->insert(buf->end(), static_cast<uint8_t*>(ptr),
                        static_cast<uint8_t*>(ptr) + size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[ImageGenerator] Download failed: "
                  << curl_easy_strerror(res) << "\n";
        return std::nullopt;
    }

    if (buffer.empty()) {
        std::cerr << "[ImageGenerator] Empty response\n";
        return std::nullopt;
    }

    fs::create_directories(output_dir);
    std::string filename = "img_" +
        std::to_string(std::hash<std::string>{}(
            prompt + seed + std::to_string(width) + "x" + std::to_string(height)))
        + ".png";
    std::string full_path = output_dir + "/" + filename;
    std::ofstream out(full_path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    std::cout << "[ImageGenerator] Saved: " << full_path
              << " (" << buffer.size() << " bytes)\n";
    return full_path;
}

std::vector<std::optional<std::string>> ImageGenerator::generate_from_json_config(
    const json& config,
    const std::string& output_dir)
{
    std::vector<std::optional<std::string>> results;
    if (!config.contains("image_prompts") || !config["image_prompts"].is_array())
        return results;

    for (const auto& prompt_obj : config["image_prompts"]) {
        std::string prompt = prompt_obj.value("prompt", "");
        int width = prompt_obj.value("width", 512);
        int height = prompt_obj.value("height", 512);
        std::string model = prompt_obj.value("model", "flux");
        std::string seed = prompt_obj.value("seed", "");
        if (!prompt.empty())
            results.push_back(generate(prompt, output_dir, width, height, model, seed));
    }
    return results;
}
