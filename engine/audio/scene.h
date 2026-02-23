//
// Created by fazkhu-3-local on 02/23/26.
//

#pragma once


namespace Physics {
    struct HitInfo;
    struct Ray;
}


namespace Audio {

    struct triangle {
        union {
            glm::vec3 vertices[3];

            struct {
                glm::vec3 v0, v1, v2;
            };
        };

        glm::vec3 center;
        glm::vec3 normal;

        bool intersect(const Physics::Ray& r, Physics::HitInfo& hit) const;
    };

    constexpr auto BVH_DEPTH = 16;

    struct bvh {
        __declspec(align(32)) struct node {
            union {
                glm::vec3 corners[2];
                struct {
                    glm::vec3 min_bound, max_bound;
                };
            };
            uint left_or_first{0};
            uint tri_num{0};

            bool is_leaf() const { return tri_num > 0; }

            void update_node_bounds(const std::vector<triangle>& triangles, const std::vector<uint>& indices);
            bool intersect_aabb(const Physics::Ray& r) const;
        };

        void build(const std::vector<triangle>& triangles, std::vector<uint>& indices);
        void intersect(const std::vector<triangle>& triangles, std::vector<uint>& indices, const Physics::Ray& ray, Physics::HitInfo& hit, uint node_idx = 0);

    private:
        void subdivide(uint node_idx, const std::vector<triangle>& triangles, std::vector<uint>& indices);

    public:
        uint root_node_idx{0}, nodes_used{1};
        std::vector<node> nodes;
    };

    struct scene {
        void load_from(const std::string& filepath);
        void draw_bvh() const;

        std::vector<triangle> triangles;
        std::vector<uint> indices;
        bvh bvh_tree;
    };

} // Audio
