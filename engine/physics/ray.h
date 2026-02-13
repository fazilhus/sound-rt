#pragma once
#include "physicsresource.h"
#include "vec3.hpp"

namespace Physics {

    constexpr auto MAX_RAY_BOUNCES = 1;

    struct Ray {
        glm::vec3 orig, dir, inv_dir;
        float length;
        float travelled;
        int bounces;

        explicit Ray(
            const glm::vec3& o, const glm::vec3& d, const bool inf_length = true, const int b = 0, const float t = 0.0f
            )
            : orig(o), dir(glm::normalize(d)), inv_dir(1.0f / dir), length(inf_length ? inf_f : glm::length(d)),
              travelled(t), bounces(b) {}
    };

} // namespace Physics