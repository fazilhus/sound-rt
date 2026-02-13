//
// Created by fazkhu-3-local on 02/03/26.
//

#pragma once

#include <deque>

#include "soloud.h"

#include "emitter.h"
#include "listener.h"
#include "physics/ray.h"


namespace Audio {
    struct Listener;
    struct Emitter;
    class AudioManager {
    public:
        static AudioManager& get() {
            static AudioManager instance;
            return instance;
        }

    private:
        AudioManager();

    public:
        ~AudioManager();

        AudioManager(const AudioManager&) = delete;
        void operator=(const AudioManager&) = delete;

        void set_emitter_collider(Physics::ColliderId cid);

        void update_listener_pos_and_at(const glm::vec3& position, const glm::quat& rot);
        void update_emitter_position(const glm::vec3& position);

        void update();

    private:
        void _direct_los_stage();
        void _indirect_stage();

        [[nodiscard]] bool _has_los(const glm::vec3& from, const glm::vec3& to) const;

        SoLoud::Soloud m_soloud;

        Listener m_listener;
        Emitter m_emitter;

        std::deque<Physics::Ray> m_queued_rays;
    };
} // Audio
