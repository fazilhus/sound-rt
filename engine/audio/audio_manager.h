//
// Created by fazkhu-3-local on 02/03/26.
//

#pragma once

#include "soloud.h"
#include "soloud_wav.h"

#include "emitter.h"
#include "listener.h"

namespace Audio {
    class Listener;
    class Emitter;
    class audio_manager {
    public:
        static audio_manager& get_instance() {
            static audio_manager instance;
            return instance;
        }

    private:
        audio_manager();

    public:
        ~audio_manager();

        audio_manager(const audio_manager&) = delete;
        void operator=(const audio_manager&) = delete;

        void set_emitter_collider(Physics::ColliderId cid);

        void update_listener_pos_and_at(const glm::vec3& position, const glm::quat& rot);
        void update_emitter_position(const glm::vec3& position);

        void update();

    private:
        SoLoud::Soloud m_soloud;

        SoLoud::handle m_handle;

        Listener m_listener;
        Emitter m_emitter;

        bool b_should_update = false;
    };
} // Audio
