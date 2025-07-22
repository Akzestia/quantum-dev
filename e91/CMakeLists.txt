cmake_minimum_required(VERSION 4.0.1)
project(e91)

include_directories(
    /usr/local/include/qpp
    /usr/include/eigen3
)

add_executable(e91
    e91.cpp
)
