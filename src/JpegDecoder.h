#pragma once

#include <stdint.h>
#include <cmath>

#include "JpegReader.h"
#include "JpegHeader.h"
#include "states.h"


class JpegDecoder {
    public:
        JpegDecoder(const uint8_t* const buff, const size_t size) noexcept;

        bool decode(uint8_t* const dst, uint16_t x1_mcu = 0, uint16_t y1_mcu = 0, uint16_t x2_mcu = 0, uint16_t y2_mcu = 0);
        bool low_pass_decode(uint8_t* const dst, uint16_t x1_mcu = 0, uint16_t y1_mcu = 0, uint16_t x2_mcu = 0, uint16_t y2_mcu = 0);

        template <StateID ANY>
        friend class ConcreteState;

    private:
        JpegReader m_reader;
        JpegHeader m_header {};
        ConcreteState<StateID::ENTRY> m_state;
        State* m_istate;

        const uint8_t m_zig_zag_map[64] {
             0,  1,  8, 16,  9,  2,  3, 10,
            17, 24, 32, 25, 18, 11,  4,  5,
            12, 19, 26, 33, 40, 48, 41, 34,
            27, 20, 13,  6,  7, 14, 21, 28,
            35, 42, 49, 56, 57, 50, 43, 36,
            29, 22, 15, 23, 30, 37, 44, 51,
            58, 59, 52, 45, 38, 31, 39, 46,
            53, 60, 61, 54, 47, 55, 62, 63
        };

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

        StateID parse_header();

        uint8_t get_dc_huff_symbol(const uint table_id) noexcept;
        uint8_t get_ac_huff_symbol(const uint table_id) noexcept;
        int16_t get_dct_coeff(const uint length) noexcept;

        bool huff_decode_block(int (&dst_block)[64], const uint table_id) noexcept;
        bool huff_decode_luma(int (&dst_block)[64], const uint mcu_row, const uint mcu_col) noexcept;
        void dequantize(int (&block)[64]) noexcept;
        void idct(int (&block)[64]) noexcept;
        void level_to_unsigned(int (&block)[64]) noexcept;
};
