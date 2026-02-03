#include "config.h"

#include "phy.h"

#include "core/cvar.h"
#include "core/idpool.h"
#include "core/maths.h"
#include "physics/ray.h"
#include "physics/simplex.h"
#include "physics/physicsmesh.h"
#include "physics/physicsresource.h"
#include "render/debugrender.h"


namespace Core {
    struct CVar;
}


namespace Physics {

    static Util::IdPool<ColliderId> collider_id_pool;
    static Colliders colliders_;
    static auto sort_axis = 0;
    static Core::CVar* s_stop_sim = nullptr;

    static Simplex saved_simplex;

    State::Dyn& State::Dyn::set_pos(const glm::vec3& p) {
        this->pos = p;
        return *this;
    }

    State::Dyn& State::Dyn::set_vel(const glm::vec3& v) {
        this->vel = v;
        return *this;
    }

    State::Dyn& State::Dyn::set_rot(const glm::quat& r) {
        this->rot = r;
        return *this;
    }

    State& State::set_inertia_tensor(const glm::mat3& m) {
        this->inv_inertia_shape = m;
        return *this;
    }

    State& State::set_orig(const glm::vec3& o) {
        this->orig = o;
        return *this;
    }

    State& State::set_inv_mass(const float im) {
        this->inv_mass = im;
        return *this;
    }

    State& State::set_scale(const glm::vec3& s) {
        this->scale = s;
        return *this;
    }

    const Colliders& get_colliders() { return colliders_; }
    Colliders& colliders() { return colliders_; }


    namespace Internal {

        glm::mat3 create_inertia_tensor(const ShapeType type, const float m, const glm::vec3& scale, const ColliderMesh& cm) {
            switch (type) {
            case ShapeType::Box:
                return glm::inverse(
                    glm::mat3(
                        {
                            {1.0f / 12.0f * m * (cm.height * cm.height * scale.x * scale.x + cm.depth * cm.depth * scale.z * scale.z), 0, 0},
                            {0, 1.0f / 12.0f * m * (cm.width * cm.width * scale.y * scale.y + cm.depth * cm.depth * scale.z * scale.z), 0},
                            {0, 0, 1.0f / 12.0f * m * (cm.width * cm.width * scale.y * scale.y + cm.height * cm.height * scale.x * scale.x)}
                        }
                        )
                    );
                // case ShapeType::Sphere:
                //     return {
                //         {},
                //         {},
                //         {}
                //     };
            case ShapeType::Custom:
                return {
                        {},
                        {},
                        {}
                };
            default:
                assert(false && "unreachable");
                return {};
            }
        }

    }

    ColliderId create_rigidbody(
        ColliderMeshId cm_id, const glm::vec3& orig, const glm::vec3& translation, const glm::quat& rotation,
        const glm::vec3& scale, float mass, ShapeType type, const uint16_t mask
        ) {
        ColliderId id;
        const auto mat = glm::translate(translation) * glm::mat4(rotation) * glm::scale(scale);
        if (collider_id_pool.Allocate(id)) {
            colliders_.meshes.emplace_back(cm_id);
            colliders_.transforms.emplace_back(mat);
            colliders_.aabbs.emplace_back(get_collider_meshes().simple[cm_id.index]);
            colliders_.masks.emplace_back(mask);
            State s;
            s.set_inv_mass(1.0f / mass).set_orig(orig).set_scale(scale);
            s.set_inertia_tensor(
                Internal::create_inertia_tensor(type, mass, scale, get_collider_meshes().complex[cm_id.index])
                );
            s.dyn.set_pos(translation);
            s.dyn.set_rot(rotation);
            colliders_.states.emplace_back(s);
        }
        else {
            colliders_.meshes[id.index] = cm_id;
            colliders_.transforms[id.index] = mat;
            colliders_.aabbs[id.index] = get_collider_meshes().simple[cm_id.index];
            colliders_.masks[id.index] = mask;
            auto& s = colliders_.states[id.index];
            s.set_inv_mass(1.0f / mass).set_orig(orig).set_scale(scale);
            s.set_inertia_tensor(
                Internal::create_inertia_tensor(type, mass, scale, get_collider_meshes().complex[cm_id.index])
                );
            s.dyn.set_pos(translation);
            s.dyn.set_rot(rotation);
        }
        return id;
    }

