#pragma once
#include <array>
#include "physicsresource.h"

namespace Physics {

    struct Simplex {
    private:
        std::size_t m_size;
        std::array<SupportPoint, 4> m_points;

    public:
        explicit Simplex() : m_size(0), m_points() {};

        Simplex& operator=(const std::initializer_list<SupportPoint>& init);

        void add_point(const SupportPoint& point);

        const SupportPoint& operator[](const int i) const { return this->m_points[i]; }
        SupportPoint& operator[](const int i) { return this->m_points[i]; }
        [[nodiscard]] std::size_t size() const { return this->m_size; }

        [[nodiscard]] auto begin() const { return this->m_points.begin(); }
        [[nodiscard]] auto end() const { return this->m_points.end() - static_cast<ptrdiff_t>(4 - this->m_size); }
    };

    bool next_simplex(Simplex& s, glm::vec3& dir);

} // namespace Physics