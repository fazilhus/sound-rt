//
// Created by fazkhu-3-local on 02/23/26.
//

#include "config.h"
#include "scene.h"

#include <iostream>

#include "audio_manager.h"
#include "fx/gltf.h"

#include "physics/physicsresource.h"
#include "physics/ray.h"
#include "core/maths.h"
#include "render/debugrender.h"

// #define USE_SSE;

namespace Audio {
    namespace Internal {
        template <typename CompType>
        void load_scene(scene& s, const fx::gltf::Document& doc, const fx::gltf::Primitive& prim) {
            const auto& ib_access = doc.accessors[prim.indices];
            const auto& ib_view = doc.bufferViews[ib_access.bufferView];
            const auto& ib = doc.buffers[ib_view.buffer];

            const auto& vb_access = doc.accessors[prim.attributes.find("POSITION")->second];
            const auto& vb_view = doc.bufferViews[vb_access.bufferView];
            const auto& vb = doc.buffers[vb_view.buffer];

            const auto num_indices = ib_access.count;
            const auto ibuf = reinterpret_cast<const CompType*>(&ib.data[ib_access.byteOffset + ib_view.byteOffset]);
            const auto vbuf = reinterpret_cast<const float*>(&vb.data[vb_access.byteOffset + vb_view.byteOffset]);

            const auto dim = (vb_access.type == fx::gltf::Accessor::Type::Vec3) ? 3 : 4;

            s.triangles.reserve(s.triangles.size() + num_indices / 3);

            for (uint32_t i = 0; i < num_indices; i += 3) {
                triangle t;
                t.v0 = glm::vec3(
                    vbuf[dim * ibuf[i]],
                    vbuf[dim * ibuf[i] + 1],
                    vbuf[dim * ibuf[i] + 2]
                    );
                t.v1 = glm::vec3(
                    vbuf[dim * ibuf[i + 1]],
                    vbuf[dim * ibuf[i + 1] + 1],
                    vbuf[dim * ibuf[i + 1] + 2]
                    );
                t.v2 = glm::vec3(
                    vbuf[dim * ibuf[i + 2]],
                    vbuf[dim * ibuf[i + 2] + 1],
                    vbuf[dim * ibuf[i + 2] + 2]
                    );

                t.center = (t.v0 + t.v1 + t.v2) * 0.3333f;
                t.normal = glm::cross(
                    t.v1 - t.v0,
                    t.v2 - t.v0
                    );
                s.triangles.emplace_back(t);
            }

        }
    }


    bool triangle::intersect(ray& r, Physics::HitInfo& hit) const {
        const auto edge1 = this->v1 - this->v0;
        const auto edge2 = this->v2 - this->v0;
        const auto ray_cross_e2 = glm::cross(r.dir, edge2);
        const auto det = glm::dot(edge1, ray_cross_e2);

        if (det > -Physics::epsilon_f && det < Physics::epsilon_f) { return false; }

        const auto inv_det = 1.0f / det;
        const auto ray_to_v0 = r.orig - this->v0;
        const auto u = inv_det * glm::dot(ray_to_v0, ray_cross_e2);
        if (u < 0.0f || u > 1.0f) { return false; }

        const auto ray_to_v0_cross_e1 = glm::cross(ray_to_v0, edge1);
        const auto v = inv_det * glm::dot(r.dir, ray_to_v0_cross_e1);
        if (v < 0.0f || u + v > 1.0f) { return false; }

        const auto t = inv_det * glm::dot(edge2, ray_to_v0_cross_e1);
        if (t > Physics::epsilon_f && t < hit.t) {
            r.t = t;
            hit.t = t;
            hit.pos = r.orig + r.dir * hit.t;
            hit.norm = this->normal;
            return true;
        }
        return false;
    }

    aabb::aabb() {
        this->min_bound = glm::vec3(Physics::max_f);
        this->max_bound = glm::vec3(-Physics::max_f);
    }

    void aabb::grow(const glm::vec3& p) {
        this->min_bound = glm::min(this->min_bound, p);
        this->max_bound = glm::max(this->max_bound, p);
    }

    float aabb::area() const {
        const auto e = max_bound - min_bound;
        return e.x * e.x + e.y * e.y + e.z * e.z;
    }

    void bvh::node::update_node_bounds(const std::vector<triangle>& triangles, const std::vector<uint>& indices) {
        for (auto i = 0; i < this->tri_num; ++i) {
            const auto tri_idx = indices[this->left_or_first + i];
            const auto& tri = triangles[tri_idx];
            this->bmin = glm::min(this->bmin, tri.v0);
            this->bmax = glm::max(this->bmax, tri.v0);
            this->bmin = glm::min(this->bmin, tri.v1);
            this->bmax = glm::max(this->bmax, tri.v1);
            this->bmin = glm::min(this->bmin, tri.v2);
            this->bmax = glm::max(this->bmax, tri.v2);
        }
    }

