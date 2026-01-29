#pragma once
#include "vec3.hpp"

namespace Physics {

    struct Ray {
        glm::vec3 orig, dir, inv_dir;

        explicit Ray(const glm::vec3& orig, const glm::vec3& dir) : orig(orig), dir(dir), inv_dir(1.0f / dir) {
        }
    };

} // namespace Physics