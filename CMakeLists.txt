cmake_minimum_required (VERSION 2.8.4)

project(cubicle)

add_subdirectory(assets)
include_directories(.)

set(CUB_SRCS
    cubicle.cpp
    glm/vec2.hpp
    glm/geometric.hpp
    )

set(PFIND_SRCS
    pathfinding.cpp
    pathfinding.h
    glm/vec2.hpp
    glm/geometric.hpp
    )

add_library(pathfinding ${PFIND_SRCS})
add_executable(cubicle ${CUB_SRCS})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(SDL2 REQUIRED)
find_package(SDL2TTF REQUIRED)
find_package(SDL2IMAGE REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIR})

target_link_libraries(cubicle pathfinding ${SDL2_LIBRARY} ${SDL2TTFMAIN_LIBRARY} ${SDL2IMAGE_LIBRARY})

set(CMAKE_CXX_FLAGS "-std=c++11")