    float bvh::node::eval_sah(
        const std::vector<triangle>& triangles, const std::vector<uint>& indices, const int axis, const float pos
        ) const {
        aabb left, right;
        auto left_c{0}, right_c{0};
        for (auto i = 0; i < this->tri_num; ++i) {
            const auto& tri = triangles[indices[this->left_or_first + i]];
            if (tri.center[axis] < pos) {
                left_c++;
                left.grow(tri.v0);
                left.grow(tri.v1);
                left.grow(tri.v2);
            } else {
                right_c++;
                right.grow(tri.v0);
                right.grow(tri.v1);
                right.grow(tri.v2);
            }
        }
        const auto cost = left_c * left.area() + right_c * right.area();
        return cost > 0 ? cost : Physics::max_f;
    }

    float bvh::node::intersect_aabb(ray& r) const {
#ifdef USE_SSE
        static __m128 mask4 = _mm_cmpeq_ps( _mm_setzero_ps(), _mm_set_ps( 1, 0, 0, 0 ) );
        const __m128 t1 = _mm_mul_ps( _mm_sub_ps( _mm_and_ps( bmin4, mask4 ), r.orig4 ), r.inv_dir4 );
        const __m128 t2 = _mm_mul_ps( _mm_sub_ps( _mm_and_ps( bmax4, mask4 ), r.orig4 ), r.inv_dir4 );
        const __m128 vmax4 = _mm_max_ps( t1, t2 );
        const __m128 vmin4 = _mm_min_ps( t1, t2 );
        const float tmax = Math::min(vmax4.m128_f32[0], vmax4.m128_f32[1], vmax4.m128_f32[2]);
        if (const float tmin = Math::max(vmin4.m128_f32[0], vmin4.m128_f32[1], vmin4.m128_f32[2]);
            tmax >= tmin && tmin < r.t && tmax > 0) return tmin;
        return Physics::max_f;
#else
        auto tmin{0.0f}, tmax{Physics::max_f};

        for (auto i = 0; i < 3; ++i) {
            const auto dmin = (bmin[i] - r.orig[i]) * r.inv_dir[i];
            const auto dmax = (bmax[i] - r.orig[i]) * r.inv_dir[i];
            tmin = Math::max(tmin, Math::min(dmin, dmax, tmax));
            tmax = Math::min(tmax, Math::max(dmin, dmax, tmin));
        }
        if (tmax >= tmin && tmin < r.t && tmax > 0) return tmin;
        return Physics::max_f;
#endif
    }

    void bvh::build(const std::vector<triangle>& triangles, std::vector<uint>& indices) {
        nodes.resize(2 << BVH_DEPTH);
        auto& root = nodes[this->root_node_idx];
        root.left_or_first = 0;
        root.tri_num = indices.size();
        root.update_node_bounds(triangles, indices);
        subdivide(this->root_node_idx, triangles, indices);
    }

    void bvh::intersect(
        const std::vector<triangle>& triangles, const std::vector<uint>& indices, ray& ray,
        Physics::HitInfo& hit
        ) {

        auto* current = &this->nodes[0];
        node* stack[64];
        uint stack_ptr{0};

        for (;;) {
            if (current->is_leaf()) {
                for (auto i = 0; i < current->tri_num; ++i) {
                    const auto tri_idx = indices[current->left_or_first + i];
                    if (const auto& tri = triangles[tri_idx];
                        tri.intersect(ray, hit)) {
                        hit.tri_n = tri_idx;
                    }
                }
                if (stack_ptr == 0) {
                    break;
                }

                current = stack[--stack_ptr];
                continue;
            }

            auto c1 = &this->nodes[current->left_or_first];
            auto c2 = &this->nodes[current->left_or_first + 1];
            auto dist1 = c1->intersect_aabb(ray);
            auto dist2 = c2->intersect_aabb(ray);
            if (dist1 > dist2) {
                std::swap(dist1, dist2);
                std::swap(c1, c2);
            }
            if (dist1 == Physics::max_f) {
                if (stack_ptr == 0) {
                    break;
                }

                current = stack[--stack_ptr];
            } else {
                current = c1;
                if (dist2 != Physics::max_f) {
                    stack[stack_ptr++] = c2;
                }
            }
        }
    }

