// ===========================================================================
// include/image_gen.hpp — HttpClient + ImageGenerator declarations
// ===========================================================================
#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

class HttpClient {
public:
    static void init_curl();
    static void cleanup_curl();

    static std::string build_pollinations_url(
        const std::string& prompt,
        int width = 512,
        int height = 512,
        const std::string& model = "flux",
        const std::string& seed = "");
};

class ImageGenerator {
public:
    std::optional<std::string> generate(
        const std::string& prompt,
        const std::string& output_dir,
        int width = 512,
        int height = 512,
        const std::string& model = "flux",
        const std::string& seed = "");

    std::vector<std::optional<std::string>> generate_from_json_config(
        const nlohmann::json& config,
        const std::string& output_dir);
};
