cmake_minimum_required(VERSION 3.16)

project(flux LANGUAGES CXX)

add_executable(flux-parser
    src/parser.cpp
    src/visualize.cpp
)

target_compile_features(flux-parser PRIVATE cxx_std_17)
target_include_directories(flux-parser PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_target_properties(flux-parser PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/debug"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(flux-parser PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(
    NAME parse_sample
    COMMAND flux-parser "${CMAKE_CURRENT_SOURCE_DIR}/testfile/test.fl"
)
