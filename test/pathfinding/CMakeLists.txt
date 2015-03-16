file(GLOB SRCS *.cpp)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}  ../../src)
add_executable(test-pathfinding ${SRCS})
target_link_libraries(test-pathfinding gtest gtest_main pathfinding)
add_test(test-pathfinding test-pathfinding)
