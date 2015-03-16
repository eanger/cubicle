#pragma once

#include <vector>
#include <glm/vec2.hpp>

namespace cubicle {
/* assumptions:
 *    target isn't occupied
 *    min/max is open/closed: [min,max)
 */
std::vector<glm::ivec2> getPath(
    glm::ivec2 start, glm::ivec2 target,
    const std::vector<glm::ivec2>& occupied_locations,
    glm::ivec2 min_corner, glm::ivec2 max_corner);
}
