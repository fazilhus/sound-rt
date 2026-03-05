//
// Created by fazkhu-3-local on 02/23/26.
//

#pragma once

#include <string>

#include "physics/physicsresource.h"


namespace Physics {
    struct HitInfo;
    struct Ray;
}


namespace Audio {

    constexpr auto MAX_RAY_BOUNCES = 4;

    struct ray {
        glm::vec3 orig, dir, inv_dir;
        float t;

        ray(const glm::vec3& o, const glm::vec3& d) : orig(o), dir(glm::normalize(d)), inv_dir(1.f / dir), t(Physics::max_f) {}
    };

    struct triangle {
        union {
            glm::vec3 vertices[3];

            struct {
                glm::vec3 v0, v1, v2;
            };
        };

        glm::vec3 center;
        glm::vec3 normal;

        bool intersect(ray& r, Physics::HitInfo& hit) const;
    };

    constexpr auto BVH_DEPTH = 16;

    struct bvh {
        enum SplitAlgo {
            Longest = 0,
            SAH,
        };

        __declspec(align(32)) struct node {
            struct aabb {
                union {
                    glm::vec3 corners[2];
                    struct {
                        glm::vec3 min_bound, max_bound;
                    };
                };

                aabb();

                void grow(const glm::vec3& p);
                float area() const;
            };

            aabb bounds;
            uint left_or_first{0};
            uint tri_num{0};

            bool is_leaf() const { return tri_num > 0; }

            void update_node_bounds(const std::vector<triangle>& triangles, const std::vector<uint>& indices);
            float eval_sah(const std::vector<triangle>& triangles, const std::vector<uint>& indices, int axis, float pos) const;
            float intersect_aabb(ray& r) const;
        };

        void build(const std::vector<triangle>& triangles, std::vector<uint>& indices);
        void intersect(const std::vector<triangle>& triangles, const std::vector<uint>& indices, ray& ray, Physics::HitInfo& hit);

    private:
        void subdivide(uint node_idx, const std::vector<triangle>& triangles, std::vector<uint>& indices, int depth = 0);

    public:
        uint root_node_idx{0}, nodes_used{1};
        SplitAlgo algo;
        std::vector<node> nodes;
    };

    struct scene {

        void load_from(const std::string& filepath, bvh::SplitAlgo algo);
        void draw_bvh() const;

        std::vector<triangle> triangles;
        std::vector<uint> indices;
        bvh bvh_tree;
    };

} // Audio
