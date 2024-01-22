cmake_minimum_required(VERSION 3.24)
project(clox CXX)

# c++14: binary literal, digit separator
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
    collections/Vec.h
    lims.h
    collections/Arr.h
    collections/ArrStack.h
)

# GTest
find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})

add_executable(testRunner
    unittests/testRunner.cpp

    unittests/chunkTest.cpp
    unittests/collections/ArrStackTest.cpp
    unittests/collections/ArrTest.cpp
    unittests/collections/VecTest.cpp
    unittests/commonTest.cpp
    unittests/limsTest.cpp
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
    collections/Vec.h
    lims.h
    collections/Arr.h
    collections/ArrStack.h
)

target_link_libraries(testRunner ${GTEST_LIBRARIES} pthread)
