//
// Created by fazkhu-3-local on 02/20/26.
//

#include "config.h"
#include "sound_path.h"

#include "physics/physicsresource.h"


namespace Audio {

    void sound_path_id::extend(const uint32_t tri_n) {
        id.push_back(tri_n);
    }

    void sound_path_data::extend(const glm::vec3& pos, const float t) {
        position = pos;
        length += t;
        bounces++;
    }

} // Audio