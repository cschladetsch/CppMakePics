# DiffusionApp — Technical Specification

## 1. Project Overview

**Purpose:** A C++23 single-file application that demonstrates a "static diffusion" engine concept — it reads image generation prompts from a JSON configuration file, calls the free Pollinations.ai API to generate real images, downloads them via libcurl, and displays the result in an SDL3 window. It also ships with 39 embedded unit tests.

**Entry point:** `main.cpp` — contains the entire application (engine, HTTP client, image generator, renderer, and test suite).

**Build system:** CMake + Ninja. Links SDL3 (submodule), libcurl (scoop install), and nlohmann/json (submodule).

**Execution modes:**
- **Normal mode (default):** Reads `config.json`, generates images, opens SDL window displaying the latest image.
- **Test mode (`--test` / `-t` / `--run-tests`):** Runs 39 unit tests, prints pass/fail summary, exits with failure count as exit code.


## 2. Source Architecture

### 2.1 Single-File Layout (main.cpp, ~1125 lines)

The file is organized into clearly delimited sections, each introduced by a `// ====...` banner comment:

| Section | Lines (approx) | Contents |
|---------|---------------|----------|
| Includes & forward declarations | 1–70 | SDL3, curl, stdlib, nlohmann/json, `cpp_llm::ModelController` stub |
| Test framework | 74–160 | `test` namespace: `TestCase`, `all_tests()`, `register_test()`, `run_all()`, macros `TEST_BEGIN`/`TEST_END`/`ASSERT_*` |
| StaticDiffusion class | 163–235 | Singleton engine with model name, inference steps, guidance scale, `switch_model()`, `dump_config()` |
| SimpleConfig class | 237–286 | Lightweight JSON config helper: load/save, typed getters with defaults, key existence, size |
| `io` namespace | 288–321 | Filesystem wrappers: `file_exists`, `file_size`, `read_file`, `write_file`, `create_directories`, `remove_file`, `directory_exists`, `is_empty`, `list_files` |
| `math` namespace | 323–378 | Numeric utilities: `clamp`, `clamp_int`, `lerp`, `gaussian`, `sigmoid`, `relu`, `mean`, `stddev`, `is_power_of_two`, `gcd`, `lcm`, `range`, `factorial`, `fibonacci`, `dot_product`, `norm`, `normalize` |
| `str` namespace | 381–492 | Text utilities: `trim`, `to_lower`, `to_upper`, `starts_with`, `ends_with`, `split`, `join`, `replace`, `repeat`, `hex_encode`, `hex_decode`, `url_encode`, `count_occurrences`, `truncate` |
| HttpClient class | 494–563 | libcurl wrapper: `download()` (to file), `download_to_string()`, static `build_pollinations_url()` |
| ImageGenerator class | 566–642 | Pollinations client: `generate()`, `generate_from_descriptions()`, `generate_from_json_config()`, static `image_filename()` |
| register_all_tests() | 645–916 | Explicit registration of all 39 tests (clears list first) |
| main() | 918–end | Dual-mode entry point: test runner or image generation + SDL rendering loop |

### 2.2 Namespaces

- **`cpp_llm`** — Stub namespace mimicking `CppLocalLlmCodeAssist`.  Contains `ModelController` class with constructor (prints model name), `swap_backend_model()`, and `name()`.  Used to make `StaticDiffusion` look like it's wired to a real LLM backend.
- **`test`** — Embedded test framework and all 39 test registrations.
- **`io`** — Filesystem utility functions (all inline, no state).
- **`math`** — Inline math helpers (pure functions, no state).
- **`str`** — Inline string helpers (pure functions, no state).


## 3. StaticDiffusion Engine

### 3.1 Intent

`StaticDiffusion` is a singleton that represents the "diffusion engine's" configuration state.  In this project it serves as a **placeholder / architectural demonstration** — the real image generation is done by calling the Pollinations API, not by the engine's `model_name` / `inference_steps` / `guidance_scale` fields.  Those fields are printed for display and config-dump purposes but do not currently influence the actual generation.

