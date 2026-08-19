cmake_minimum_required(VERSION 3.25)

if(POLICY CMP0155)
    cmake_policy(SET CMP0155 OLD)
endif()

project(StaticDiffusionBridge CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Allow Clang 18 to use MSVC STL headers (nlohmann/json pulls them in)
add_compile_definitions(_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)

# CURL: scoop-installed, no cmake config file. Set target-level include/lib directly.
set(CURL_INCLUDE_DIR "C:/Users/chris/scoop/apps/curl/8.21.0_7/include")
set(CURL_LIBRARY "C:/Users/chris/scoop/apps/curl/8.21.0_7/lib/libcurl.dll.a")
set(CURL_DLL "C:/Users/chris/scoop/apps/curl/8.21.0_7/bin/libcurl-x64.dll")

add_subdirectory(external/SDL)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(DiffusionApp
    src/main.cpp
    src/image_gen.cpp
)

target_include_directories(DiffusionApp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/json/include"
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

# Automatically copy SDL3.dll to the executable output folder after build
add_custom_command(TARGET DiffusionApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:DiffusionApp>
)

if(MSVC)
    target_compile_options(DiffusionApp PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(DiffusionApp PRIVATE -Wall -Wextra -pedantic -O3)
endif()
