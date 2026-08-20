cmake_minimum_required(VERSION 3.25)

project(StaticDiffusionBridge CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0155)
    cmake_policy(SET CMP0155 OLD)
endif()

add_compile_definitions(_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)

# --- Subdirectories ---
add_subdirectory(external/SDL)
add_subdirectory(external/json)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# --- Main app ---
add_executable(DiffusionApp
    src/main.cpp
    src/image_gen.cpp
    src/test_stub.cpp
)

target_include_directories(DiffusionApp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/json/include"
)

target_link_libraries(DiffusionApp PRIVATE
    SDL3-shared
)

add_custom_command(TARGET DiffusionApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3-shared>
    $<TARGET_FILE_DIR:DiffusionApp>
)

# --- Unit tests ---
enable_testing()
add_subdirectory(tests)

if(MSVC)
    target_compile_options(DiffusionApp PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(DiffusionApp PRIVATE -Wall -Wextra -pedantic -O3)
endif()
