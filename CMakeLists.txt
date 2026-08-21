cmake_minimum_required(VERSION 3.25)

project(StaticDiffusionBridge CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0155)
    cmake_policy(SET CMP0155 OLD)
endif()

add_compile_definitions(_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERIFY_INTERFACE_HEADER_SETS OFF)

# --- SDL3 from submodule ---
add_subdirectory(external/SDL)

# --- GoogleTest from submodule ---
add_subdirectory(external/googletest)

# --- Main app ---
add_executable(DiffusionApp
    src/main.cpp
    src/image_gen.cpp
    src/test_stub.cpp
)

target_include_directories(DiffusionApp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(DiffusionApp PRIVATE
    SDL3::SDL3
    GTest::gtest
)

set_target_properties(DiffusionApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# --- Unit tests ---
enable_testing()
add_subdirectory(tests)

# --- Compiler warnings ---
if(MSVC)
    target_compile_options(DiffusionApp PRIVATE /W4 /permissive- /Zc:__cplusplus)
    target_compile_options(TestRunner PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(DiffusionApp PRIVATE -Wall -Wextra -pedantic -O3)
    target_compile_options(TestRunner PRIVATE -Wall -Wextra -pedantic -O3)
endif()
