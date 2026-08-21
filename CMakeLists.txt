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

# --- curl (system install via Scoop) ---
# Curl is at C:\Users\chris\scoop\apps\curl\8.21.0_7
find_path(CURL_INCLUDE_DIR curl/curl.h
    PATHS "C:/Users/chris/scoop/apps/curl/8.21.0_7/include"
    DOC "curl include directory")
find_library(CURL_LIBRARY curl
    PATHS "C:/Users/chris/scoop/apps/curl/8.21.0_7/lib"
    DOC "curl library")
if(CURL_INCLUDE_DIR AND CURL_LIBRARY)
    message(STATUS "curl found: ${CURL_INCLUDE_DIR} ${CURL_LIBRARY}")
else()
    message(WARNING "curl not found — build will fail if curl is required")
endif()

# --- nlohmann/json (submodule) ---
add_subdirectory(external/json)

# --- SDL3 from submodule ---
add_subdirectory(external/SDL)

# --- GoogleTest from submodule ---
add_subdirectory(external/googletest)

# --- imgui (submodule) ---
# Build imgui core + SDL3 platform + SDL3 renderer backends
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
)
set(IMGUI_BACKEND_SOURCES
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

# --- Main app ---
add_executable(DiffusionApp
    src/main.cpp
    src/image_gen.cpp
    src/test_stub.cpp
    ${IMGUI_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
)

target_include_directories(DiffusionApp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends"
)

target_link_libraries(DiffusionApp PRIVATE
    SDL3::SDL3
    GTest::gtest
    nlohmann_json::nlohmann_json
)

if(CURL_INCLUDE_DIR AND CURL_LIBRARY)
    target_include_directories(DiffusionApp PRIVATE "${CURL_INCLUDE_DIR}")
    target_link_libraries(DiffusionApp PRIVATE "${CURL_LIBRARY}")
    target_compile_definitions(DiffusionApp PRIVATE HAVE_CURL=1)
endif()

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
