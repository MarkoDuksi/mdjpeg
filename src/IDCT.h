#pragma once

#include <cmath>


class IDCT {

    public:

        void transform(int (&block)[64]) const noexcept;

    private:

        const float m0 = 2.0 * std::cos(1.0 / 16.0 * 2.0 * M_PI);
        const float m1 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
        const float m3 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
        const float m5 = 2.0 * std::cos(3.0 / 16.0 * 2.0 * M_PI);
        const float m2 = m0 - m5;
        const float m4 = m0 + m5;

        const float s0 = std::cos(0.0 / 16.0 * M_PI) / std::sqrt(8);
        const float s1 = std::cos(1.0 / 16.0 * M_PI) / 2.0;
        const float s2 = std::cos(2.0 / 16.0 * M_PI) / 2.0;
        const float s3 = std::cos(3.0 / 16.0 * M_PI) / 2.0;
        const float s4 = std::cos(4.0 / 16.0 * M_PI) / 2.0;
        const float s5 = std::cos(5.0 / 16.0 * M_PI) / 2.0;
        const float s6 = std::cos(6.0 / 16.0 * M_PI) / 2.0;
        const float s7 = std::cos(7.0 / 16.0 * M_PI) / 2.0;
};