The class exists to show the pattern that a real diffusion engine would follow: singleton access, model name, inference step count, guidance scale, and the ability to swap models at runtime.

### 3.2 Design

```
StaticDiffusion
├── inline static unique_ptr<StaticDiffusion> instance_
├── inline static mutex mutex_
├── model_name_ : string
├── inference_steps_ : int
├── guidance_scale_ : double
└── llm_controller_ : unique_ptr<cpp_llm::ModelController>
```

- **Singleton:** `get_instance(name, steps, guidance)` — thread-safe via mutex.  Creates the instance on first call; returns the existing instance on subsequent calls.  Deleted copy constructor and assignment operator.
- **Model switching:** `switch_model(new_name)` — updates `model_name_` and calls `llm_controller_->swap_backend_model()`.  If the new name equals the current name, it prints "already set" and returns without change.
- **Config dump:** `dump_config()` — prints a formatted block showing model name, inference steps, and guidance scale.  Accesses `llm_controller_->name()` to ensure the field is "used" (satisfying any potential unused-field warnings).
- **Accessors:** `model_name()`, `inference_steps()`, `guidance_scale()` — const getters used by tests.


## 4. SimpleConfig

### 4.1 Intent

A lightweight JSON configuration helper that wraps `std::map<std::string, nlohmann::json>`.  Used by the app to read/write `config.json` and by tests to validate JSON round-tripping and typed access with defaults.

### 4.2 API

- `load(path)` — reads JSON file into `data` map.  Throws `runtime_error` on failure.
- `save(path)` — writes `data` map as formatted JSON (4-space indent).
- `get_string(key, default_val)` — returns string value or default.  Only returns the value if the stored JSON is actually a string.
- `get_int(key, default_val)` — returns int value or default.  Handles both integer and floating-point JSON numbers (truncates floats).
- `get_double(key, default_val)` — returns double value or default.
- `has_key(key)` — presence check.
- `size()` — number of keys.
- `clear()`, `set(key, value)`, `erase(key)` — mutation.


## 5. io Namespace (Filesystem Utilities)

All functions are inline and delegate to `std::filesystem`:

| Function | Returns | Notes |
|----------|---------|-------|
| `file_exists(path)` | bool | `fs::exists` |
| `file_size(path)` | uintmax_t | `fs::file_size` |
| `read_file(path)` | string | Reads entire file as text; throws on failure |
| `write_file(path, content)` | void | Binary write; throws on failure |
| `create_directories(path)` | bool | `fs::create_directories` |
| `remove_file(path)` | bool | `fs::remove` |
| `directory_exists(path)` | bool | `fs::is_directory` |
| `is_empty(path)` | bool | `fs::is_empty` |
| `list_files(dir)` | vector<string> | Sorted filenames in directory; empty if not a directory |


## 6. math Namespace

All functions are inline pure functions:

| Function | Signature | Purpose |
|----------|-----------|---------|
| `clamp` | `(double v, double lo, double hi) → double` | Constrain value to range |
| `clamp_int` | `(int v, int lo, int hi) → int` | Integer clamp |
| `lerp` | `(double a, double b, double t) → double` | Linear interpolation |
| `gaussian` | `(double x, double mu, double sigma) → double` | Normal PDF |
| `sigmoid` | `(double x) → double` | 1/(1+e^-x) |
| `relu` | `(double x) → double` | max(0, x) |
| `mean` | `(vector<double>) → double` | Arithmetic mean (0 for empty) |
| `stddev` | `(vector<double>) → double` | Sample standard deviation (0 for n<2) |
| `is_power_of_two` | `(int n) → bool` | n>0 && (n&(n-1))==0 |
| `gcd` | `(int a, int b) → int` | Euclidean algorithm |
| `lcm` | `(int a, int b) → int` | |a*b|/gcd(a,b), 0 if either is 0 |
| `range` | `(int start, int end) → vector<int>` | [start, end) |
| `factorial` | `(int n) → int` | Iterative product |
| `fibonacci` | `(int n) → int` | Iterative Fibonacci (F(0)=0, F(1)=1) |
| `dot_product` | `(vector<double> a, vector<double> b) → double` | Sum of elementwise products, truncated to shorter vector |
| `norm` | `(vector<double> v) → double` | sqrt(dot_product(v,v)) |
| `normalize` | `(vector<double> v) → vector<double>` | unit vector in same direction; returns v if norm is 0 |