    ColliderId create_rigidbody(
        ColliderMeshId cm_id, const glm::vec3& orig, const glm::vec3& translation, const glm::quat& rotation,
        const float scale, const float mass, const ShapeType type, const uint16_t mask
        ) {
        return create_rigidbody(cm_id, orig, translation, rotation, glm::vec3(scale), mass, type, mask);
    }

    ColliderId create_staticbody(
        ColliderMeshId cm_id, const glm::vec3& orig, const glm::vec3& translation, const glm::quat& rotation,
        const glm::vec3& scale, const uint16_t mask
        ) {
        ColliderId id;
        const auto mat = glm::translate(translation) * glm::mat4(rotation) * glm::scale(scale);
        if (collider_id_pool.Allocate(id)) {
            colliders_.meshes.emplace_back(cm_id);
            colliders_.transforms.emplace_back(mat);
            colliders_.aabbs.emplace_back(get_collider_meshes().simple[cm_id.index]);
            colliders_.masks.emplace_back(mask);
            State s;
            s.set_inv_mass(0.0f).set_orig(orig).set_scale(scale);
            s.set_inertia_tensor(
                glm::mat3(0.0f)
                );
            s.dyn.set_pos(translation);
            s.dyn.set_rot(rotation);
            colliders_.states.emplace_back(s);
        }
        else {
            colliders_.meshes[id.index] = cm_id;
            colliders_.transforms[id.index] = mat;
            colliders_.aabbs[id.index] = get_collider_meshes().simple[cm_id.index];
            colliders_.masks[id.index] = mask;
            auto& s = colliders_.states[id.index];
            s.set_inv_mass(0.0f).set_orig(orig).set_scale(scale);
            s.set_inertia_tensor(
                glm::mat3(0.0f)
                );
            s.dyn.set_pos(translation);
            s.dyn.set_rot(rotation);
        }
        return id;
    }

    ColliderId create_staticbody(
        ColliderMeshId cm_id, const glm::vec3& orig, const glm::vec3& translation, const glm::quat& rotation,
        const float scale, const uint16_t mask
        ) {
        return create_staticbody(cm_id, orig, translation, rotation, glm::vec3(scale), mask);
    }

    void set_transform(const ColliderId collider, const glm::mat4& t) {
        assert(collider_id_pool.IsValid(collider));
        colliders_.transforms[collider.index] = t;
    }

    void init_debug() {
        s_stop_sim = Core::CVarCreate(Core::CVar_Int, "s_stop_sim", "0");
    }

