//
// Created by fazkhu-3-local on 02/03/26.
//

#include "config.h"
#include "audio_manager.h"


#include "core/maths.h"
#include "core/random.h"
#include "physics/phy.h"
#include "physics/ray.h"
#include "render/debugrender.h"


namespace Audio {


    AudioManager::AudioManager() {
        m_soloud.init();
        m_soloud.setMaxActiveVoiceCount(MAX_VOICES_PER_EMITTER + 5);

        m_emitter.init(
            m_soloud,
            fs::create_path_from_rel_s("assets/audio/jazz.mp3")
            );

        m_soloud.set3dListenerPosition(0, 0, 0);
        m_soloud.set3dListenerUp(0, 1, 0);
        m_soloud.set3dListenerAt(0, 0, -1);
    }

    AudioManager::~AudioManager() {
        m_soloud.stopAll();
        m_soloud.deinit();
    }

    void AudioManager::load_scene(const std::string& filepath, const bvh::SplitAlgo algo) {
        m_scene.load_from(filepath, algo);
    }

    void AudioManager::set_emitter_collider(const Physics::ColliderId cid) { m_emitter.m_self_collider = cid; }
    void AudioManager::update_sound_speed(const float sound_speed) { m_soloud.set3dSoundSpeed(sound_speed); }

    void AudioManager::update_listener_pos_and_at(const glm::vec3& position, const glm::quat& rot) {
        m_listener.m_position = position;
        m_listener.m_rotation = rot;
        m_listener.m_transform = glm::translate(m_listener.m_position) * glm::mat4(m_listener.m_rotation);
        m_soloud.set3dListenerPosition(m_listener.m_position.x, m_listener.m_position.y, m_listener.m_position.z);
        const auto fwd = Math::forward_from_quat(m_listener.m_rotation);
        m_soloud.set3dListenerAt(fwd.x, fwd.y, fwd.z);
    }

    void AudioManager::update_emitter_position(const glm::vec3& position) {
        m_emitter.m_position = position;
        m_emitter.m_transform = glm::translate(m_emitter.m_position);
    }

    void AudioManager::update() {
        m_emitter.reset_voices();

        _direct_los_stage();
        _indirect_stage();

        m_emitter.update(m_soloud);
        m_soloud.update3dAudio();
    }

    void AudioManager::debug_draw() const {
        m_scene.draw_bvh();
    }

    void AudioManager::_direct_los_stage() {
        const auto res = _has_los(m_listener.m_position, m_emitter.m_position);
        if (res) { m_emitter.activate_voice(m_emitter.m_position); }
    }

    void AudioManager::_indirect_stage() {
        for (auto i = 0; i < NUM_RAYS_PER_FRAME; ++i) {
            m_ray_cq.emplace_back(m_emitter.m_position, Core::RandomPointOnUnitSphere());
        }

        while (!m_ray_cq.empty()) {
            auto r = m_ray_cq.front();
            m_ray_cq.pop_front();

            sound_path_id path_id;
            sound_path_data path_data;
            for (auto i = 0; i < 4; ++i) {
                Physics::HitInfo hit_info;
                m_scene.bvh_tree.intersect(m_scene.triangles, m_scene.indices, r, hit_info);
                if (hit_info.hit()) {
                    path_id.extend(hit_info.tri_n);
                    path_data.extend(hit_info.pos, hit_info.t);
                    if (!m_paths[i].contains(path_id)) { m_paths[i][path_id] = path_data; }

                    r = ray(hit_info.pos + Physics::epsilon_f * hit_info.norm, glm::reflect(r.dir, hit_info.norm));
                    continue;
                }

                break;
            }
        }

        auto num_voices = 0;
        for (auto & m_path : m_paths) {
            for (auto it = m_path.begin(); it != m_path.end();) {
                if (const auto& pd = it->second;
                    _has_los(m_listener.m_position, pd.position)) {
                    const auto bounce_ratio = static_cast<float>(Physics::MAX_RAY_BOUNCES - pd.bounces) / static_cast<float>(Physics::MAX_RAY_BOUNCES);
                    Debug::DrawBox(
                        pd.position,
                        glm::quat(),
                        0.1f,
                        glm::vec4(
                            bounce_ratio,
                            1.0f - bounce_ratio,
                            0.0f,
                            1.0f)
                        );
                    num_voices++;
                    ++it;
                } else {
                    it = m_path.erase(it);
                }
            }
        }

        for (auto & m_path : m_paths) {
            for (auto it = m_path.begin(); it != m_path.end(); ++it) {
                const auto& [position, length, bounces] = it->second;
                m_emitter.activate_voice(position, length, bounces);
            }
        }
    }

    bool AudioManager::_has_los(const glm::vec3& from, const glm::vec3& to) const {
        const auto ray = Physics::Ray(from, to - from, false);
        Physics::HitInfo info;
        auto b_res = Physics::cast_ray(ray, info, Physics::CollisionMask::Audio);
        return !b_res || info.collider == m_emitter.m_self_collider;
    }


} // Audio
