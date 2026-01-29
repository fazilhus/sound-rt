#pragma once
#include "physicsresource.h"
#include "vec3.hpp"


namespace Physics {

    struct Ray;
    struct HitInfo;
    struct ColliderMeshId;

    struct ColliderMesh {
        struct Triangle {
            union {
                glm::vec3 vertices[3];

                struct {
                    glm::vec3 v0, v1, v2;
                };
            };

            glm::vec3 center;
            glm::vec3 norm;
            bool selected = false;

            bool intersect(const Ray& r, HitInfo& hit) const;
        };

        struct Primitive {
            std::vector<Triangle> triangles;
        };

        glm::vec3 center = glm::vec3(0);
        float radius = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float depth = 0.0f;
        std::vector<glm::vec3> vertices;
        std::vector<Primitive> primitives;

        [[nodiscard]] std::size_t num_of_vertices() const;

        bool intersect(const Ray& r, HitInfo& hit) const;
        [[nodiscard]] glm::vec3 furthest_along(const glm::mat4& t, const glm::vec3& dir) const;
    };

    struct AABB {
        union {
            glm::vec3 corners[2];
            struct {
                glm::vec3 min_bound, max_bound;
            };
        };

        explicit AABB() : min_bound(max_f), max_bound(-max_f) {}

        void grow(const glm::vec3& p);
        void grow_rot(const glm::mat4& t);

        bool intersect(const Ray& r, HitInfo& hit) const;
        bool intersect(const AABB& other) const;
    };

    AABB rotate_aabb_affine(const AABB& orig, const glm::mat4& t);

    struct ColliderMeshes {
        std::vector<AABB> simple;
        std::vector<ColliderMesh> complex;
    };

    ColliderMeshes& get_collider_meshes();

    ColliderMeshId load_collider_mesh(const std::string& filepath);

} // namespace Physics