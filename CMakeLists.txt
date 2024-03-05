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
add_test(NAME parser_test COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/input1.txt -out test_output_parser_test)
add_test(NAME fibonacci COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/fibonacci.inslt -out test_output_fibonacci)
add_test(NAME multi_dimensional_array COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/multi_dimensional_array.inslt -out test_output_multi_dimensional_array)
add_test(NAME read_input COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/read_input.inslt -out test_output_read_input)
add_test(NAME fast_fibonacci COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/fast_fibonacci.inslt -out test_output_fast_fibonacci)
add_test(NAME gridpath COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/gridpath.inslt -out test_output_gridpath)
add_test(NAME add_arrays COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/add_arrays.inslt -out test_output_add_arrays)
add_test(NAME abc COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/abc.inslt -out test_output_abc)
add_test(NAME multiline_strings COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/multiline_strings.inslt -out test_output_multiline_strings)
add_test(NAME function_definitions_everywhere COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/function_definitions_everywhere.inslt -out test_output_function_definitions_everywhere)
add_test(NAME functions_do_return_fail COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/functions_do_return_fail.inslt -out test_output_functions_do_return_fail)
set_property(TEST functions_do_return_fail PROPERTY WILL_FAIL TRUE)
add_test(NAME functions_do_return_ok COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/functions_do_return_ok.inslt -out test_output_functions_do_return_ok)

# uncomment the following lines to enable tests for exercise 1.1
add_test(NAME solution_1.1_1 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.1_tests/solution_1.1_1.inslt -out test_output_solution_1.1_1)
add_test(NAME solution_1.1_2 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.1_tests/solution_1.1_2.inslt -out test_output_solution_1.1_2)
add_test(NAME solution_1.1_3 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.1_tests/solution_1.1_3.inslt -out test_output_solution_1.1_3)
set_property(TEST solution_1.1_2 PROPERTY WILL_FAIL TRUE)
set_property(TEST solution_1.1_3 PROPERTY WILL_FAIL TRUE)

# uncomment the following lines to enable tests for exercise 1.2
# add_test(NAME solution_1.2_1 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.2_tests/solution_1.2_1.inslt -out test_output_solution_1.2_1)
# add_test(NAME solution_1.2_2 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.2_tests/solution_1.2_2.inslt -out test_output_solution_1.2_2)
# add_test(NAME solution_1.2_3 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.2_tests/solution_1.2_3.inslt -out test_output_solution_1.2_3)
# add_test(NAME solution_1.2_4 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.2_tests/solution_1.2_4.inslt -out test_output_solution_1.2_4)
# add_test(NAME solution_1.2_5 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_1.2_tests/solution_1.2_5.inslt -out test_output_solution_1.2_5)
# set_property(TEST solution_1.2_2 PROPERTY WILL_FAIL TRUE)
# set_property(TEST solution_1.2_3 PROPERTY WILL_FAIL TRUE)
# set_property(TEST solution_1.2_4 PROPERTY WILL_FAIL TRUE)
# set_property(TEST solution_1.2_5 PROPERTY WILL_FAIL TRUE)

# uncomment the following lines to enable tests for exercise 2.1
# add_executable(test_output_2.1 ${PROJECT_SOURCE_DIR}/tests/solution_2.1_tests/test.cpp)
# add_test(NAME solution_2.1_1 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_2.1_tests/solution_2.1_1.inslt -out test_output_solution_2.1_1)
# set_tests_properties(solution_2.1_1 PROPERTIES FIXTURES_SETUP exe_2.1_1)
# add_test(
#     NAME output_test_solution_2.1_1
#     COMMAND test_output_2.1 ${PROJECT_SOURCE_DIR}/tests/solution_2.1_tests/expected_output_2.1_1.txt test_output_solution_2.1_1 test_output_solution_2.1_1.txt
#     DEPENDS solution_2.1_1
# )
# set_tests_properties(output_test_solution_2.1_1 PROPERTIES FIXTURES_REQUIRED exe_2.1_1)

# uncomment the following lines to enable tests for exercise 3.1
# add_executable(test_output_3.1 ${PROJECT_SOURCE_DIR}/tests/solution_3.1_tests/test.cpp)
# add_test(NAME solution_3.1_1 COMMAND inslt ${PROJECT_SOURCE_DIR}/tests/solution_3.1_tests/solution_3.1_1.inslt -out test_output_solution_3.1_1)
# set_tests_properties(solution_3.1_1 PROPERTIES FIXTURES_SETUP exe_3.1_1)
# add_test(
#     NAME output_test_solution_3.1_1
#     COMMAND test_output_3.1 ${PROJECT_SOURCE_DIR}/tests/solution_3.1_tests/expected_output_3.1_1.txt test_output_solution_3.1_1 test_output_solution_3.1_1.txt
#     DEPENDS solution_3.1_1
# )
# set_tests_properties(output_test_solution_3.1_1 PROPERTIES FIXTURES_REQUIRED exe_3.1_1)
