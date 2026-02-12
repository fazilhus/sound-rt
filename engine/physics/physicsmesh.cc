#include "config.h"
#include "physicsmesh.h"

#include <set>

#include "plane.h"
#include "ray.h"
#include "core/idpool.h"
#include "core/maths.h"
#include "fx/gltf.h"


namespace Physics {

    static Util::IdPool<ColliderMeshId> collider_mesh_id_pool;
    static ColliderMeshes collider_meshes;


    namespace Internal {

        template <typename CompType>
        void LoadColliderMeshPrimitive(
            const fx::gltf::Document& doc, AABB* aabb, ColliderMesh* mesh, std::size_t prim_n
            ) {
            const auto& prim = doc.meshes[0].primitives[prim_n];

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
            for (uint32_t i = 0; i < num_indices; i += 3) {
                ColliderMesh::Triangle t;
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

                t.center = (t.v0 + t.v1 + t.v2) / 3.0f;
                t.norm = glm::cross(
                    t.v1 - t.v0,
                    t.v2 - t.v0
                    );
                mesh->primitives[prim_n].triangles.emplace_back(t);
            }

            std::set<CompType> unique_vertices;
            for (uint32_t i = 0; i < ib_access.count; ++i) {
                if (!unique_vertices.contains(dim * ibuf[i])) {
                    unique_vertices.insert(dim * ibuf[i]);
                }
            }

            for (auto it = unique_vertices.begin(); it != unique_vertices.end(); it = std::next(it, 3)) {
                const auto vertex = glm::vec3(
                        vbuf[*it],
                        vbuf[*it + 1],
                        vbuf[*it + 2]
                        );
                mesh->vertices.emplace_back(vertex);
                mesh->center += vertex;
            }

            aabb->grow(glm::vec3(vb_access.min[0], vb_access.min[1], vb_access.min[2]));
            aabb->grow(glm::vec3(vb_access.max[0], vb_access.max[1], vb_access.max[2]));

            mesh->radius = Math::max(
                Math::max(vb_access.max[0], vb_access.max[1], vb_access.max[2]),
                Math::max(vb_access.min[0], vb_access.min[1], vb_access.min[2])
                );
            mesh->width = aabb->max_bound.x - aabb->min_bound.x;
            mesh->height = aabb->max_bound.y - aabb->min_bound.y;
            mesh->depth = aabb->max_bound.z - aabb->min_bound.z;
        }

    }


    bool ColliderMesh::Triangle::intersect(const Ray& r, HitInfo& hit) const {
        const auto edge1 = this->v1 - this->v0;
        const auto edge2 = this->v2 - this->v0;
        const auto ray_cross_e2 = glm::cross(r.dir, edge2);
        const auto det = glm::dot(edge1, ray_cross_e2);

        if (det < epsilon_f) { return false; }

        const auto inv_det = 1.0f / det;
        const auto ray_to_v0 = r.orig - this->v0;
        const auto u = inv_det * glm::dot(ray_to_v0, ray_cross_e2);
        if ((u < 0.0f && fabs(u) > epsilon_f) || (u > 1.0f && fabs(u - 1.0f) > epsilon_f)) { return false; }

        const auto ray_to_v0_cross_e1 = glm::cross(ray_to_v0, edge1);
        const auto v = inv_det * glm::dot(r.dir, ray_to_v0_cross_e1);
        if ((v < 0.0f && fabs(v) > epsilon_f) || (u + v > 1.0f && fabs(u + v - 1.0f) > epsilon_f)) { return false; }

        hit.t = inv_det * glm::dot(edge2, ray_to_v0_cross_e1);
        if (hit.t > epsilon_f) {
            hit.local_pos = r.orig + r.dir * hit.t;
            hit.local_norm = this->norm;
            return true;
        }
        return false;
    }

    bool ColliderMesh::intersect(const Ray& r, HitInfo& hit) const {
        for (std::size_t i = 0; i < this->primitives.size(); ++i) {
            const auto& p = this->primitives[i];
            for (std::size_t j = 0; j < p.triangles.size(); ++j) {
                const auto& t = p.triangles[j];
                if (HitInfo temp_hit;
                    t.intersect(r, temp_hit)) {
                    if (temp_hit.t < hit.t) {
                        hit = temp_hit;
                        hit.prim_n = i;
                        hit.tri_n = j;
                    }
                }
            }
        }

        return hit.hit();
    }

    glm::vec3 ColliderMesh::furthest_along(const glm::mat4& t, const glm::vec3& dir) const {
        glm::vec3 best_point{};
        float best_dist{-max_f};
        for (const auto& v : this->vertices) {
            const auto point = glm::vec3(t * glm::vec4(v, 1.0f));
            const auto dist = glm::dot(point, dir);
            if (dist > best_dist) {
                best_point = point;
                best_dist = dist;
            }
        }
        return best_point;
    }