## 7. str Namespace

All functions are inline pure functions:

| Function | Purpose |
|----------|---------|
| `trim(s)` | Remove leading/trailing whitespace (space, tab, newline, CR, form feed, vertical tab) |
| `to_lower(s)` | ASCII lowercase via `std::tolower` |
| `to_upper(s)` | ASCII uppercase via `std::toupper` |
| `starts_with(s, prefix)` | Check if `s` begins with `prefix` |
| `ends_with(s, suffix)` | Check if `s` ends with `suffix` |
| `split(s, delim)` | Split string by char delimiter into vector |
| `join(parts, delim)` | Join vector of strings with delimiter (empty vector → "") |
| `replace(s, from, to)` | Replace all occurrences of `from` with `to` |
| `repeat(s, n)` | Concatenate `s` `n` times |
| `hex_encode(data)` | Convert byte vector to lowercase hex string |
| `hex_decode(hex)` | Convert hex string to byte vector (throws on odd length or invalid chars) |
| `url_encode(s)` | Percent-encode per RFC 3986: alphanumerics and `- _ . ~` pass through; everything else → `%HH` |
| `count_occurrences(s, sub)` | Count non-overlapping occurrences of substring |
| `truncate(s, max_len, suffix)` | Truncate to max_len, append suffix if truncated |


## 8. HttpClient

### 8.1 Purpose

Wraps libcurl to download URLs to either a file or an in-memory string.  Provides a static helper to build Pollinations.ai image URLs.

### 8.2 Lifecycle

- Constructor: calls `curl_global_init(CURL_GLOBAL_DEFAULT)`.
- Destructor: calls `curl_global_cleanup()`.
- Each `download()`/`download_to_string()` call creates and destroys its own `CURL*` handle.

### 8.3 API

- `download(url, output_path) → bool` — Downloads URL to file.  Returns true if curl succeeded and HTTP status is 200–299.
- `download_to_string(url, out) → bool` — Downloads URL into a string.  Returns true on success.
- `build_pollinations_url(prompt, width, height, model, seed) → string` — Constructs a Pollinations.ai image URL:
  ```
  https://image.pollinations.ai/prompt/<url-encoded-prompt>?width=W&height=H&model=M&seed=S
  ```
  - `width` default 512, `height` default 512, `model` default "flux", `seed` default "".
  - The seed parameter is only appended if non-empty.

### 8.4 Callbacks

- `write_cb` — libcurl write callback that fwrites to a `FILE*`.
- `str_write_cb` — libcurl write callback that appends to a `std::string*`.

### 8.5 Notes

- `CURLOPT_FOLLOWLOCATION` is set so redirects are followed.
- Timeout 60s, connect timeout 15s for file downloads; 30s timeout for string downloads.
- The constructor/destructor pair uses `curl_global_init`/`curl_global_cleanup`.  In the current code each `generate()` call creates a new `HttpClient`, so init/cleanup happens once per image.  This is correct but not optimal for high-throughput scenarios (see Limitations).


## 9. ImageGenerator

### 9.1 Purpose

Orchestrates image generation by calling the Pollinations API via `HttpClient` and saving the result to disk.

### 9.2 API

- `generate(prompt, output_dir, width, height, model, seed) → optional<string>`
  - Computes a deterministic filename based on a hash of (prompt + seed + width + height).
  - Builds the Pollinations URL.
  - Creates the output directory if it doesn't exist.
  - Downloads the image.
  - On success: returns the full path to the saved file.
  - On failure: deletes any partial file, prints an error, returns `nullopt`.

