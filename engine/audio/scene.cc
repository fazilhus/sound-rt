//
// Created by fazkhu-3-local on 02/23/26.
//

#include "config.h"
#include "scene.h"

#include "fx/gltf.h"

#include "physics/physicsresource.h"
#include "physics/ray.h"
#include "core/maths.h"
#include "render/debugrender.h"


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


    bool triangle::intersect(const Physics::Ray& r, Physics::HitInfo& hit) const {
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
        if (t > Physics::epsilon_f) {
            hit.t = Math::min(t, hit.t);
            hit.pos = r.orig + r.dir * hit.t;
            hit.norm = this->normal;
            return true;
        }
        return false;
    }

    void bvh::node::update_node_bounds(const std::vector<triangle>& triangles, const std::vector<uint>& indices) {
        this->min_bound = glm::vec3(Physics::inf_f);
        this->max_bound = glm::vec3(-Physics::inf_f);

        for (auto i = 0; i < this->tri_num; ++i) {
            const auto tri_idx = indices[this->left_or_first + i];
            const auto& tri = triangles[tri_idx];
            this->min_bound = glm::min(this->min_bound, tri.v0);
            this->min_bound = glm::min(this->min_bound, tri.v1);
            this->min_bound = glm::min(this->min_bound, tri.v2);
            this->max_bound = glm::max(this->max_bound, tri.v0);
            this->max_bound = glm::max(this->max_bound, tri.v1);
            this->max_bound = glm::max(this->max_bound, tri.v2);
        }
    }

    bool bvh::node::intersect_aabb(const Physics::Ray& r) const {
        auto tmin{0.0f}, tmax{Physics::max_f};

        for (auto i = 0; i < 3; ++i) {
            const auto sign = signbit(r.inv_dir[i]);
            const auto bmin = this->corners[sign][i];
            const auto bmax = this->corners[!sign][i];
            const auto dmin = (bmin - r.orig[i]) * r.inv_dir[i];
            const auto dmax = (bmax - r.orig[i]) * r.inv_dir[i];
            tmin = Math::max(tmin, Math::min(dmin, dmax, tmax));
            tmax = Math::min(tmax, Math::max(dmin, dmax, tmin));
        }
        return tmin < tmax;
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
        const std::vector<triangle>& triangles, std::vector<uint>& indices, const Physics::Ray& ray,
        Physics::HitInfo& hit, const uint node_idx
        ) {
        const auto& root_node = this->nodes[node_idx];
        if (!root_node.intersect_aabb(ray)) return;

        if (root_node.is_leaf()) {
            for (auto i = 0; i < root_node.tri_num; ++i) {
                const auto& tri = triangles[indices[root_node.left_or_first + i]];
                tri.intersect(ray, hit);
            }
        } else {
            intersect(triangles, indices, ray, hit, root_node.left_or_first);
            intersect(triangles, indices, ray, hit, root_node.left_or_first + 1);
        }
    }

    void bvh::subdivide(const uint node_idx, const std::vector<triangle>& triangles, std::vector<uint>& indices) {
        auto& root_node = this->nodes[node_idx];
        if (root_node.tri_num <= 2) return;

        const auto extent = root_node.max_bound - root_node.min_bound;
        auto axis = 0;
        if (extent.y > extent.x) { axis = 1; }
        if (extent.z > extent[axis]) { axis = 2; }
        const auto split = root_node.min_bound[axis] + extent[axis] * 0.5f;

        auto i = root_node.left_or_first;
        auto j = i + root_node.tri_num - 1;
        while (i <= j) {
            if (triangles[indices[i]].center[axis] < split) {
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
        subdivide(left_i, triangles, indices);
        subdivide(right_i, triangles, indices);
    }

    void scene::load_from(const std::string& filepath) {
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

        bvh_tree.build(triangles, indices);
    }

    void scene::draw_bvh() const {
#if _DEBUG
        for (auto i = 0; i < this->bvh_tree.nodes_used; ++i) {
            const auto n = this->bvh_tree.nodes[i];
            Debug::DrawBox(
                0.5f * (n.max_bound + n.min_bound),
                glm::quat(),
                n.max_bound.x - n.min_bound.x,
                n.max_bound.y - n.min_bound.y,
                n.max_bound.z - n.min_bound.z,
                glm::vec4(0, 1, 0, 1),
                Debug::WireFrame,
                2.0f
                );
        }
#endif
    }

} // Audio