#include "transform.h"

#include <sys/types.h>
#include <cmath>


using namespace mdjpeg;

/// \cond
namespace {

namespace ZigZag {

    const uint8_t map[64] {
            0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    };
}  // namespace ZigZag

namespace IDCT {

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
}  // namespace IDCT

}  // namespace
/// \endcond

void transform::reverse_zig_zag(int (&block)[64]) noexcept {

    int temp[64];

    for (uint i = 0; i < 64; ++i) {

        temp[i] = block[i];
    }

    for (uint i = 0; i < 64; ++i) {

        block[ZigZag::map[i]] = temp[i];
    }
}

/// \note AAN %IDCT implementation adapted from and contributed back to https://github.com/dannye/jed/blob/master/src/decoder.cpp.
void transform::idct(int (&block)[64]) noexcept {

    float intermediate[64];

    for (uint i = 0; i < 8; ++i) {

        const float g0 = block[0 * 8 + i] * IDCT::s0;
        const float g1 = block[4 * 8 + i] * IDCT::s4;
        const float g2 = block[2 * 8 + i] * IDCT::s2;
        const float g3 = block[6 * 8 + i] * IDCT::s6;
        const float g4 = block[5 * 8 + i] * IDCT::s5;
        const float g5 = block[1 * 8 + i] * IDCT::s1;
        const float g6 = block[7 * 8 + i] * IDCT::s7;
        const float g7 = block[3 * 8 + i] * IDCT::s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * IDCT::m1;
        const float d3 = e3;
        const float d4 = e4 * IDCT::m2;
        const float d5 = e5 * IDCT::m3;
        const float d6 = e6 * IDCT::m4;
        const float d7 = e7;
        const float d8 = e8 * IDCT::m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        intermediate[0 * 8 + i] = b0 + b7;
        intermediate[1 * 8 + i] = b1 + b6;
        intermediate[2 * 8 + i] = b2 + b5;
        intermediate[3 * 8 + i] = b3 + b4;
        intermediate[4 * 8 + i] = b3 - b4;
        intermediate[5 * 8 + i] = b2 - b5;
        intermediate[6 * 8 + i] = b1 - b6;
        intermediate[7 * 8 + i] = b0 - b7;
    }

    for (uint i = 0; i < 8; ++i) {

        const float g0 = intermediate[i * 8 + 0] * IDCT::s0;
        const float g1 = intermediate[i * 8 + 4] * IDCT::s4;
        const float g2 = intermediate[i * 8 + 2] * IDCT::s2;
        const float g3 = intermediate[i * 8 + 6] * IDCT::s6;
        const float g4 = intermediate[i * 8 + 5] * IDCT::s5;
        const float g5 = intermediate[i * 8 + 1] * IDCT::s1;
        const float g6 = intermediate[i * 8 + 7] * IDCT::s7;
        const float g7 = intermediate[i * 8 + 3] * IDCT::s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * IDCT::m1;
        const float d3 = e3;
        const float d4 = e4 * IDCT::m2;
        const float d5 = e5 * IDCT::m3;
        const float d6 = e6 * IDCT::m4;
        const float d7 = e7;
        const float d8 = e8 * IDCT::m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        block[i * 8 + 0] = std::lround(b0 + b7);
        block[i * 8 + 1] = std::lround(b1 + b6);
        block[i * 8 + 2] = std::lround(b2 + b5);
        block[i * 8 + 3] = std::lround(b3 + b4);
        block[i * 8 + 4] = std::lround(b3 - b4);
        block[i * 8 + 5] = std::lround(b2 - b5);
        block[i * 8 + 6] = std::lround(b1 - b6);
        block[i * 8 + 7] = std::lround(b0 - b7);
    }
}

void transform::range_normalize(int (&block)[64]) noexcept {

    for (uint i = 0; i < 64; ++i) {

        if (block[i] < -128) {

            block[i] = 0;
        }

        else if (block[i] > 127) {

            block[i] = 255;
        }

        else {

            block[i] += 128;
        }
    }
}
