#include <iostream>
#include <vector>
#include <glm/vec2.hpp>

#include "pathfinding.h"

using namespace std;
using namespace glm;
using namespace cubicle;
int main() {

  ivec2 min{0,0};
  ivec2 max{5,5};
  vector<ivec2> empty_occupied;

  // empty test
  cout << "1:\n";
  auto t1 = getPath(ivec2{0,0}, ivec2{0,0}, empty_occupied, min, max);
  if(t1.size() == 0){
    cout << "success\n";
  } else {
    cout << "failure\n";
  }

  // unblocked, straight path next door
  cout << "2:\n";
  auto t2 = getPath(ivec2{0,0}, ivec2{1,0}, empty_occupied, min, max);
  for(const auto& p : t2){
    cout << p.x << "," << p.y << "\n";
  }
  if(t2.size() == 1 && 
     t2[0] == ivec2{1,0}){
    cout << "success\n";
  } else {
    cout << "failure\n";
  }

  // blocked
  auto blocked = empty_occupied;
  blocked.push_back(ivec2{1,0});
  auto t3 = getPath(ivec2{0,0}, ivec2{2,0}, blocked, min, max);
  cout << "3:\n";
  for(const auto& p : t3){
    cout << p.x << "," << p.y << "\n";
  }
  if(t3.size() == 2 && 
     t3[0] == ivec2{1,1} &&
     t3[1] == ivec2{2,0}){
    cout << "success\n";
  } else {
    cout << "failure\n";
  }

  return 0;
}

