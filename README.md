# CppMakePics

Minimal C++23 project providing deterministic image filename generation with
SHA-256 fingerprinting, dimension validation, prompt sanitization, and path
construction. Built with CMake + Ninja, Clang 18, SDL3, and GoogleTest.

## Project structure

```mermaid
graph TD
    Root["CppMakePics/"] --> CMake["CMakeLists.txt<br/>project(StaticDiffusionBridge), C++23"]
    Root --> Include["include/"]
    Root --> Src["src/"]
    Root --> Tests["tests/"]
    Root --> External["external/"]
    Root --> Bin["bin/<br/>(build output, git-ignored)"]

    Include --> H1["image_gen.hpp<br/>ImageGenerator class + static helpers"]
    Include --> H2["sha256.hpp<br/>self-contained SHA-256, public domain"]

    Src --> S1["main.cpp<br/>entry point"]
    Src --> S2["image_gen.cpp<br/>ImageGenerator implementation"]
    Src --> S3["test_stub.cpp<br/>RUN_ALL_TESTS wrapper"]

    Tests --> T1["image_gen_tests.cpp<br/>78 GoogleTest cases, 9 fixtures"]
    Tests --> T2["CMakeLists.txt<br/>TestRunner target + add_test"]

    External --> SDL["SDL/<br/>SDL3 submodule"]
    External --> GT["googletest/<br/>GoogleTest v1.16.0 submodule"]

    Bin --> Exe1["DiffusionApp.exe"]
    Bin --> Exe2["tests/TestRunner.exe"]

    style Root fill:#f9f,stroke:#333
    style Bin fill:#ff9,stroke:#333
    style SDL fill:#9cf,stroke:#333
    style GT fill:#9cf,stroke:#333
```

## Build targets

```mermaid
graph LR
    subgraph "CMake configuration"
        A1["add_subdirectory(external/SDL)<br/>→ SDL3::SDL3"]
        A2["add_subdirectory(external/googletest)<br/>→ GTest::gtest, GTest::gtest_main"]
    end

    subgraph "Targets"
        T1["DiffusionApp<br/>src/main.cpp + image_gen.cpp + test_stub.cpp<br/>links: SDL3::SDL3, GTest::gtest<br/>output: bin/DiffusionApp.exe"]
        T2["TestRunner<br/>tests/image_gen_tests.cpp + ../src/image_gen.cpp + ../src/test_stub.cpp<br/>links: SDL3::SDL3, GTest::gtest, GTest::gtest_main<br/>output: bin/tests/TestRunner.exe<br/>add_test(NAME TestRunner COMMAND TestRunner)"]
    end

    A1 --> T1
    A2 --> T1
    A2 --> T2

    style T1 fill:#cfc,stroke:#333
    style T2 fill:#fcc,stroke:#333
```

## Class API

```mermaid
classDiagram
    class ImageGenerator {
        +image_filename(prompt, seed, width, height) string
        +static valid_dimensions(w, h) bool
        +static clamp_dimension(v, lo, hi) int
        +static clamp_width(w) int
        +static clamp_height(h) int
        +static sanitize_prompt(p) string
        +static default_seed() string
        +static build_full_path(dir, name) string
        -hash_prompt(prompt, seed, w, h) string
        -make_filename(prompt_hash, seed, w, h) string
    }

    class Sha256 {
        +static hash(data, len) string
        -static process_block(state, block)
        -static rotate_right(x, n) uint32_t
        -static choose(e, a, b) uint32_t
        -static majority(a, b, c) uint32_t
        -static sigma0(x) uint32_t
        -static sigma1(x) uint32_t
        -static gamma0(x) uint32_t
        -static gamma1(x) uint32_t
    }

    ImageGenerator ..> Sha256 : uses for hash_prompt
    ImageGenerator : MIN_DIM = 1
    ImageGenerator : MAX_DIM = 10000

    style ImageGenerator fill:#cfc,stroke:#333
    style Sha256 fill:#fcc,stroke:#333
```

## Filename format

```
img_<32 hex chars from SHA-256>.png
```

SHA-256 input: `prompt + "\n" + seed + "\n" + width + "x" + height`

Example: `img_a1b2c3d4e5f678901234567890123456.png`

## Test fixtures (78 tests, 9 suites)

| Fixture | Tests | Coverage |
|---|---|---|
| `FilenameTest` | 15 | determinism, uniqueness, format (img\_ prefix, .png suffix, 32 hex chars), edge cases (empty, zero, max, long, unicode) |
| `ImageGeneratorValidDim` | 9 | valid_dimensions: min/max range, zero, negative, over-max, boundaries |
| `ImageGeneratorClamp` | 10 | clamp_dimension, clamp_width, clamp_height, MIN/MAX_DIM constants |
| `SanitizePrompt` | 22 | trim, collapse whitespace, special chars, unicode, long prompts, edge cases |
| `DefaultSeed` | 3 | returns "0", not empty, consistent |
| `BuildFullPath` | 9 | empty dir, trailing slash/backslash, nested, spaces, root-like |
| `Interaction` | 3 | sanitize→filename, valid_dims→filename, clamp→filename |
| `ShaFilenameProperty` | 6 | deterministic, different inputs→different hashes, lowercase hex, length consistency, size encoding, width/height both matter |
| `Fuzz` | 1 | 9 varied inputs (empty, whitespace, long, boundary dimensions) |

## Prerequisites

- **Windows 10/11**
- **LLVM/Clang** — `C:/Program Files/LLVM/bin/clang++.exe` (Clang 18+)
  Or install via Scoop: `scoop install llvm`
- **CMake** 3.25+ with **Ninja** generator
- **Git** with submodules initialized

## Build

```bash
# From repo root
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j$(nproc)
```

Output:
- `bin/DiffusionApp.exe` — main app
- `bin/tests/TestRunner.exe` — test runner

## Run tests

```bash
# Via ctest
cd build
ctest --output-on-failure

# Direct
./bin/tests/TestRunner.exe
```

## Submodules

```bash
git submodule update --init --recursive
```

Current submodules: `external/SDL` (SDL3), `external/googletest` (GoogleTest v1.16.0).

## Compiler

Uses Clang 18.1.8 from `C:/Program Files/LLVM/bin/`. Defined
`_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH` to allow Clang to use MSVC STL.

## License

See external dependencies for their respective licenses. `sha256.hpp` is public domain.
