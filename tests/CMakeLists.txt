# ===========================================================================
# tests/CMakeLists.txt — Unit tests output to bin/tests/
# No GTest dependency — builds a standalone test binary.
# ===========================================================================

add_executable(TestRunner
    image_gen_tests.cpp
    ../src/image_gen.cpp
    ../src/test_stub.cpp
)

target_include_directories(TestRunner PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/json/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL/src"
)

target_link_libraries(TestRunner PRIVATE
    SDL3-shared
)

set_target_properties(TestRunner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin/tests"
)

# Copy SDL3.dll next to TestRunner.exe so it can load SDL at runtime
add_custom_command(TARGET TestRunner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3-shared>
    $<TARGET_FILE_DIR:TestRunner>
)