    bool cast_ray(const Ray& ray, HitInfo& hit, const uint16_t mask) {
        std::vector<HitInfo> aabb_hits;
        for (uint32_t i = 0; i < colliders_.meshes.size(); ++i) {
            const auto c_mask = colliders_.masks[i];
            const auto result = (mask & c_mask);
            if (c_mask != CollisionMask::None && result == 0) {
                continue;
            }
            const auto c = ColliderId(i);
            const auto cm = colliders_.meshes[i];
            const auto& aabb = colliders_.aabbs[i];
            if (HitInfo temp_hit;
                aabb.intersect(ray, temp_hit)) {
                if (ray.length != inf_f && temp_hit.t > ray.length) {
                    continue;
                }
                aabb_hits.emplace_back(temp_hit);
                aabb_hits.back().collider = c;
                aabb_hits.back().mesh = cm;
            }
        }

        if (aabb_hits.empty()) { return false; }

        std::ranges::sort(
            aabb_hits
            , [](const HitInfo& lhs, const HitInfo& rhs)-> bool { return lhs.t < rhs.t; }
            );

        HitInfo best_hit;
        for (const auto it: aabb_hits) {
            auto& cm = get_collider_meshes().complex[it.mesh.index];
            const auto& t = colliders_.transforms[it.collider.index];
            const auto inv_t = glm::inverse(t);
            const auto model_ray = Ray(
                inv_t * glm::vec4(ray.orig, 1.0f), Math::safe_normal(inv_t * glm::vec4(ray.dir, 0.0f))
                );

            for (auto& p: cm.primitives) { for (auto& tri: p.triangles) { tri.selected = false; } }

            if (HitInfo temp_hit;
                cm.intersect(model_ray, temp_hit)) {
                if (ray.length != inf_f && temp_hit.t > ray.length) {
                    continue;
                }
                if (temp_hit.t < best_hit.t) {
                    best_hit = temp_hit;
                    best_hit.collider = it.collider;
                    best_hit.mesh = it.mesh;
                    best_hit.local_dir = model_ray.dir;
                    best_hit.pos = t * glm::vec4(best_hit.local_pos, 1.0f);
                    best_hit.norm = t * glm::vec4(best_hit.local_norm, 0.0f);
                }
            }
        }

        if (best_hit.hit()) {
            hit = best_hit;
            get_collider_meshes().complex[hit.mesh.index].primitives[hit.prim_n].triangles[hit.tri_n].selected = true;
            return true;
        }

        return false;
    }

    bool cast_ray(const glm::vec3& start, const glm::vec3& dir, HitInfo& hit, const uint16_t mask) {
        return cast_ray(Ray(start, dir), hit, mask);
    }

    void add_center_impulse(const ColliderId collider, const glm::vec3& dir) {
        auto& state = colliders_.states[collider.index];
        state.dyn.impulse_accum += dir;
    }

    void add_impulse(const ColliderId collider, const glm::vec3& loc, const glm::vec3& dir) {
        auto& state = colliders_.states[collider.index];
        state.dyn.impulse_accum += dir;

        state.dyn.torque_accum += glm::cross(
            loc - state.dyn.pos,
            dir
            );
    }

    static std::vector<AABBPair> aabb_collisions;
    static std::vector<CollisionInfo> collisions_to_solve;
    static float time_acc = 0.0f;
    static float physics_step_timer = 1.0f / 60.0f;

    void step(const float dt) {
        time_acc += dt;
        if (time_acc < physics_step_timer) {
            return;
        }
        time_acc = 0.0f;
        for (std::size_t i = 0; i < colliders_.states.size(); ++i) {
            auto& state = colliders_.states[i];
            const auto rotm = glm::mat3_cast(state.dyn.rot);
            const auto inv_inertia_tensor = rotm * state.inv_inertia_shape * glm::transpose(rotm);

            state.dyn.vel += state.dyn.impulse_accum * state.inv_mass;
            state.dyn.pos += state.dyn.vel * dt;

            state.dyn.angular_vel += inv_inertia_tensor * state.dyn.torque_accum;
            state.dyn.rot = glm::normalize(state.dyn.rot + 0.5f * glm::quat(0.0f, state.dyn.angular_vel) * state.dyn.rot * dt);

            state.dyn.impulse_accum = glm::vec3(0);
            state.dyn.torque_accum = glm::vec3(0);

            colliders_.transforms[i] = glm::translate(state.dyn.pos) * glm::mat4_cast(state.dyn.rot) * glm::scale(glm::vec3(state.scale));
        }

        update_aabbs();

        for (std::size_t i = 0; i < get_colliders().states.size(); ++i) {
            add_center_impulse(ColliderId(i), gravity);
        }

        sort_and_sweep(aabb_collisions);
        collisions_to_solve.clear();
        for (const auto& [a, b]: aabb_collisions) {
            if (gjk(a, b, saved_simplex)) {
                if (const auto ci = epa(saved_simplex, a, b);
                    ci.has_collision) {
                    collisions_to_solve.push_back(ci);
                }
            }
        }


        for (const auto& ci : collisions_to_solve) {
            collision_solver(ci, ci.a_id, ci.b_id, dt);
        }
    }