    void bvh::subdivide(const uint node_idx, const std::vector<triangle>& triangles, std::vector<uint>& indices, const int depth) {
        auto& root_node = this->nodes[node_idx];
        if (root_node.tri_num <= 2 || depth >= BVH_DEPTH) return;

        auto best_axis{0};
        auto best_pos{0.0f};
        switch (this->algo) {
        case Longest: {
            const auto extent = root_node.bmax - root_node.bmin;
            if (extent.y > extent.x) { best_axis = 1; }
            if (extent.z > extent[best_axis]) { best_axis = 2; }
            best_pos = root_node.bmin[best_axis] + extent[best_axis] * 0.5f;
        } break;
        case SAH: {
            const auto best_cost = best_split_plane(root_node, triangles, indices, best_axis, best_pos);
            const auto parent_cost = calc_node_cost(root_node);
            if (best_cost >= parent_cost) { return; }
        } break;
        }

        auto i = root_node.left_or_first;
        auto j = i + root_node.tri_num - 1;
        while (i <= j) {
            if (triangles[indices[i]].center[best_axis] < best_pos) {
                i++;
            } else {
                std::swap(indices[i], indices[j--]);
            }
        }

        const auto left_count = i - root_node.left_or_first;
        if (left_count == 0 || left_count == root_node.tri_num) return;

        const auto left_i = nodes_used++;
        const auto right_i = nodes_used++;
        auto& left_c = this->nodes[left_i];
        left_c.left_or_first = root_node.left_or_first;
        left_c.tri_num = left_count;
        auto& right_c = this->nodes[right_i];
        right_c.left_or_first = i;
        right_c.tri_num = root_node.tri_num - left_count;
        root_node.left_or_first = left_i;
        root_node.tri_num = 0;
        left_c.update_node_bounds(triangles, indices);
        right_c.update_node_bounds(triangles, indices);
        subdivide(left_i, triangles, indices, depth + 1);
        subdivide(right_i, triangles, indices, depth + 1);
    }

    float bvh::best_split_plane(
        const node& root_node, const std::vector<triangle>& triangles, const std::vector<uint>& indices, int& best_axis, float& best_pos
        ) const {
        auto best_cost{Physics::max_f};
        for (auto axis = 0; axis < 3; ++axis) {
            // for (auto i = 0; i < root_node.tri_num; ++i) {
            //     const auto& tri = triangles[indices[root_node.left_or_first + i]];
            //     const auto temp_pos = tri.center[axis];
            //     if (const auto temp_cost = root_node.eval_sah(triangles, indices, axis, temp_pos);
            //         temp_cost < best_cost) {
            //         best_pos = temp_pos;
            //         best_cost = temp_cost;
            //         best_axis = axis;
            //     }
            // }
            const auto bmin = root_node.bmin[axis];
            const auto bmax = root_node.bmax[axis];
            if (bmin == bmax) continue;
            const auto scale = (bmax - bmin) / 100.0f;
            for (auto i = 1; i < 100; ++i) {
                const auto temp_pos = bmin + scale * i;
                if (const auto temp_cost = root_node.eval_sah(triangles, indices, axis, temp_pos);
                    temp_cost < best_cost) {
                    best_pos = temp_pos;
                    best_cost = temp_cost;
                    best_axis = axis;
                }
            }
        }
        return best_cost;
    }

    float bvh::calc_node_cost(const node& root_node) const {
        const auto pe = root_node.bmax - root_node.bmin;
        const auto parent_area = pe.x * pe.x + pe.y * pe.y + pe.z * pe.z;
        return static_cast<float>(root_node.tri_num) * parent_area;
    }

    void scene::load_from(const std::string& filepath, const bvh::SplitAlgo algo) {
        fx::gltf::Document doc;
        try {
            if (filepath.ends_with("glb")) { doc = fx::gltf::LoadFromBinary(filepath); }
            else { doc = fx::gltf::LoadFromText(filepath); }
        }
        catch (const std::exception& err) {
            printf(err.what());
#if _DEBUG
            assert(false);
#endif
        }

        for (const auto& mesh : doc.meshes) {
            switch (const auto& prim = mesh.primitives[0];
                doc.accessors[prim.indices].componentType) {
                case fx::gltf::Accessor::ComponentType::Byte:
                Internal::load_scene<int8_t>(*this, doc, prim);
                break;
                case fx::gltf::Accessor::ComponentType::UnsignedByte:
                Internal::load_scene<uint8_t>(*this, doc, prim);
                break;
                case fx::gltf::Accessor::ComponentType::Short:
                Internal::load_scene<int16_t>(*this, doc, prim);
                break;
                case fx::gltf::Accessor::ComponentType::UnsignedShort:
                Internal::load_scene<uint16_t>(*this, doc, prim);
                break;
                case fx::gltf::Accessor::ComponentType::UnsignedInt:
                Internal::load_scene<uint32_t>(*this, doc, prim);
                break;
                default:
                assert(false); // not supported
                }
        }

        indices.resize(triangles.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i] = i;
        }

        bvh_tree.algo = algo;
        bvh_tree.build(triangles, indices);
    }

    void scene::draw_bvh() const {
#if _DEBUG
        for (auto i = 0; i < this->bvh_tree.nodes_used; ++i) {
            const auto n = this->bvh_tree.nodes[i];
            Debug::DrawBox(
                0.5f * (n.bmax + n.bmin),
                glm::quat(),
                n.bmax.x - n.bmin.x,
                n.bmax.y - n.bmin.y,
                n.bmax.z - n.bmin.z,
                glm::vec4(0, 1, 0, 1),
                Debug::WireFrame,
                2.0f
                );
        }
#endif
    }

} // Audio