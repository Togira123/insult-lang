cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -g3 -ggdb3 -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_BUILD_TYPE Debug)
project(insult)
include(CTest)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
file(GLOB all_srcs
    ${PROJECT_SOURCE_DIR}/include/*/*.h
    ${PROJECT_SOURCE_DIR}/include/*.h
    ${PROJECT_SOURCE_DIR}/src/*/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)
add_executable(a.out ${all_srcs})
add_test(NAME parser_test COMMAND a.out ${PROJECT_SOURCE_DIR}/tests/input1.txt test_output_parser_test)
add_test(NAME fibonacci COMMAND a.out ${PROJECT_SOURCE_DIR}/tests/fibonacci.inslt test_output_fibonacci)
add_test(NAME multi_dimensional_array COMMAND a.out ${PROJECT_SOURCE_DIR}/tests/multi_dimensional_array.inslt test_output_multi_dimensional_array)
add_test(NAME read_input COMMAND a.out ${PROJECT_SOURCE_DIR}/tests/read_input.inslt test_output_read_input)
add_test(NAME fast_fibonacci COMMAND a.out ${PROJECT_SOURCE_DIR}/tests/fast_fibonacci.inslt test_output_fast_fibonacci)
add_test(NAME gridpath COMMAND a.out ${PROJECT_SOURCE_DIR}/tests/gridpath.inslt test_output_gridpath)
