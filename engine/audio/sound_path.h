//
// Created by fazkhu-3-local on 02/20/26.
//

#pragma once


namespace Physics {
    struct ColliderId;
}


namespace Audio {

    struct sound_path_id {
        void extend(Physics::ColliderId cid, uint32_t tri_n);

        std::vector<uint64_t> id;
    };

    struct sound_path_data {
        void extend(const glm::vec3& pos, float t);

        glm::vec3 position;
        float length{};
        int bounces{};
    };

} // Audio

namespace std {
    template<>
    struct hash<Audio::sound_path_id> {
        size_t operator()(const Audio::sound_path_id& path) const {
            auto ret = std::hash<uint64_t>()(path.id[0]);
            for (auto i = 1; i < path.id.size(); ++i) {
                ret ^= std::hash<uint64_t>()(path.id[i]);
            }
            return ret;
        }
    };

    template<>
    struct equal_to<Audio::sound_path_id> {
        size_t operator()(const Audio::sound_path_id& lhs, const Audio::sound_path_id& rhs) const {
            return std::hash<Audio::sound_path_id>()(lhs) == std::hash<Audio::sound_path_id>()(rhs);
        }
    };
}