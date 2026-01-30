#include "config.h"
#include "simplex.h"

#include "core/maths.h"


namespace Physics {

    Simplex& Simplex::operator=(const std::initializer_list<SupportPoint>& init) {
        this->m_size = 0;
        for (const auto& p : init) {
            this->m_points[this->m_size++] = p;
        }
        return *this;
    }

    void Simplex::add_point(const SupportPoint& point) {
        this->m_points = {point, this->m_points[0], this->m_points[1], this->m_points[2]};
        this->m_size = Math::min(this->m_size + 1, 4ull);
    }

    bool simplex_line(Simplex& s, glm::vec3& dir) {
        const auto a = s[0];
        const auto b = s[1];

        const auto ab = b.point - a.point;
        const auto ao = -a.point;

        if (Math::same_dir(ab, ao)) {
            s = { a, b };
            dir = glm::cross(glm::cross(ab, ao), ab);
        }
        else {
            s = { a };
            dir = ao;
        }
        return false;
    }

    bool simplex_triangle(Simplex& s, glm::vec3& dir) {
        const auto a = s[0];
        const auto b = s[1];
        const auto c = s[2];

        const auto ab = b.point - a.point;
        const auto ac = c.point - a.point;
        const auto ao = -a.point;

        const auto abc = glm::cross(ab, ac);

        if (Math::same_dir(glm::cross(abc, ac), ao)) {
            if (Math::same_dir(ac, ao)) {
                s = { a, c };
                dir = glm::cross(glm::cross(ac, ao), ac);
            } else {
                if (Math::same_dir(ab, ao)) {
                    s = { a, b };
                    dir = glm::cross(glm::cross(ab, ao), ab);
                }
                else {
                    s = { a };
                    dir = ao;
                }
            }
        } else {
            if (Math::same_dir(glm::cross(ab, abc), ao)) {
                if (Math::same_dir(ab, ao)) {
                    s = { a, b };
                    dir = glm::cross(glm::cross(ab, ao), ab);
                }
                else {
                    s = { a };
                    dir = ao;
                }
            } else {
                if (Math::same_dir(abc, ao)) {
                    s = { a, b, c };
                    dir = abc;
                } else {
                    s = { a, c, b };
                    dir = -abc;
                }
            }
        }
        return false;
    }

    bool simplex_tetrahedron(Simplex& s, glm::vec3& dir) {
        // TODO can be optimized to use less checks
        const auto a = s[0];
        const auto b = s[1];
        const auto c = s[2];
        const auto d = s[3];

        const auto ab = b.point - a.point;
        const auto ac = c.point - a.point;
        const auto ad = d.point - a.point;
        const auto ao = -a.point;

        const auto abc = glm::cross(ab, ac);
        const auto acd = glm::cross(ac, ad);
        const auto adb = glm::cross(ad, ab);

        if (Math::same_dir(abc, ao)) {
            return simplex_triangle(s = { a, b, c }, dir);
        }

        if (Math::same_dir(acd, ao)) {
            return simplex_triangle(s = { a, c, d }, dir);
        }

        if (Math::same_dir(adb, ao)) {
            return simplex_triangle(s = { a, d, b }, dir);
        }

        return true;
    }

    bool next_simplex(Simplex& s, glm::vec3& dir) {
        switch (s.size()) {
        case 2: return simplex_line(s, dir);
        case 3: return simplex_triangle(s, dir);
        case 4: return simplex_tetrahedron(s, dir);
        default: return false;
        }
    }

} // namespace Physics