- `generate_from_descriptions(descriptions, output_dir) → vector<optional<string>>`
  - Takes a `map<string, string>` of name→prompt pairs.
  - Prepends "name: " to the prompt if the name is non-empty.
  - Calls `generate()` for each entry.

- `generate_from_json_config(config, output_dir) → vector<optional<string>>`
  - Reads the `"image_prompts"` array from a `nlohmann::json` object.
  - For each entry extracts `prompt` (required), `seed`, `width`, `height`, `model` (all optional with defaults).
  - Calls `generate()` for each.

- `image_filename(prompt, seed, width, height) → string` (static)
  - Computes `std::hash<std::string>{}(prompt + seed + to_string(width) + "x" + to_string(height))`.
  - Returns `"img_<hex-hash>.png"`.

### 9.3 Pollinations URL Format

```
https://image.pollinations.ai/prompt/<url-encoded-prompt>?width=<W>&height=<H>&model=<M>&seed=<S>
```

- The prompt is URL-encoded using `str::url_encode()`.
- `model` is passed as-is to Pollinations (the API accepts model names like "flux", "flux-realism", etc.).
- The `seed` parameter makes generation deterministic for the same inputs.

### 9.4 Output Directory

The generator creates `generated_images/` (or whatever `output_dir` is set to) if it doesn't exist, using `io::create_directories()`.  Images are saved as hash-based `.png` filenames.


## 10. main() — Normal Mode Data Flow

```
1. Parse args → if --test/-t/--run-tests present → jump to test mode.

2. Config:
   if config.json doesn't exist → write default config with two example prompts.
   Read config.json with ifstream → parse with nlohmann::json.
   Extract: model_name, inference_steps, guidance_scale, alternative_model.

3. StaticDiffusion engine:
   get_instance(name, steps, guidance) → prints "[StaticDiffusion] Initializing core engine..."
   dump_config() → prints config block.
   If alternative_model non-empty → switch_model(alt_model) → re-dump_config().

4. Image generation:
   ImageGenerator generator;
   output_dir = "generated_images"
   create_directories(output_dir)
   if config has "image_prompts" array:
       image_results = generator.generate_from_json_config(parsed_json, output_dir)
   else:
       fallback: descs["main"] = name; if alt_model non-empty descs["alt"] = alt_model
       image_results = generator.generate_from_descriptions(descs, output_dir)

   Count generated vs failed; print summary.

5. SDL3 init:
   SDL_Init(SDL_INIT_VIDEO)
   SDL_CreateWindow("DiffusionApp - Image Generator", 800, 600, SDL_WINDOW_RESIZABLE)
   SDL_CreateRenderer(window, nullptr, 0)
   SDL_SetRenderVSync(renderer, 1, 0)   // 60 Hz VSync

6. Find latest image:
   list_files(output_dir) → filter .png/.jpg/.jpeg → pick most recent by last_write_time.

7. Load image:
   SDL_LoadSurface(path) → SDL_CreateTextureFromSurface(renderer, surface)
   → SDL_DestroySurface(original surface)
   If loading fails → texture stays null, window shows no image.

8. Render loop (60 FPS, SDL_Delay(16)):
   - Clear to (30,30,40,255).
   - If texture: SDL_RenderTexture(texture, NULL, &dest_rect) where dest_rect fills window.
   - If show_text: draw semi-transparent black overlay rect at (10,10,380,50);
     create surface from status string, create texture, query size, render at (15,15);
     destroy texture and surface.
   - SDL_RenderPresent(renderer).
   - Handle events: SDL_EVENT_QUIT / ESC → exit; SPACE → toggle show_text.

9. Cleanup: destroy texture, renderer, window, SDL_Quit().
   Print "[App] Exited normally." Return 0.
```


## 11. main() — Test Mode Data Flow

