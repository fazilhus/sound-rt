#pragma once
#include "vec3.hpp"
#include "vec4.hpp"


namespace Physics {
    struct Ray;
    struct HitInfo;

    struct Plane {
        glm::vec3 norm;
        float dist;

        explicit Plane(const glm::vec3& point, const glm::vec3& norm)
            : norm(norm), dist(glm::length(point)) {}

        Plane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
            const auto v1 = a - b;
            const auto v2 = a - c;
            norm = glm::cross(v1, v2);
            dist = glm::length(a * norm);
        }

        [[nodiscard]] glm::vec3 point() const { return norm * dist; }

        bool intersect(const Ray& ray, HitInfo& hit) const;
    };

} // namespace Physics
