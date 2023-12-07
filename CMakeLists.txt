cmake_minimum_required(VERSION 3.27)
project(clox CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(clox
    main.cc
    common.h
)