    std::size_t ColliderMesh::num_of_vertices() const {
        return this->vertices.size();
    }

    void AABB::grow(const glm::vec3& p) {
        this->min_bound = glm::min(this->min_bound, p);
        this->max_bound = glm::max(this->max_bound, p);
    }

    void AABB::grow_rot(const glm::mat4& t) {
        const auto xa = t[0] * this->min_bound.x;
        const auto xb = t[0] * this->max_bound.x;
        const auto ya = t[1] * this->min_bound.y;
        const auto yb = t[1] * this->max_bound.y;
        const auto za = t[2] * this->min_bound.z;
        const auto zb = t[2] * this->max_bound.z;
        this->min_bound = glm::vec3(glm::min(xa, xb) + glm::min(ya, yb) + glm::min(za, zb) + t[3]);
        this->max_bound = glm::vec3(glm::max(xa, xb) + glm::max(ya, yb) + glm::max(za, zb) + t[3]);
    }

    bool AABB::intersect(const Ray& r, HitInfo& hit) const {
        auto tmin{0.0f}, tmax{max_f};

        for (auto i = 0; i < 3; ++i) {
            const auto sign = signbit(r.inv_dir[i]);
            const auto bmin = this->corners[sign][i];
            const auto bmax = this->corners[!sign][i];
            const auto dmin = (bmin - r.orig[i]) * r.inv_dir[i];
            const auto dmax = (bmax - r.orig[i]) * r.inv_dir[i];
            tmin = Math::max(tmin, Math::min(dmin, dmax, tmax));
            tmax = Math::min(tmax, Math::max(dmin, dmax, tmin));
        }

        hit.t = tmin;
        return tmin < tmax;
    }

    bool AABB::intersect(const AABB& other) const {
        return (this->min_bound.x <= other.max_bound.x && this->max_bound.x >= other.min_bound.x) &&
         (this->min_bound.y <= other.max_bound.y && this->max_bound.y >= other.min_bound.y) &&
         (this->min_bound.z <= other.max_bound.z && this->max_bound.z >= other.min_bound.z);
    }

    AABB rotate_aabb_affine(const AABB& orig, const glm::mat4& t) {
        AABB ret{orig};
        ret.grow_rot(t);
        return ret;
    }

    ColliderMeshId load_collider_mesh(const std::string& filepath) {
        ColliderMeshId mesh_id;
        AABB* aabb;
        ColliderMesh* mesh;
        if (collider_mesh_id_pool.Allocate(mesh_id)) {
            collider_meshes.simple.emplace_back();
            collider_meshes.complex.emplace_back();
        }
        aabb = &collider_meshes.simple[mesh_id.index];
        mesh = &collider_meshes.complex[mesh_id.index];

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
            collider_mesh_id_pool.Deallocate(mesh_id);
            return {};
        }

        const auto primitive_count = doc.meshes[0].primitives.size();
        mesh->primitives.resize(primitive_count);
        std::size_t vertex_count{0};
        for (std::size_t i = 0; i < primitive_count; ++i) {
            const auto& prim = doc.meshes[0].primitives[i];
            const auto ib_accessor = doc.accessors[prim.indices];
            const auto vb_accessor = doc.accessors[prim.attributes.find("POSITION")->second];
            mesh->primitives[i].triangles.reserve(ib_accessor.count / 3);
            vertex_count += vb_accessor.count;
        }
        mesh->vertices.reserve(vertex_count / 3);

        for (std::size_t i = 0; i < primitive_count; ++i) {
            const auto& prim = doc.meshes[0].primitives[i];
            switch (doc.accessors[prim.indices].componentType) {
            case fx::gltf::Accessor::ComponentType::Byte:
                Internal::LoadColliderMeshPrimitive<int8_t>(doc, aabb, mesh, i);
                break;
            case fx::gltf::Accessor::ComponentType::UnsignedByte:
                Internal::LoadColliderMeshPrimitive<uint8_t>(doc, aabb, mesh, i);
                break;
            case fx::gltf::Accessor::ComponentType::Short:
                Internal::LoadColliderMeshPrimitive<int16_t>(doc, aabb, mesh, i);
                break;
            case fx::gltf::Accessor::ComponentType::UnsignedShort:
                Internal::LoadColliderMeshPrimitive<uint16_t>(doc, aabb, mesh, i);
                break;
            case fx::gltf::Accessor::ComponentType::UnsignedInt:
                Internal::LoadColliderMeshPrimitive<uint32_t>(doc, aabb, mesh, i);
                break;
            default:
                assert(false); // not supported
                break;
            }
        }

        mesh->center /= static_cast<float>(mesh->num_of_vertices());

        return mesh_id;
    }

    ColliderMeshes& get_collider_meshes() { return collider_meshes; }

} // namespace Physics
