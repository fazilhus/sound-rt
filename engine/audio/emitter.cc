//
// Created by fazkhu-3-local on 02/02/26.
//

#include "config.h"
#include "emitter.h"

#include "audio_manager.h"
#include "core/maths.h"


namespace Audio {

    void Voices::init(SoLoud::Soloud& soloud, SoLoud::Wav& source, const unsigned int attenuation_type, const float rolloff, const float min_dist, const float max_dist) {
        m_data.resize(MAX_VOICES_PER_EMITTER);
        for (auto& [_1, _2, m_handle] : m_data) {
            m_handle = soloud.play3d(source, 0, 0, 0, 0, 0, 0, 0);
            soloud.set3dSourceAttenuation(m_handle, attenuation_type, rolloff);
            soloud.set3dSourceMinMaxDistance(m_handle, min_dist, max_dist);
        }
    }

    void Emitter::init(SoLoud::Soloud& soloud, const std::string& path) {
        m_source.load(path.c_str());
        m_source.setLooping(true);
        m_source.setVolume(1.0f);
        m_source.set3dDistanceDelay(true);
        m_source.set3dDopplerFactor(true);
        m_source.setInaudibleBehavior(true, false);

        m_voices.init(soloud,m_source, m_attenuation_type, m_rolloff, m_min_dist, m_max_dist);
    }

    void Emitter::update(SoLoud::Soloud& soloud) const {
        for (auto i = 0; i < m_voices.m_next_available_voice; i++) {
            const auto& voice = m_voices.m_data[i];
            soloud.set3dSourcePosition(voice.m_handle, voice.m_position.x, voice.m_position.y, voice.m_position.z);
            soloud.setVolume(voice.m_handle, voice.m_volume);
        }

        for (auto i = m_voices.m_next_available_voice; i < m_voices.m_data.size(); i++) {
            soloud.setVolume(m_voices.m_data[i].m_handle, 0.0f);
        }
    }

    void Emitter::activate_voice(const glm::vec3& pos, float travelled) {
        const auto idx = m_voices.m_next_available_voice++;
        m_voices.m_data[idx].m_position = pos;
        travelled = Math::clampf(travelled, m_min_dist, m_max_dist);
        m_voices.m_data[idx].m_volume = 0.01f * m_min_dist / (m_min_dist + m_rolloff * (travelled - m_min_dist));
    }

    void Emitter::reset_voices() {
        m_voices.m_next_available_voice = 0;
    }

} // Audio