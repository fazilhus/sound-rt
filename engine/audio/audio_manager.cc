//
// Created by fazkhu-3-local on 02/03/26.
//

#include "config.h"
#include "audio_manager.h"

#include "core/maths.h"
#include "core/random.h"
#include "physics/phy.h"
#include "physics/ray.h"


namespace Audio {

    audio_manager::audio_manager() {
        m_soloud.init();
        m_emitter.m_source.load(fs::create_path_from_rel_s("assets/audio/jazz.mp3").c_str());
        m_emitter.m_source.setLooping(true);
        m_emitter.m_source.setVolume(1.0f);
        m_emitter.m_source.setInaudibleBehavior(true, false);

        m_soloud.set3dListenerPosition(0, 0, 0);
        m_soloud.set3dListenerUp(0, 1, 0);
        m_soloud.set3dListenerAt(0, 0, -1);
        m_handle = m_soloud.play3d(m_emitter.m_source, 0, 0, 0);
        m_soloud.set3dSourceAttenuation(m_handle, m_emitter.m_attenuation_type, m_emitter.m_rolloff);
        m_soloud.set3dSourceMinMaxDistance(m_handle, m_emitter.m_min_dist, m_emitter.m_max_dist);
    }

    audio_manager::~audio_manager() {
        m_soloud.stopAll();
        m_soloud.deinit();
    }

    void audio_manager::set_emitter_collider(const Physics::ColliderId cid) {
        m_emitter.m_self_collider = cid;
    }

    void audio_manager::update_listener_pos_and_at(const glm::vec3& position, const glm::quat& rot) {
        m_listener.m_position = position;
        m_listener.m_rotation = rot;
        m_listener.m_transform = glm::translate(m_listener.m_position) * glm::mat4(m_listener.m_rotation);
        m_soloud.set3dListenerPosition(m_listener.m_position.x, m_listener.m_position.y, m_listener.m_position.z);
        const auto fwd = Math::forward_from_quat(m_listener.m_rotation);
        m_soloud.set3dListenerAt(fwd.x, fwd.y, fwd.z);
        b_should_update = true;
    }

    void audio_manager::update_emitter_position(const glm::vec3& position) {
        m_emitter.m_position = position;
        m_emitter.m_transform = glm::translate(m_emitter.m_position);
        m_soloud.set3dSourcePosition(m_handle, m_emitter.m_position.x, m_emitter.m_position.y, m_emitter.m_position.z);
        b_should_update = true;
    }

    void audio_manager::update() {
        if (b_should_update) {
            m_soloud.update3dAudio();
            b_should_update = false;
        }

        _direct_los_stage();
        _indirect_stage();
    }

    void audio_manager::_direct_los_stage() {
        const auto has_los = _has_los(m_listener.m_position, m_emitter.m_position);
        m_soloud.setVolume(m_handle, has_los ? 0.0f : m_emitter.m_volume);
    }

    void audio_manager::_indirect_stage() {
        for (auto i = 0; i < 10; ++i) {
            this->m_queued_rays.emplace_back(m_emitter.m_position, Core::RandomPointOnUnitCircle());
        }

        while (!this->m_queued_rays.empty()) {
            const auto r = this->m_queued_rays.front();
            this->m_queued_rays.pop_front();

            if (Physics::HitInfo info;
                Physics::cast_ray(r, info, Physics::CollisionMask::Audio)) {

            }
        }
    }

    bool audio_manager::_has_los(const glm::vec3& from, const glm::vec3& to) const {
        const auto ray = Physics::Ray(from, to - from, false);
        Physics::HitInfo info;
        auto b_res = Physics::cast_ray(ray, info, Physics::CollisionMask::Audio);
        return b_res && info.collider != m_emitter.m_self_collider;
    }


} // Audio