    void update_aabbs() {
        for (std::size_t i = 0; i < colliders_.aabbs.size(); ++i) {
            const auto cmid = colliders_.meshes[i];
            auto& aabb = colliders_.aabbs[i];
            aabb = rotate_aabb_affine(get_collider_meshes().simple[cmid.index], colliders_.transforms[i]);
        }
    }

    void sort_and_sweep(std::vector<AABBPair>& aabb_pairs) {
        aabb_pairs.clear();
        std::vector<uint32_t> indices(colliders_.aabbs.size());
        for (uint32_t i = 0; i < indices.size(); ++i) { indices[i] = i; }

        std::ranges::sort(
            indices,
            [&](const uint32_t a, const uint32_t b)-> bool {
                const auto min_a = colliders_.aabbs[a].min_bound[sort_axis];
                const auto min_b = colliders_.aabbs[b].min_bound[sort_axis];
                return min_a < min_b;
            }
            );

        auto sum = glm::vec3(0), sum2 = glm::vec3(0);
        glm::vec3 v;
        for (std::size_t ii = 0; ii < indices.size(); ++ii) {
            const auto i = indices[ii];
            const auto a = colliders_.aabbs[i];
            const auto a_mask = colliders_.masks[i];
            const auto point = 0.5f * (a.min_bound + a.max_bound);
            sum += point;
            sum2 += point * point;

            for (std::size_t jj = ii + 1; jj < indices.size(); ++jj) {
                const auto j = indices[jj];
                const auto b = colliders_.aabbs[j];
                const auto b_mask = colliders_.masks[i];
                if (a_mask == CollisionMask::None || (a_mask & CollisionMask::Physics) != 0)
                if (b.min_bound[sort_axis] > a.max_bound[sort_axis]) {
                    break;
                }
                if (a.intersect(b)) {
                    aabb_pairs.emplace_back(AABBPair{ColliderId(i), ColliderId(j)});
                }
            }

            v = sum2 - sum * sum / static_cast<float>(indices.size());
            sort_axis = 0;
            if (v.y > v.x) sort_axis = 1;
            if (v.z > v.y) sort_axis = 2;
        }
    }

    SupportPoint support(const ColliderId a_id, const ColliderId b_id, const glm::vec3& dir) {
        const auto& collider_a = get_collider_meshes().complex[colliders_.meshes[a_id.index].index];
        const auto& collider_b = get_collider_meshes().complex[colliders_.meshes[b_id.index].index];
        const auto& transform_a = colliders_.transforms[a_id.index];
        const auto& transform_b = colliders_.transforms[b_id.index];
        const auto a = collider_a.furthest_along(transform_a, dir);
        const auto b = collider_b.furthest_along(transform_b, -dir);
        return { a - b, a, b };
    }

    bool gjk(const ColliderId a_id, const ColliderId b_id, Simplex& out_simplex) {
        auto s = support(a_id, b_id, glm::vec3(1, 0, 0));
        out_simplex = {};
        out_simplex.add_point(s);
        auto dir = -s.point;
        for (;;) {
        // for (auto i = 0; i < 64; ++i) {
            s = support(a_id, b_id, dir);
            out_simplex.add_point(s);
            if (glm::dot(out_simplex[0].point, dir) < 0.0f) { return false; }
            if (next_simplex(out_simplex, dir)) { return true; }
        }
        return false;
    }

    struct Face {
        union {
            SupportPoint v[3];
            struct {
                SupportPoint v0, v1, v2;
            };
        };
        glm::vec3 normal;
    };

    struct Edge {
        union {
            SupportPoint v[2];
            struct {
                SupportPoint v0, v1;
            };
        };
    };