```
1. Detect --test / -t / --run-tests in argv.

2. Call test::register_all_tests():
   - Clears the internal test list.
   - Registers 39 test lambdas via TEST_BEGIN(name) … TEST_END.
   - Each lambda returns bool (true = pass).

3. Call test::run_all():
   - Iterates the list.
   - For each test:
       a. Record start time.
       b. Try: call fn(). If true → increment pass count, print "[PASS] name (Nms)".
          If false → increment fail count, print "[FAIL] name".
       c. Catch std::exception → increment fail, print "[FAIL] name — exception: msg".
       d. Catch ... → increment fail, print "[FAIL] name — unknown exception".
   - Print summary: "RESULTS: P passed, F failed, T total".
   - Return F (failure count) as exit code.
```


## 12. The 39 Tests — Detailed Breakdown

### Group 1: StaticDiffusion (tests 1–5)

Tests the singleton engine's core identity: instance uniqueness, parameter storage, and model switching.  These are the "hub" class through which the app's conceptual model name / step count / guidance flow, so validating it is highest priority.

| # | Test name | Validates |
|---|-----------|-----------|
| 1 | singleton returns same instance | Two `get_instance()` calls return the same address |
| 2 | model name is set correctly | Constructor stores name; accessor returns it |
| 3 | inference steps are stored | Constructor stores steps; accessor returns them |
| 4 | guidance scale is stored | Constructor stores guidance; accessor returns it (near-equality) |
| 5 | model switching updates name | `switch_model("new")` changes stored name from "old" |

### Group 2: SimpleConfig (tests 6–10)

Tests the JSON config helper that the app uses to read/write `config.json`.  These are filesystem-backed round-trip tests: they create temporary files, load them, and verify the values survive.

| # | Test name | Validates |
|---|-----------|-----------|
| 6 | loads valid JSON file | Write JSON with string + int, load it, read both back correctly |
| 7 | get_string returns default for missing key | Absent key returns the provided default |
| 8 | get_int returns default for missing key | Absent key returns the provided default |
| 9 | get_double returns default for missing key | Absent key returns the provided default |
| 10 | save and reload round-trip | Set string/int/double, save, reload, verify all three survive |

### Group 3: io Namespace (tests 11–15)

Tests the filesystem wrappers.  These are essential because the app cannot function without reading config, writing images, and checking directory existence.

| # | Test name | Validates |
|---|-----------|-----------|
| 11 | file_exists returns true for existing file | `main.cpp` exists and is detected |
| 12 | file_exists returns false for nonexistent file | Obvious non-path returns false |
| 13 | read_file reads file content correctly | Write "Hello, world!", read back, compare |
| 14 | write_file creates file with correct content | Write "Test content 123", read back, compare |
| 15 | directory_exists returns true for build dir | `build/` directory is detected |

### Group 4: math Namespace (tests 16–25)

Tests the numeric utility functions.  These are pure functions with deterministic outputs, so they are straightforward to test exhaustively at boundary values.

| # | Test name | Validates |
|---|-----------|-----------|
| 16 | clamp clamps below minimum | `clamp(5, 10, 20)` returns 10 |
| 17 | clamp clamps above maximum | `clamp(25, 10, 20)` returns 20 |
| 18 | clamp returns value within range | `clamp(15, 10, 20)` returns 15 |
| 19 | lerp at t=0 returns a | `lerp(10, 20, 0)` returns 10 |
| 20 | lerp at t=1 returns b | `lerp(10, 20, 1)` returns 20 |
| 21 | lerp at t=0.5 returns midpoint | `lerp(0, 100, 0.5)` returns 50 |
| 22 | is_power_of_two: true for powers | 1, 2, 4, 8, 1024 all return true |
| 23 | is_power_of_two: false for non-powers | 3, 5, 7, 100, 0 all return false |
| 24 | gcd computes correctly | (12,8)=4, (17,13)=1, (100,25)=25, (7,7)=7, (0,5)=5 |
| 25 | factorial computes correctly | 0!=1, 1!=1, 5!=120, 6!=720 |

