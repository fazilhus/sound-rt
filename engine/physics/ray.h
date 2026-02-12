#pragma once
#include "physicsresource.h"
#include "vec3.hpp"

namespace Physics {

    constexpr auto MAX_RAY_BOUNCES = 1;

    struct Ray {
        glm::vec3 orig, dir, inv_dir;
        float length;
        int bounces;

        explicit Ray(const glm::vec3& o, const glm::vec3& d, const bool inf_length = true)
            : orig(o), dir(glm::normalize(d)), inv_dir(1.0f / dir), length(inf_f), bounces(0) {
            if (!inf_length)
                length = glm::length(d);
        }
    };

} // namespace Physics