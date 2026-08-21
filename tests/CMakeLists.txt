# tests/CMakeLists.txt — GoogleTest-based test runner, output to bin/tests/
add_executable(TestRunner
    image_gen_tests.cpp
    ../src/image_gen.cpp
    ../src/test_stub.cpp
)

target_include_directories(TestRunner PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)

target_link_libraries(TestRunner PRIVATE
    SDL3::SDL3
    GTest::gtest
    GTest::gtest_main
)

set_target_properties(TestRunner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin/tests"
)

add_test(NAME TestRunner COMMAND TestRunner)
