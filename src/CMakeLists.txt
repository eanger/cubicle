file(GLOB SRCS *.cpp *.h)
file(GLOB test_SRCS test/*.cpp)

include_directories(.)

add_executable(cubicle ${SRCS})
set_target_properties(cubicle PROPERTIES ENABLE_EXPORTS TRUE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(SDL2 REQUIRED)
find_package(SDL2TTF REQUIRED)
find_package(SDL2IMAGE REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIR})

target_link_libraries(cubicle ${SDL2_LIBRARY} ${SDL2TTFMAIN_LIBRARY} ${SDL2IMAGE_LIBRARY})

add_subdirectory(gtest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
add_executable(run-tests ${test_SRCS})
target_link_libraries(run-tests gtest gtest_main cubicle)