### Group 5: str Namespace (tests 26–33)

Tests the text utility functions used for URL encoding, filename manipulation, and overlay text.  These are pure functions with clear expected outputs.

| # | Test name | Validates |
|---|-----------|-----------|
| 26 | trim removes leading/trailing whitespace | "  hello  " → "hello"; "\t\nhello\r\n" → "hello"; "hello" → "hello"; "" → "" |
| 27 | to_lower converts correctly | "Hello World" → "hello world"; "ALREADY" → "already"; "" → "" |
| 28 | to_upper converts correctly | "Hello World" → "HELLO WORLD"; "already" → "ALREADY" |
| 29 | starts_with detects prefix | "hello world" starts with "hello" (true), "world" (false); "abc" starts with "abc" (true), "abcd" (false) |
| 30 | ends_with detects suffix | "hello world" ends with "world" (true), "hello" (false); "abc" ends with "abc" (true), "abcd" (false) |
| 31 | split by delimiter | "a,b,c" split by ',' → {"a","b","c"} (size 3, each element correct) |
| 32 | join with delimiter | {"a","b","c"} joined by "," → "a,b,c"; joined by "" → "abc"; empty vector → "" |
| 33 | replace replaces all occurrences | "aaa" replace "a"→"b" → "bbb"; "hello world" replace "world"→"there" → "hello there"; "no match" with no match → unchanged |

### Group 6: HttpClient and ImageGenerator (tests 34–39)

Tests the HTTP and image-generation integration surface.  These test pure functions (URL construction, filename hashing) and lifecycle sanity (constructor/destructor) — they do **not** require network access.

| # | Test name | Validates |
|---|-----------|-----------|
| 34 | build_pollinations_url constructs valid URL | "a cat" + 512×512 + flux + seed 42 → URL with correct prefix, suffix, and URL-encoded prompt |
| 35 | build_pollinations_url default parameters | "test prompt" with defaults → URL still has width, height, model query params |
| 36 | image_filename produces consistent hash-based name | Same inputs → same filename; filename ends in ".png" |
| 37 | image_filename different prompts produce different names | "cat" vs "dog" → different filenames |
| 38 | HttpClient constructor/destructor don't crash | Create and destroy an HttpClient; if we reach the end without crash, test passes |
| 39 | output directory is created if missing | `generate()` is called with a non-existent directory; after the call (and cleanup), the directory should exist (directory creation is called unconditionally inside generate) |

**Test 39 note:** This test makes a real Pollinations API call because `ImageGenerator::generate()` always attempts the download.  In an offline environment the download will fail, but the test only asserts `true` at the end — the directory creation is the actual thing being verified (it happens before the download attempt).  The test cleans up the temporary directory afterward.  This makes the test somewhat environment-dependent; a more isolated version would test `io::create_directories` directly or mock the HTTP layer.


## 13. Test Framework Internals

### 13.1 Storage

```cpp
namespace test {
    struct TestCase {
        std::string name;
        std::function<bool()> fn;
    };

    inline std::vector<TestCase>& all_tests() {
        static std::vector<TestCase> tests;
        return tests;
    }

    inline void register_test(std::string name, std::function<bool()> fn) {
        all_tests().push_back({std::move(name), std::move(fn)});
    }
}
```

### 13.2 Registration Macros

```cpp
#define TEST_BEGIN(name) \
    { \
        register_test(name, [&]()->bool {

#define TEST_END \
        }); \
    }
```

- `TEST_BEGIN(name)` opens a scope `{`, calls `register_test()` immediately with a lambda, and starts the lambda body.
- `TEST_END` closes the lambda with `});` and closes the scope `}`.
- Because `register_test()` is called **inside** the function body (not at static initialization time), there is no static-init-order issue.  Calling `register_all_tests()` multiple times is safe because it clears the list first.

### 13.3 Assertion Macros

