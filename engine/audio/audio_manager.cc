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
        m_soloud.setMaxActiveVoiceCount(MAX_VOICES_PER_EMITTER);

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

    void AudioManager::set_emitter_collider(const Physics::ColliderId cid) {
        m_emitter.m_self_collider = cid;
    }

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
        m_soloud.update3dAudio();
        m_emitter.reset_voices();

        // _direct_los_stage();
        _indirect_stage();

        m_emitter.update(m_soloud);
    }

    void AudioManager::_direct_los_stage() {
        const auto res = _has_los(m_listener.m_position, m_emitter.m_position);
        if (res) {
            m_emitter.activate_voice(m_emitter.m_position);
        }
    }

    void AudioManager::_indirect_stage() {
        for (auto i = 0; i < 1024; ++i) {
            this->m_queued_rays.emplace_back(m_emitter.m_position, Core::RandomPointOnUnitSphere());
        }

        while (!this->m_queued_rays.empty()) {
            const auto ray = this->m_queued_rays.front();
            this->m_queued_rays.pop_front();

            if (Physics::HitInfo hit_info;
                Physics::cast_ray(ray, hit_info, Physics::CollisionMask::Audio)) {
                const auto new_ray_pos = hit_info.pos + Physics::epsilon_f * hit_info.norm;
                if (ray.bounces < Physics::MAX_RAY_BOUNCES) {
                    const auto new_ray = Physics::Ray(new_ray_pos,
                        glm::reflect(ray.dir, hit_info.norm),
                        true,
                        ray.bounces + 1,
                        hit_info.t + ray.travelled);
                    this->m_queued_rays.emplace_back(
                            new_ray
                        );
                }

                if (_has_los(m_listener.m_position, new_ray_pos)) {
                    // Debug::DrawBox(hit_info.pos, glm::quat(), 0.1f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
                    m_emitter.activate_voice(new_ray_pos, ray.travelled);
                }
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
