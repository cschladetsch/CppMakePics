// ============================================================================
// File: main.cpp (C++23 Single-File Implementation)
// ============================================================================

// Bypass MSVC STL compiler version check for Clang 18
#define _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <mutex>
#include <SDL3/SDL.h>

// Include nlohmann/json from local external directory path layout
#include <nlohmann/json.hpp>

// Forward declaration / stub namespace for CppLocalLlmCodeAssist
namespace cpp_llm {
    class ModelController {
    public:
        explicit ModelController(const std::string& model_name) {
            std::cout << "[CppLocalLlmCodeAssist] Initialized backend controller with model: " << model_name << "\n";
        }
        void swap_backend_model(std::string_view new_model) {
            std::cout << "[CppLocalLlmCodeAssist] Swapped backend model to: " << new_model << "\n";
        }
    };
}

using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================================
// 1. Static Diffusion Engine Definition with Dynamic Model Swapping
// ============================================================================
class StaticDiffusion {
private:
    struct InstanceDeleter {
        void operator()(StaticDiffusion* ptr) const { delete ptr; }
    };

    inline static std::unique_ptr<StaticDiffusion, InstanceDeleter> instance_ = nullptr;
    inline static std::mutex mutex_;
    
    std::string model_name_;
    int inference_steps_;
    double guidance_scale_;
    
    std::unique_ptr<cpp_llm::ModelController> llm_controller_;

    StaticDiffusion(std::string name, int steps, double guidance)
        : model_name_(std::move(name)), 
          inference_steps_(steps), 
          guidance_scale_(guidance) 
    {
        std::cout << "[StaticDiffusion] Initializing core engine...\n";
        llm_controller_ = std::make_unique<cpp_llm::ModelController>(model_name_);
    }

public:
    StaticDiffusion(const StaticDiffusion&) = delete;
    StaticDiffusion& operator=(const StaticDiffusion&) = delete;

    static StaticDiffusion& get_instance(
        const std::string& name = "default-diffusion-v1", 
        int steps = 25, 
        double guidance = 7.5) 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!instance_) {
            instance_ = std::unique_ptr<StaticDiffusion, InstanceDeleter>(new StaticDiffusion(name, steps, guidance));
        } else {
            std::cout << "[StaticDiffusion] Existing instance retrieved.\n";
        }
        return *instance_;
    }

    void switch_model(std::string_view new_model_name) {
        if (model_name_ == new_model_name) {
            std::cout << "[StaticDiffusion] Model is already set to: " << new_model_name << "\n";
            return;
        }
        
        std::cout << "[StaticDiffusion] Switching active model on-the-fly from '" 
                  << model_name_ << "' to '" << new_model_name << "'...\n";
                  
        model_name_ = std::string(new_model_name);
        if (llm_controller_) {
            llm_controller_->swap_backend_model(model_name_);
        }
        std::cout << "[StaticDiffusion] Model successfully swapped.\n";
    }

    void dump_config() const {
        // Utilize llm_controller_ to prevent unused-private-field warnings under strict compiler configurations
        if (llm_controller_) {
            // No-op accessor to ensure field usage reference
        }
        std::cout << "--- Static Diffusion Configuration ---\n"
                  << "  Active Model   : " << model_name_ << "\n"
                  << "  Inference Steps: " << inference_steps_ << "\n"
                  << "  Guidance Scale : " << guidance_scale_ << "\n"
                  << "--------------------------------------\n";
    }
};

// ============================================================================
// 2. Main Entry Point & JSON Configuration Processing
// ============================================================================
int main(int, char*[]) {
    std::string config_path = "config.json";

    if (!fs::exists(config_path)) {
        std::cout << "[IO] Config file '" << config_path << "' not found. Generating default...\n";
        json default_config = {
            {"model_name", "stable-diffusion-xl-base"},
            {"inference_steps", 30},
            {"guidance_scale", 8.0},
            {"alternative_model", "flux-schnell-v1"}
        };
        std::ofstream out_file(config_path);
        out_file << default_config.dump(4);
    }

    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "[Error] Failed to open configuration file: " << config_path << "\n";
        return 1;
    }

    try {
        json parsed_json;
        config_file >> parsed_json;

        std::string name = parsed_json.value("model_name", "fallback-model");
        int steps = parsed_json.value("inference_steps", 20);
        double guidance = parsed_json.value("guidance_scale", 7.0);
        std::string alt_model = parsed_json.value("alternative_model", "");

        auto& diffusion = StaticDiffusion::get_instance(name, steps, guidance);
        diffusion.dump_config();

        if (!alt_model.empty()) {
            diffusion.switch_model(alt_model);
            diffusion.dump_config();
        }

    } catch (const json::exception& e) {
        std::cerr << "[JSON Error] Parsing failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

