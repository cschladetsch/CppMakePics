# CppMakePics

Minimal C++23 application that reads `config.json` for image prompts, downloads
images from [Pollinations](https://image.pollinations.ai), and displays them in
an SDL3 window (800×600) refreshing every 30 seconds.

## Project structure

```
CppMakePics/
├── CMakeLists.txt          # Build config (CMake + Ninja, clang++)
├── config.json             # Image prompts (prompt, width, height, model, seed)
├── include/                # Public headers
│   └── image_gen.hpp
├── src/                    # Source files
│   ├── main.cpp            # SDL3 window + 30s refresh render loop
│   └── image_gen.cpp       # HttpClient + ImageGenerator
├── tests/
│   └── r.ps1              # PowerShell script: build + run normal mode
├── t.ps1                  # PowerShell script: build + run (alias for tests/r.ps1)
├── bin/                    # Build output (git-ignored)
│   ├── DiffusionApp.exe
│   ├── SDL3.dll
│   └── libcurl-x64.dll
├── build/                  # CMake build directory (git-ignored)
├── external/               # Git submodules
│   ├── SDL/               # SDL3 (release-3.4.0)
│   ├── json/              # nlohmann/json (v3.11.2)
│   └── CppLogiMake/
└── images/                 # Downloaded images (git-ignored)
```

## Prerequisites

- **Windows 10/11**
- **LLVM/Clang** — `C:/Program Files/LLVM/bin/clang++.exe` (Clang 18+)
  Or install via Scoop: `scoop install llvm`
- **CMake** 3.25+ with **Ninja** generator
- **Git** with submodules initialized
- **Scoop** package manager (for curl DLL)

## Build

```bash
# From repo root
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ ..
cmake --build .
```

Output: `bin/DiffusionApp.exe` plus `SDL3.dll` and `libcurl-x64.dll`.

## Run

```bash
# From repo root (PowerShell recommended)
./bin/DiffusionApp.exe
```

The app:
1. Reads `config.json` (creates a default one if missing)
2. Downloads images from Pollinations to `images/`
3. Displays the latest image in an 800×600 SDL3 window
4. Refreshes images every 30 seconds

Controls:
- **SPACE** — toggle overlay text
- **ESC** — quit

## Config

`config.json` format:

```json
{
    "image_prompts": [
        {
            "prompt": "a serene landscape with mountains and a lake at sunset",
            "width": 800,
            "height": 600,
            "model": "flux",
            "seed": "1001"
        }
    ]
}
```

If `config.json` doesn't exist, the app generates one with two default prompts.

## PowerShell scripts

- `t.ps1` — Build and run the application (from repo root)
- `tests/r.ps1` — Same as t.ps1 (build + run normal mode)

## Dependencies

- **SDL3** — via git submodule at `external/SDL/`
- **nlohmann/json** — via git submodule at `external/json/`
- **libcurl** — scoop-installed at `C:/Users/chris/scoop/apps/curl/8.21.0_7/`

## Submodules

```bash
git submodule update --init --recursive
```

## Compiler

Uses Clang 18.1.8 from `C:/Program Files/LLVM/bin/`. Defined
`_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH` to allow Clang to use MSVC STL
headers (required by nlohmann/json).

## License

See external dependencies for their respective licenses.
