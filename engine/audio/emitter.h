//
// Created by fazkhu-3-local on 02/02/26.
//

#pragma once
#include "soloud_wav.h"
#include "physics/physicsresource.h"


namespace Audio {
    class Emitter {
    public:
        glm::mat4 m_transform{};
        glm::vec3 m_position{};

        SoLoud::Wav m_source;

        unsigned int m_attenuation_type{SoLoud::AudioSource::INVERSE_DISTANCE};
        Physics::ColliderId m_self_collider;
        float m_rolloff{0.75f};
        float m_min_dist{5.0f}, m_max_dist{50.0f};
    };
} // Audio
