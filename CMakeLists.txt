cmake_minimum_required(VERSION 3.24)
project(clox CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(clox
    main.cpp
    common.h
    chunk.h
    chunk.cpp
    memory.h
    memory.cpp
    debug.h
    debug.cpp
    value.h
    value.cpp
    vm.h
    vm.cpp
    compiler.h
    compiler.cpp
    scanner.h
    scanner.cpp
    object.h
    object.cpp
    table.h
    table.cpp
    collection/Vec.h
)

# GTest
find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})

add_executable(testRunner
    unittests/testRunner.cpp
    unittests/collection/VecTest.cpp
    unittests/commonTest.cpp
    unittests/valueTest.cpp

    common.h
    chunk.h
    chunk.cpp
    memory.h
    memory.cpp
    debug.h
    debug.cpp
    value.h
    value.cpp
    vm.h
    vm.cpp
    compiler.h
    compiler.cpp
    scanner.h
    scanner.cpp
    object.h
    object.cpp
    table.h
    table.cpp
    collection/Vec.h
)

target_link_libraries(testRunner ${GTEST_LIBRARIES} pthread)
