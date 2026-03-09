//
// Created by fazkhu-3-local on 02/02/26.
//

#pragma once
#include <array>

#include "soloud_biquadresonantfilter.h"
#include "soloud_wav.h"
#include "physics/physicsresource.h"


namespace Audio {

    struct Voice {
        glm::vec3 m_position{};
        float m_volume{0.0f};
        SoLoud::handle m_handle{};
    };

    struct Voices {
        std::vector<Voice> m_data{};
        std::size_t m_next_available_voice{};

        void init(SoLoud::Soloud& soloud, SoLoud::Bus& bus, SoLoud::Wav& source, unsigned int attenuation_type, float rolloff, float min_dist, float max_dist);
    };

    constexpr auto MAX_VOICES_PER_EMITTER{512};//1023-5};

    struct Emitter {
        glm::mat4 m_transform{};
        glm::vec3 m_position{};

        SoLoud::Wav m_source;
        std::array<SoLoud::Bus, 5> m_busses;
        // std::array<SoLoud::BiquadResonantFilter, 4> m_filters;
        Voices m_voices;

        unsigned int m_attenuation_type{SoLoud::AudioSource::INVERSE_DISTANCE};
        Physics::ColliderId m_self_collider;
        float m_rolloff{0.1f};
        float m_min_dist{0.1f}, m_max_dist{50.0f};
        float m_volume{1.0f};

        void init(SoLoud::Soloud& soloud, const std::string& path);

        void update(SoLoud::Soloud& soloud) const;

        void activate_voice(const glm::vec3& pos, float travelled = 0.0f, int bounces = 0);
        void reset_voices();
    };
} // Audio