```cpp
#define ASSERT_TRUE(cond)   if (!(cond)) { err; return false; }
#define ASSERT_FALSE(cond)  if (cond)    { err; return false; }
#define ASSERT_EQ(a, b)     if ((a) != (b)) { err with values; return false; }
#define ASSERT_NEAR(a, b, eps) if (|a-b| > eps) { err with values; return false; }
#define ASSERT_THROWS(expr) do { bool caught=false; try{expr;}catch(...){caught=true;} if(!caught){err; return false;} } while(0)
```

All macros print the failed expression text via `#cond` (stringification) and, for `ASSERT_EQ`/`ASSERT_NEAR`, the actual runtime values.

### 13.4 Runner

```cpp
inline int run_all() {
    auto& tests = all_tests();
    for (auto& t : tests) {
        auto start = high_resolution_clock::now();
        try {
            bool ok = t.fn();
            auto ms = duration_cast<milliseconds>(now - start).count();
            if (ok) { ++passed; print "[PASS] name (Nms)"; }
            else    { ++failed; print "[FAIL] name"; }
        } catch (std::exception& e) { ++failed; print "[FAIL] name — exception: e.what()"; }
        catch (...)                { ++failed; print "[FAIL] name — unknown exception"; }
    }
    print summary;
    return failed;
}
```


## 14. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.25)
project(StaticDiffusionBridge CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/json/include)

# CURL: scoop-installed, no cmake config file. Direct paths.
set(CURL_INCLUDE_DIR "C:/Users/chris/scoop/apps/curl/8.21.0_7/include")
set(CURL_LIBRARY "C:/Users/chris/scoop/apps/curl/8.21.0_7/lib/libcurl.dll.a")
set(CURL_DLL "C:/Users/chris/scoop/apps/curl/8.21.0_7/bin/libcurl-x64.dll")

add_subdirectory(external/SDL)

add_executable(DiffusionApp main.cpp)

target_include_directories(DiffusionApp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include"
    "${CURL_INCLUDE_DIR}"
)

target_link_libraries(DiffusionApp PRIVATE
    SDL3::SDL3
    "${CURL_LIBRARY}"
)

# Copy curl DLL alongside the executable
add_custom_command(TARGET DiffusionApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CURL_DLL}"
    $<TARGET_FILE_DIR:DiffusionApp>
)

