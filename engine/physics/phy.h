#pragma once
#include "physicsresource.h"


namespace Physics {

    struct Ray;
    struct Simplex;

    enum class ShapeType : uint8_t {
        Box = 0,
        // Sphere,
        Custom,
    };

    namespace CollisionMask {
        enum : uint16_t {
            None = 1 << 0,
            Physics = 1 << 1,
            Audio = 1 << 2,
            All = (1 << 16) - 1,
        };
    }

    struct State {
        struct Dyn {
            glm::vec3 pos = glm::vec3(0);
            glm::vec3 vel = glm::vec3(0);
            glm::quat rot = glm::quat();
            glm::vec3 angular_vel = glm::vec3(0);
            glm::vec3 impulse_accum = glm::vec3(0);
            glm::vec3 torque_accum = glm::vec3(0);

            Dyn& set_pos(const glm::vec3& p);
            Dyn& set_vel(const glm::vec3& v);
            Dyn& set_rot(const glm::quat& r);
        };

        Dyn dyn;
        glm::mat3 inv_inertia_shape = glm::mat3(0);
        glm::vec3 orig = glm::vec3(0);
        glm::vec3 scale = glm::vec3(1.0f);
        float inv_mass = 1.0f;

        State& set_inertia_tensor(const glm::mat3& m);
        State& set_orig(const glm::vec3& o);
        State& set_inv_mass(float im);
        State& set_scale(const glm::vec3& s);
    };

    struct AABB;

    struct Colliders {
        std::vector<ColliderMeshId> meshes;
        std::vector<AABB> aabbs;
        std::vector<glm::mat4> transforms;
        std::vector<State> states;
        std::vector<uint16_t> masks;
    };

    constexpr auto gravity = glm::vec3(0, -9.81f, 0);

    const Colliders& get_colliders();
    Colliders& colliders();

    ColliderId create_rigidbody(
        ColliderMeshId cm_id, const glm::vec3& orig, const glm::vec3& translation, const glm::quat& rotation,
        const glm::vec3& scale = glm::vec3(1.0f), float mass = 1.0f, ShapeType type = ShapeType::Box,
        uint16_t mask = 0
        );
    ColliderId create_rigidbody(
        ColliderMeshId cm_id, const glm::vec3& orig, const glm::vec3& translation, const glm::quat& rotation,
        float scale = 1.0f, float mass = 1.0f, ShapeType type = ShapeType::Box, uint16_t mask = 0
        );
    ColliderId create_staticbody(
        ColliderMeshId cm_id, const glm::vec3& orig, const glm::vec3& translation, const glm::quat& rotation,
        const glm::vec3& scale = glm::vec3(1.0f), uint16_t mask = 0
        );
    ColliderId create_staticbody(
        ColliderMeshId cm_id, const glm::vec3& orig, const glm::vec3& translation, const glm::quat& rotation,
        float scale = 1.0f, uint16_t mask = 0
        );
    void set_transform(ColliderId collider, const glm::mat4& t);

    void init_debug();

    bool cast_ray(const Ray& ray, HitInfo& hit, uint16_t mask = CollisionMask::All);
    bool cast_ray(const glm::vec3& start, const glm::vec3& dir, HitInfo& hit, uint16_t mask = CollisionMask::All);

    void add_center_impulse(ColliderId collider, const glm::vec3& dir);
    void add_impulse(ColliderId collider, const glm::vec3& loc, const glm::vec3& dir);
    void step(float dt);
    void update_aabbs();

    struct AABBPair {
        ColliderId a, b;
    };

    void sort_and_sweep(std::vector<AABBPair>& aabb_pairs);

    bool gjk(ColliderId a_id, ColliderId b_id, Simplex& out_simplex);
    CollisionInfo epa(const Simplex& simplex, ColliderId a_id, ColliderId b_id);
    void collision_solver(const CollisionInfo& collision_info, ColliderId a_id, ColliderId b_id, float dt);

} // namespace Physics
