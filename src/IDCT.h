#pragma once


class IDCT {
    public:
        void transform(int (&block)[64]) noexcept;

    private:
        const float m0 = 1.847759065;
        const float m1 = 1.414213562;
        const float m3 = 1.414213562;
        const float m5 = 0.765366865;
        const float m2 = 1.082392200;
        const float m4 = 2.613125930;

        const float s0 = 0.353553391;
        const float s1 = 0.490392640;
        const float s2 = 0.461939766;
        const float s3 = 0.415734806;
        const float s4 = 0.353553391;
        const float s5 = 0.277785117;
        const float s6 = 0.191341716;
        const float s7 = 0.097545161;
};
