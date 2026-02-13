#pragma once

#include "soloud.h"


namespace Audio {

    struct Listener {
        glm::mat4 m_transform{};
        glm::quat m_rotation{};
        glm::vec3 m_position{};

        const int MAX_RAYS = 100;
    };

} // Audio
