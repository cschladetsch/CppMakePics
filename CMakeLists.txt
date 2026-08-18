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

add_executable(DiffusionApp main.cpp)

if(MSVC)
    target_compile_options(DiffusionApp PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(DiffusionApp PRIVATE -Wall -Wextra -pedantic -O3)
endif()