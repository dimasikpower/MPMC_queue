cmake_minimum_required(VERSION 3.14)
project(MPMCQueue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

add_executable(test_mpmc src/main.cpp)
target_include_directories(test_mpmc PRIVATE include)
target_compile_options(test_mpmc PRIVATE -O2 -pthread)