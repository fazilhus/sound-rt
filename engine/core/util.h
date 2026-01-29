#pragma once
#include <ostream>

#include "vec3.hpp"

inline std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
    os << '(' << v.x << ',' << v.y << ',' << v.z << ')';
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec4& v) {
    os << '(' << v.x << ',' << v.y << ',' << v.z << ',' << v.w << ')';
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::quat& q) {
    os << "(w:" << q.w << ',' << q.x << ',' << q.y << ',' << q.z << ')';
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::mat4& m) {
    os << m[0] << '\n' << m[1] << '\n' << m[2] << '\n' << m[3];
    return os;
}