    std::pair<std::vector<glm::vec4>, std::size_t> get_face_normals(
        const std::vector<SupportPoint>& polytope, const std::vector<std::size_t>& faces
        ) {
        std::vector<glm::vec4> normals;
        auto min_triangle{0ull};
        auto min_distance{max_f};

        for (size_t i = 0; i < faces.size(); i += 3) {
            const auto a = polytope[faces[i + 0]].point;
            const auto b = polytope[faces[i + 1]].point;
            const auto c = polytope[faces[i + 2]].point;

            auto normal = glm::normalize(cross(b - a, c - a));
            float distance = glm::dot(normal, a);

            if (distance < 0) {
                normal *= -1;
                distance *= -1;
            }

            normals.emplace_back(normal, distance);

            if (distance < min_distance) {
                min_triangle = i / 3;
                min_distance = distance;
            }
        }

        return {normals, min_triangle};
    }

    void add_if_unique(
        std::vector<std::pair<std::size_t, std::size_t>>& edges, const std::vector<std::size_t>& faces,
        const std::size_t a, const std::size_t b
        ) {
        const auto reverse = std::ranges::find(
            edges,
            std::make_pair(faces[b], faces[a])
            );

        if (reverse != edges.end()) { edges.erase(reverse); }
        else { edges.emplace_back(faces[a], faces[b]); }
    }

    CollisionInfo epa(const Simplex& simplex, ColliderId a_id, ColliderId b_id) {
        constexpr auto max_iterations = 64;
        constexpr auto max_faces = 64;
        constexpr auto max_edges = 32;

        std::vector<SupportPoint> polytope(simplex.begin(), simplex.end());
        std::vector<size_t> faces = {
            0, 1, 2,
            0, 3, 1,
            0, 2, 3,
            1, 3, 2
        };
        auto [normals, min_face] = get_face_normals(polytope, faces);
        auto min_norm{glm::vec3{}};
        auto min_dist{max_f};

        for (auto i{0ull}; min_dist == max_f && i < max_iterations; ++i) {
            min_norm = glm::xyz(normals[min_face]);
            min_dist = normals[min_face].w;

            const auto s = support(a_id, b_id, min_norm);
            auto s_dist{ glm::dot(min_norm, s.point) };

            if (std::abs(s_dist - min_dist) > epsilon_f) {
                min_dist = max_f;
                std::vector<std::pair<std::size_t, std::size_t>> unique_edges{};
                for (auto j{0ull}; j < normals.size(); ++j) {
                    const auto f{ j * 3 };
                    if (glm::dot(glm::vec3(normals[j]), s.point) > glm::dot(glm::vec3(normals[j]), polytope[faces[f]].point)) {
                        add_if_unique(unique_edges, faces, f + 0, f + 1);
                        add_if_unique(unique_edges, faces, f + 1, f + 2);
                        add_if_unique(unique_edges, faces, f + 2, f + 0);

                        faces[f + 2] = faces.back();
                        faces.pop_back();
                        faces[f + 1] = faces.back();
                        faces.pop_back();
                        faces[f + 0] = faces.back();
                        faces.pop_back();

                        normals[j] = normals.back();
                        normals.pop_back();

                        j--;
                    }
                }

                std::vector<std::size_t> new_faces{};
                for (const auto [e1, e2] : unique_edges) {
                    new_faces.push_back(e1);
                    new_faces.push_back(e2);
                    new_faces.push_back(polytope.size());
                }

                polytope.push_back(s);

                const auto [new_normals, new_min_face] = get_face_normals(polytope, new_faces);
                auto old_min_distance{max_f};
                for (size_t j = 0; j < normals.size(); j++) {
                    if (normals[j].w < old_min_distance) {
                        old_min_distance = normals[j].w;
                        min_face = j;
                    }
                }

                if (new_normals[new_min_face].w < old_min_distance) {
                    min_face = new_min_face + normals.size();
                }

                faces.insert(faces.end(), new_faces.begin(), new_faces.end());
                normals.insert(normals.end(), new_normals.begin(), new_normals.end());
            }
        }

        const auto& s0 = polytope[faces[3 * min_face + 0]];
        const auto& s1 = polytope[faces[3 * min_face + 1]];
        const auto& s2 = polytope[faces[3 * min_face + 2]];
        const auto a = s0.point;
        const auto b = s1.point;
        const auto c = s2.point;

        const auto ab = b - a;
        const auto ac = c - a;
        const auto ao =   - a;

        const auto abab = glm::dot(ab, ab);
        const auto abac = glm::dot(ab, ac);
        const auto acac = glm::dot(ac, ac);
        const auto aoab = glm::dot(ao, ab);
        const auto aoac = glm::dot(ao, ac);

        const auto denom = abac * abac - abab * acac;
        const auto s = (abac * aoac - acac * aoab) / denom;
        const auto t = (abac * aoab - abab * aoac) / denom;
        const auto r = 1.0f - s - t;

        CollisionInfo ret{};
        ret.contact_point_a = r * s0.a + s * s1.a + t * s2.a;
        ret.contact_point_b = r * s0.b + s * s1.b + t * s2.b;
        ret.contact_point = 0.5f * (ret.contact_point_a + ret.contact_point_b);
        ret.normal = -min_norm;
        ret.penetration_depth = min_dist;
        ret.has_collision = true;
        ret.a_id = a_id;
        ret.b_id = b_id;

        return ret;
    }

