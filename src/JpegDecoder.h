#pragma once

#include <stdint.h>
#include <cmath>

#include "JpegReader.h"
#include "Dequantizer.h"
#include "ZigZag.h"
#include "Huffman.h"
#include "states.h"


class JpegDecoder {
    public:
        JpegDecoder(const uint8_t* const buff, const size_t size) noexcept;

        bool decode(uint8_t* const dst, uint x1_blocks = 0, uint y1_blocks = 0, uint x2_blocks = 0, uint y2_blocks = 0);
        bool low_pass_decode(uint8_t* const dst, uint x1_blocks = 0, uint y1_blocks = 0, uint x2_blocks = 0, uint y2_blocks = 0);

        template <StateID ANY>
        friend class ConcreteState;

    private:
        ConcreteState<StateID::ENTRY> m_state;
        State* m_istate;

        JpegReader m_reader;
        Dequantizer m_dequantizer {};
        ZigZag m_zigzag {};
        Huffman m_huffman {};

        struct {
            uint height_px {0};
            uint width_px {0};
            uint height_blocks {0};
            uint width_blocks {0};
            uint horiz_chroma_subs_factor {false};

            void set_dims(uint height_px, uint width_px) {
                this->height_px = height_px;
                this->width_px = width_px;
                height_blocks = (height_px + 7) / 8;
                width_blocks = (width_px + 7) / 8;
            }

            bool is_set() const noexcept {
                return height_px;
            }
        } m_frame_info {};

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

        void idct(int (&block)[64]) noexcept;
        void level_to_unsigned(int (&block)[64]) noexcept;
};
