cmake_minimum_required (VERSION 2.8.4)

project(cubicle)
set(CMAKE_CXX_FLAGS "-std=c++11")

add_subdirectory(src)
add_subdirectory(assets)
enable_testing()
add_test(run-tests src/run-tests)