    void collision_solver(const CollisionInfo& collision_info, const ColliderId a_id, const ColliderId b_id, const float dt) {
        constexpr auto rest = 0.6f;
        constexpr auto bais_factor = 0.25f;
        constexpr auto slop = 0.001f;
        auto& a_state = colliders_.states[a_id.index];
        auto& b_state = colliders_.states[b_id.index];
        const auto& a_t = colliders_.transforms[a_id.index];
        const auto& b_t = colliders_.transforms[b_id.index];

        const auto a_r = collision_info.contact_point - glm::vec3(a_t * glm::vec4(a_state.orig, 1.0f));
        const auto b_r = collision_info.contact_point - glm::vec3(b_t * glm::vec4(b_state.orig, 1.0f));

        const auto a_vel = a_state.dyn.vel + glm::cross(a_state.dyn.angular_vel, a_r);
        const auto b_vel = b_state.dyn.vel + glm::cross(b_state.dyn.angular_vel, b_r);

        const auto tnorm = collision_info.normal;
        const auto rel_vel = glm::dot(tnorm, (a_vel - b_vel));
        if (rel_vel > epsilon_f) return;

        const auto a_rotm = glm::mat3_cast(a_state.dyn.rot);
        const auto b_rotm = glm::mat3_cast(b_state.dyn.rot);
        const auto a_inv_inertia = a_rotm * a_state.inv_inertia_shape * glm::transpose(a_rotm);
        const auto b_inv_inertia = b_rotm * b_state.inv_inertia_shape * glm::transpose(b_rotm);
        const auto a_rn = glm::cross(a_r, tnorm);
        const auto b_rn = glm::cross(b_r, tnorm);
        const auto a_term = a_state.inv_mass + glm::dot(a_rn, a_inv_inertia * a_rn);
        const auto b_term = b_state.inv_mass + glm::dot(b_rn, b_inv_inertia * b_rn);
        const auto bais = bais_factor / dt * Math::max(0.0f, collision_info.penetration_depth - slop);
        const auto a_j = (-(1.0f + rest) * rel_vel + bais) / (a_term + b_term);
        const auto b_j = -a_j;

        if (a_state.inv_mass != 0.0f) {
            const auto a_impulse = a_j * tnorm;
            const auto a_angular_impulse = glm::cross(a_r, a_impulse);
            a_state.dyn.impulse_accum += a_impulse;
            a_state.dyn.torque_accum += a_angular_impulse;
        }
        if (b_state.inv_mass != 0.0f) {
            const auto b_impulse = b_j * tnorm;
            const auto b_angular_impulse = glm::cross(b_r, b_impulse);
            b_state.dyn.impulse_accum += b_impulse;
            b_state.dyn.torque_accum += b_angular_impulse;
        }
    }

} // namespace Physics