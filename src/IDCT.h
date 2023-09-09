#pragma once

#include <cmath>

class IDCT {
    public:
        void transform(int (&block)[64]) noexcept;

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

        // const float m0 = 1.847759065;
        // const float m1 = 1.414213562;
        // const float m3 = 1.414213562;
        // const float m5 = 0.765366865;
        // const float m2 = 1.082392200;
        // const float m4 = 2.613125930;

        // const float s0 = 0.353553391;
        // const float s1 = 0.490392640;
        // const float s2 = 0.461939766;
        // const float s3 = 0.415734806;
        // const float s4 = 0.353553391;
        // const float s5 = 0.277785117;
        // const float s6 = 0.191341716;
        // const float s7 = 0.097545161;
};
