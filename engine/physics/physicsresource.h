#pragma once
#include "vec3.hpp"


namespace Physics {

    constexpr auto epsilon_f = std::numeric_limits<float>::epsilon();
    constexpr auto max_f = std::numeric_limits<float>::max();
    constexpr auto inf_f = std::numeric_limits<float>::infinity();

    struct ColliderId {
        uint32_t index: 22; // 4M concurrent colliders
        uint32_t generation: 10; // 1024 generations per index

        constexpr static ColliderId Create(uint32_t id) {
            ColliderId ret{id & 0x003FFFFF, (id & 0xFFC00000) >> 22};
            return ret;
        }

        static ColliderId Create(uint32_t index, uint32_t generation) {
            ColliderId ret;
            ret.index = index;
            ret.generation = generation;
            return ret;
        }

        explicit constexpr operator uint32_t() const {
            return ((generation << 22) & 0xFFC00000ul) + (index & 0x003FFFFFul);
        }

        static constexpr ColliderId Invalid() { return Create(0xFFFFFFFF); }
        constexpr uint32_t HashCode() const { return index; }
        const bool operator==(const ColliderId& rhs) const { return uint32_t(*this) == uint32_t(rhs); }
        const bool operator!=(const ColliderId& rhs) const { return uint32_t(*this) != uint32_t(rhs); }
        const bool operator<(const ColliderId& rhs) const { return index < rhs.index; }
        const bool operator>(const ColliderId& rhs) const { return index > rhs.index; }
    };

    struct ColliderMeshId {
        uint32_t index: 22; // 4M concurrent meshes
        uint32_t generation: 10; // 1024 generations per index

        constexpr static ColliderMeshId Create(uint32_t id) {
            ColliderMeshId ret{id & 0x003FFFFF, (id & 0xFFC00000) >> 22};
            return ret;
        }

        explicit constexpr operator uint32_t() const {
            return ((generation << 22) & 0xFFC00000ul) + (index & 0x003FFFFFul);
        }

        static constexpr ColliderMeshId Invalid() { return Create(0xFFFFFFFF); }
        constexpr uint32_t HashCode() const { return index; }
        const bool operator==(const ColliderMeshId& rhs) const { return uint32_t(*this) == uint32_t(rhs); }
        const bool operator!=(const ColliderMeshId& rhs) const { return uint32_t(*this) != uint32_t(rhs); }
        const bool operator<(const ColliderMeshId& rhs) const { return index < rhs.index; }
        const bool operator>(const ColliderMeshId& rhs) const { return index > rhs.index; }
    };

    struct HitInfo {
        glm::vec3 local_dir = glm::vec3(0);
        glm::vec3 local_pos = glm::vec3(0);
        glm::vec3 local_norm = glm::vec3(0);
        glm::vec3 pos = glm::vec3(0);
        glm::vec3 norm = glm::vec3(0);
        float t = max_f;
        ColliderId collider = ColliderId::Invalid();
        ColliderMeshId mesh = ColliderMeshId::Invalid();
        std::size_t prim_n;
        std::size_t tri_n;

        [[nodiscard]] bool hit() const {
            return t < max_f;
        }
    };

    struct CollisionInfo {
        glm::vec3 contact_point{0}, contact_point_a{0}, contact_point_b{0};
        glm::vec3 normal{0};
        float penetration_depth{0.0f};
        bool has_collision{false};
        ColliderId a_id, b_id;
    };

    struct SupportPoint {
        glm::vec3 point, a, b;
    };

} // namespace Physics
