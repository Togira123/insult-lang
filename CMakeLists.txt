cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG " -ggdb3 -g3 -DDEBUG -D_GLIBCXX_DEBUG")
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
add_executable(inslt ${all_srcs})
add_test(NAME parser_test COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/input1.txt test_output_parser_test)
add_test(NAME fibonacci COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/fibonacci.inslt test_output_fibonacci)
add_test(NAME multi_dimensional_array COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/multi_dimensional_array.inslt test_output_multi_dimensional_array)
add_test(NAME read_input COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/read_input.inslt test_output_read_input)
add_test(NAME fast_fibonacci COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/fast_fibonacci.inslt test_output_fast_fibonacci)
add_test(NAME gridpath COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/gridpath.inslt test_output_gridpath)
add_test(NAME add_arrays COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/add_arrays.inslt test_output_add_arrays)
add_test(NAME abc COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/abc.inslt test_output_abc)
add_test(NAME multiline_strings COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/multiline_strings.inslt test_output_multiline_strings)
add_test(NAME function_definitions_everywhere COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/function_definitions_everywhere.inslt test_output_function_definitions_everywhere)

add_test(NAME solution_1.1_1 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.1_tests/solution_1.1_1.inslt test_output_solution_1.1_1)
add_test(NAME solution_1.1_2 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.1_tests/solution_1.1_2.inslt test_output_solution_1.1_2)
add_test(NAME solution_1.1_3 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.1_tests/solution_1.1_3.inslt test_output_solution_1.1_3)

set_property(TEST solution_1.1_2 PROPERTY WILL_FAIL TRUE)
set_property(TEST solution_1.1_3 PROPERTY WILL_FAIL TRUE)