if(MSVC)
    target_compile_options(DiffusionApp PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(DiffusionApp PRIVATE -Wall -Wextra -pedantic -O3)
endif()

# Copy SDL3.dll to executable output folder
add_custom_command(TARGET DiffusionApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:DiffusionApp>
)
```

Key points:
- CURL paths are hardcoded to the scoop install location because curl doesn't ship a CMake config file.
- SDL3 is added as a subdirectory (git submodule).
- The curl DLL is copied to the build output directory so the executable can find it at runtime.
- SDL3.dll is also copied via a second POST_BUILD command.


## 15. config.json Schema

```json
{
    "model_name": "stable-diffusion-xl-base",
    "inference_steps": 30,
    "guidance_scale": 8.0,
    "alternative_model": "flux-schnell-v1",
    "image_prompts": [
        {
            "prompt": "a serene landscape with mountains and a lake at sunset",
            "width": 512,
            "height": 512,
            "model": "flux",
            "seed": "1001"
        }
    ]
}
```

Fields:
- `model_name` (string) — displayed by StaticDiffusion; does not currently affect generation.
- `inference_steps` (int) — placeholder; displayed but not used by Pollinations.
- `guidance_scale` (double) — placeholder; displayed but not used by Pollinations.
- `alternative_model` (string, optional) — if present, the engine switches to this model after the initial config dump.
- `image_prompts` (array, optional) — list of image generation requests.  If absent, the app falls back to using `model_name` and `alternative_model` as prompt text.

Each `image_prompts` entry:
- `prompt` (string, required) — text description sent to Pollinations.
- `width` (int, optional, default 512) — image width.
- `height` (int, optional, default 512) — image height.
- `model` (string, optional, default "flux") — Pollinations model identifier.
- `seed` (string, optional) — deterministic seed.


## 16. External Dependencies

| Dependency | Version | Source | Purpose |
|------------|---------|--------|---------|
| SDL3 | 3.5.0 | git submodule `external/SDL` | Window, renderer, event loop, surface/texture loading |
| libcurl | 8.21.0 | scoop `curl/8.21.0_7` | HTTP downloads from Pollinations.ai |
| nlohmann/json | latest | git submodule `external/json` | JSON parsing and serialization for config |
| C++ standard library | — | Clang 18.1.8 (MSVC STL) | `std::filesystem`, `std::function`, `std::optional`, etc. |

The `external/CppLocalLlmCodeAssist` submodule is present but only used as a **stub reference** — a forward-declared `cpp_llm::ModelController` class is defined inline in `main.cpp` to mimic its interface.  The real CppLocalLlmCodeAssist library is not linked or used.


## 17. Build & Run

### 17.1 Build

```bash
cd build
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  ..
cmake --build .   # or: ninja DiffusionApp
```

CMakeLists.txt already encodes the scoop curl paths, so no extra `-DCURL_*` flags are needed.

### 17.2 Run (Normal Mode)

```bash
# From PowerShell:
.\run.ps1

# Or directly:
build\DiffusionApp.exe
```

### 17.3 Run Tests

```bash
build\DiffusionApp.exe --test
build\DiffusionApp.exe -t
build\DiffusionApp.exe --run-tests
```

Exit code = number of failed tests (0 = all 39 pass).

### 17.4 run.ps1

PowerShell script that:
1. Accepts `-BuildType` (default Release) and `-Clean` switch.
2. If `-Clean`: kills running DiffusionApp/SDL3 processes, removes `build/`.
3. Ensures `build/` exists, configures with cmake, builds, then launches `DiffusionApp.exe`.


## 18. Limitations & Known Issues

1. **Text rendering:** Overlay text uses SDL's built-in surface-from-string path (no SDL_ttf).  Works on the current build but is not a robust cross-platform text solution.

2. **Image filename extension:** Always `.png` even if Pollinations returns JPEG.  SDL_LoadSurface autodetects the format, so this works, but it's inconsistent.

3. **StaticDiffusion is a placeholder:** The `model_name`, `inference_steps`, and `guidance_scale` fields are displayed but do not influence the actual Pollinations API call.  Bridging them to real generation parameters is future work.

4. **Network dependency:** Image generation requires internet access to `image.pollinations.ai`.  If the network is down, the app runs but shows no image.

5. **Singleton not resettable:** `StaticDiffusion::instance_` is an inline static.  If test mode and normal mode were combined in one process, the test's singleton would persist.  Currently the two modes are mutually exclusive (--test either is or isn't passed).

6. **Test 39 is environment-dependent:** It makes a real API call.  In an offline environment the download fails but the test still passes (it only asserts `true` at the end, and the directory creation is verified by existence check).  A more isolated test would mock HTTP or test `io::create_directories` directly.

7. **curl global init per image:** Each `generate()` call creates a new `HttpClient`, so `curl_global_init`/`curl_global_cleanup` happens once per image.  Acceptable for a handful of images per run, but a singleton HttpClient would be better for high-throughput use.

8. **`fs::path` → `std::string` conversions:** The code explicitly calls `.string()` on `fs::path` objects where needed because implicit conversion is not available in all standard library implementations.

9. **Windows header macro collision:** `min` and `max` macros from `minwindef.h` collide with `std::min`/`std::max`.  Fixed with `#define NOMINMAX` before including Windows/SDK headers (via SDL).

10. **MSVC deprecated `fopen` warning:** `fopen` is deprecated in the MSVC STL.  The code uses it in the curl write callback.  A warning is emitted but it compiles and works correctly.  Could be replaced with `fopen_s` for cleanliness.