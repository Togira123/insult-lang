cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -g3 -ggdb3 -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_BUILD_TYPE Debug)
project(insult)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
file(GLOB all_srcs
    ${PROJECT_SOURCE_DIR}/include/*.h
    ${PROJECT_SOURCE_DIR}/include/*.hpp
    ${PROJECT_SOURCE_DIR}/src/*.c
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)
add_executable(a.out ${all_srcs})
