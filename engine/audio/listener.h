#pragma once

#include "soloud.h"


namespace Audio {

    class Listener {
    public:
        glm::mat4 m_transform{};
        glm::quat m_rotation{};
        glm::vec3 m_position{};
    };

} // Audio
