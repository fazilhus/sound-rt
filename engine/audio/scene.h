//
// Created by fazkhu-3-local on 02/23/26.
//

#pragma once

#include <string>
#include <immintrin.h>

#include "physics/physicsresource.h"


namespace Physics {
    struct HitInfo;
    struct Ray;
}


namespace Audio {

    constexpr auto MAX_RAY_BOUNCES = 4;

    __declspec(align(64)) struct ray {
        union {
            struct {
                glm::vec3 orig;
                float dummy1;
            };
            __m128 orig4;
        };
        union {
            struct {
                glm::vec3 dir;
                float dummy2;
            };
            __m128 dir4;
        };
        union {
            struct {
                glm::vec3 inv_dir;
                float dummy3;
            };
            __m128 inv_dir4;
        };
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

    struct bvh {
        enum SplitAlgo {
            Longest = 0,
            SAH,
        };

        __declspec(align(32)) struct node {
            union {
                struct {
                    glm::vec3 bmin;
                    uint left_or_first;
                };
                __m128 bmin4;
            };
            union {
                struct {
                    glm::vec3 bmax;
                    uint tri_num;
                };
                __m128 bmax4;
            };

            bool is_leaf() const { return tri_num > 0; }

            void update_node_bounds(const std::vector<triangle>& triangles, const std::vector<uint>& indices);
            float eval_sah(const std::vector<triangle>& triangles, const std::vector<uint>& indices, int axis, float pos) const;
            float intersect_aabb(ray& r) const;
        };

        void build(const std::vector<triangle>& triangles, std::vector<uint>& indices);
        void intersect(const std::vector<triangle>& triangles, const std::vector<uint>& indices, ray& ray, Physics::HitInfo& hit);

    private:
        void subdivide(uint node_idx, const std::vector<triangle>& triangles, std::vector<uint>& indices, int depth = 0);
        float best_split_plane(const node& root_node, const std::vector<triangle>& triangles, const std::vector<uint>& indices, int& best_axis, float& best_pos) const;
        [[nodiscard]] float calc_node_cost(const node& root_node) const;

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
