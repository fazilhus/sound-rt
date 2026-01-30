#pragma once
#include <numbers>
#include "vec3.hpp"

#include "physics/physicsresource.h"

namespace Math {

    constexpr auto pi_f = std::numbers::pi_v<float>;

    inline float len_sq(const glm::vec3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }

    inline glm::vec3 safe_normal(const glm::vec3& v) {
        const auto len_sq = v.x * v.x + v.y * v.y + v.z * v.z;
        if (len_sq <= Physics::epsilon_f) { return {0, 0, 0}; }
        return v / sqrt(len_sq);
    }

    inline glm::vec2 norm_screen_pos(const glm::vec2& sp, const glm::vec2& ss) {
        return glm::vec2(2.0f * (sp / ss) - 1.0f);
    }

    inline glm::quat rot_vec3(const glm::vec3& norm) {
        constexpr auto up = glm::vec3(0.0f, 0.0f, 1.0f);
        const auto axis = glm::cross(up, norm);
        const float angle = glm::acos(glm::dot(up, norm));

        return glm::angleAxis(angle, glm::normalize(axis));
    }

    template <typename T>
    T min(const T a, const T b) { return a < b ? a : b; }
    template <typename T>
    T min(const T a, const T b, const T c) { return min(min(a, b), c); }

    template <typename T>
    T max(const T a, const T b) { return a > b ? a : b; }
    template <typename T>
    T max(const T a, const T b, const T c) { return max(max(a, b), c); }

    inline float clamp(const float v, const float min_v, const float max_v) {
        return v < min_v ? min_v : v > max_v ? max_v : v;
    }

    inline bool same_dir(const glm::vec3& a, const glm::vec3& b) {
        return glm::dot(a, b) > 0.0f;
    }

    inline glm::quat add_scaled_vec(const glm::quat& a, const glm::quat& b) {
        auto q = glm::quat(0.0f, b.x * b.w, b.y * b.w, b.z * b.w);
        q *= a;
        return a + 0.5f * q;
    }

}
