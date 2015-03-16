#include "gtest/gtest.h"
#include "pathfinding.h"
#include "glm/vec2.hpp"

using namespace std;
using namespace glm;
using namespace cubicle;

TEST(path_test, empty_test){
  ivec2 min{0,0};
  ivec2 max{5,5};
  vector<ivec2> empty_occupied;
  auto path = getPath(ivec2{0,0}, ivec2{0,0}, empty_occupied, min, max);
  EXPECT_EQ(path.size(), 0);
}

TEST(path_test, one_move){
  ivec2 min{0,0};
  ivec2 max{5,5};
  vector<ivec2> empty_occupied;
  auto path = getPath(ivec2{0,0}, ivec2{1,0}, empty_occupied, min, max);
  EXPECT_EQ(path.size(), 1);
  EXPECT_EQ(path[0].x, 1);
  EXPECT_EQ(path[0].y, 0);
}

TEST(path_test, blocked){
  ivec2 min{0,0};
  ivec2 max{5,5};
  vector<ivec2> blocked;
  blocked.push_back(ivec2{1,0});
  auto path = getPath(ivec2{0,0}, ivec2{2,0}, blocked, min, max);
  EXPECT_EQ(path.size(), 2);
  EXPECT_EQ(path[0].x, 1);
  EXPECT_EQ(path[0].y, 1);
  EXPECT_EQ(path[1].x, 2);
  EXPECT_EQ(path[1].y, 0);
}

