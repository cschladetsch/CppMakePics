cmake_minimum_required(VERSION 3.25)

if(POLICY CMP0155)
    cmake_policy(SET CMP0155 OLD)
endif()

project(StaticDiffusionBridge CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/json/include)

add_subdirectory(external/SDL)

add_executable(DiffusionApp main.cpp)

if(MSVC)
    target_compile_options(DiffusionApp PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(DiffusionApp PRIVATE -Wall -Wextra -pedantic -O3)
endif()

# Ensure SDL3 include path is visible to DiffusionApp
target_include_directories(DiffusionApp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include")

