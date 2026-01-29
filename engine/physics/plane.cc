#include "config.h"
#include "plane.h"

#include "physicsresource.h"
#include "ray.h"


namespace Physics {

    bool Plane::intersect(const Ray& ray, HitInfo& hit) const  {
        if (const auto denominator = glm::dot(this->norm, ray.dir);
            abs(denominator) > epsilon_f) {
            if (const auto t = glm::dot(this->point() - ray.orig, this->norm) / denominator;
                t > epsilon_f) {
                hit.pos = ray.orig + t * ray.dir;
                return true;
                }
            }
        return false;
    }
}
