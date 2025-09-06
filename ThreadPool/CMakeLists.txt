cmake_minimum_required(VERSION 3.28)

project(ThreadPool)

add_library(ThreadPool SHARED ThreadPool.cpp)

add_executable(test test.cpp)
target_link_libraries(test ThreadPool)