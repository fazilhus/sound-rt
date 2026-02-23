//
// Created by fazkhu-3-local on 02/03/26.
//

#pragma once

#include <deque>

#include "soloud.h"

#include "emitter.h"
#include "listener.h"
#include "scene.h"
#include "sound_path.h"
#include "physics/ray.h"


namespace Audio {
    struct Listener;
    struct Emitter;

    constexpr auto NUM_RAYS_PER_FRAME = 128;

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

        void load_scene(const std::string& filepath, bvh::SplitAlgo algo = bvh::Longest);

        void set_emitter_collider(Physics::ColliderId cid);

        void update_sound_speed(float sound_speed);
        void update_listener_pos_and_at(const glm::vec3& position, const glm::quat& rot);
        void update_emitter_position(const glm::vec3& position);

        void update();

        void debug_draw() const;

    private:
        void _direct_los_stage();
        void _indirect_stage();

        [[nodiscard]] bool _has_los(const glm::vec3& from, const glm::vec3& to) const;

        SoLoud::Soloud m_soloud;

        Listener m_listener;
        Emitter m_emitter;

        std::deque<ray> m_ray_cq;
        std::unordered_map<sound_path_id, sound_path_data> m_paths[4];
        scene m_scene;
    };
} // Audio
