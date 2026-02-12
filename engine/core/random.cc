//------------------------------------------------------------------------------
//  random.cc
//  (C) 2018-2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "random.h"

#include "maths.h"


namespace Core {
    union RandomUnion {
        unsigned int i;
        float f;
    };

    //------------------------------------------------------------------------------
    /**
    XorShift128 implementation.
*/
    uint FastRandom() {
        // These are predefined to give us the largest
        // possible sequence of random numbers
        static uint x = 123456789;
        static uint y = 362436069;
        static uint z = 521288629;
        static uint w = 88675123;
        uint t = x ^ (x << 11);
        x = y;
        y = z;
        z = w;
        return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
    }

    //------------------------------------------------------------------------------
    /**
    Thanks to Nic Werneck (https://xor0110.wordpress.com/2010/09/24/how-to-generate-floating-point-random-numbers-efficiently/)
*/
    float RandomFloat() {
        static RandomUnion r;
        r.i = FastRandom() & 0x007fffff | 0x3f800000;
        return r.f - 1.0f;
    }

    //------------------------------------------------------------------------------
    /**
*/
    float RandomFloatNTP() {
        static RandomUnion r;
        r.i = FastRandom() & 0x007fffff | 0x40000000;
        return r.f - 3.0f;
    }

    float RandomNormal(const float mean, const float std_deriv) {
        const auto theta = 2.0f * Math::pi_f * RandomFloat();
        const auto rho = sqrtf(-2.0f * logf(RandomFloat()));
        const auto scale = std_deriv * rho;
        return mean + scale * cosf(theta);
    }

    glm::vec3 RandomPointOnUnitCircle() {
        return glm::normalize(glm::vec3{
            RandomNormal(0.0f, 1.0f),
            0.0f,
            RandomNormal(0.0f, 1.0f),
        });
    }

    glm::vec3 RandomPointOnUnitSphere() {
        return glm::normalize(glm::vec3{
            RandomNormal(0.0f, 1.0f),
            RandomNormal(0.0f, 1.0f),
            RandomNormal(0.0f, 1.0f),
        });
    }
} // namespace Core