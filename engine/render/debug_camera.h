#pragma once
#include "cameramanager.h"
#include "physics/ray.h"


namespace Render {

    struct DebugCamera {
        glm::vec3 pos = glm::vec3(0);
        glm::quat ort = glm::identity<glm::quat>();
        glm::mat4 trans = glm::mat4(1);
        float speed;
        float rot_speed;

        explicit DebugCamera(const float speed = 1.0f, const float rot_speed = 2.0f)
            : speed(speed), rot_speed(rot_speed) {}

        void Update(float dt);

        Physics::Ray SpawnRay() const;
    };

} // namespace Render
