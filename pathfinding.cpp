#include <algorithm>
#include <cstdlib>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "pathfinding.h"

using namespace std;
using namespace glm;

namespace {
struct PathNode {
  ivec2 pos;
  PathNode* parent;
  int heuristic;
  int movement_cost;
  bool operator>(const PathNode& p) const {
    return heuristic + movement_cost > p.heuristic + p.movement_cost;
  }
  bool operator==(const PathNode& p) const {
    return pos == p.pos;
  }
  PathNode(ivec2 pos, int movement_cost, ivec2 target, PathNode* parent)
      : pos{pos}, movement_cost{movement_cost}, parent{parent} {
    // calculate heuristic cost
    // TODO: might want to multiply everything by 10 to get better precision with integer costs
    heuristic = 10 * (abs(target.x - pos.x) + abs(target.y - pos.y));
  }
};

// TODO might require less naive version of sorted insert. It would be more
// like implementing our own priority queue, but iteratable
void addNeighbor(vector<PathNode>& open, const PathNode& neighbor) {
  auto elem = begin(open);
  for(; elem != end(open); ++elem){
    if(neighbor > *elem) {
      break;
    }
  }
  open.insert(elem, neighbor);
}

// pair of position and movement cost
const vector<pair<ivec2,int>> possible_positions = {
  {{-1,-1}, 14}, {{0 ,-1}, 10}, {{1 ,-1}, 14},
  {{-1, 0}, 10}, /*cur*/        {{1 , 0}, 10},
  {{-1, 1}, 14}, {{0 , 1}, 10}, {{1 , 1}, 14}};


}

namespace cubicle {

vector<ivec2> getPath(ivec2 start, ivec2 target,
                      const vector<ivec2>& occupied_locations,
                      ivec2 min_corner, ivec2 max_corner) {
  vector<ivec2> path;
  if(start == target) {
    return path;
  }

  // the open list is ordered by DECREASING cost (movement_cost + heuristic)
  vector<PathNode> open, closed;

  open.emplace_back(start, 0, target, nullptr);
  bool done = false;
  PathNode* final_node = nullptr;
  while(!final_node && !open.empty()){
    closed.push_back(open.back());
    auto& current_node = closed.back();
    open.pop_back();
    for(const auto& p : possible_positions){
      auto& pos = p.first;
      auto& cost = p.second;
      auto neighbor_pos = current_node.pos + pos;
      // neighbor is the target, so we're done
      if(neighbor_pos == target){
        final_node = &current_node;
        break;
      }
      // check for reachability
      if (find(begin(occupied_locations),
               end(occupied_locations),
               neighbor_pos) != end(occupied_locations)) {
        continue;
      }
      PathNode neighbor(neighbor_pos, cost, target, &current_node);
      // check if already on closed list
      auto closed_elem = find(begin(closed), end(closed), neighbor);
      if(closed_elem != end(closed)){
        continue;
      }
      // check if already on open list
      auto open_elem = find(begin(open), end(open), neighbor);
      if(open_elem != end(open)){
        // check if this path is lower cost
        if(open_elem->movement_cost > current_node.movement_cost + cost) {
          open.erase(open_elem);
          neighbor.movement_cost =
              current_node.movement_cost + cost;
        } else {
          continue;
        }
      }
      addNeighbor(open, neighbor);
    }
  }
  // add target to last pos in path
  path.push_back(target);
  // reconstruct path
  while(final_node && final_node->parent){
    path.push_back(final_node->pos);
    final_node = final_node->parent;
  }
  reverse(begin(path), end(path));
  return path;
}

}
