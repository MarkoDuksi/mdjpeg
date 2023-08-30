#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "states.h"
#include "JpegReader.h"
#include "Huffman.h"
#include "Dequantizer.h"
#include "ZigZag.h"
#include "IDCT.h"
#include "BlockWriter.h"


class JpegDecoder {
    public:
        JpegDecoder(const uint8_t* const buff, const size_t size) noexcept;

        bool decode(uint8_t* const dst, uint x1_blocks, uint y1_blocks, uint x2_blocks, uint y2_blocks);
        bool decode(uint8_t* const dst, uint x1_blocks, uint y1_blocks, uint x2_blocks, uint y2_blocks, BlockWriter& writer);
        bool dc_decode(uint8_t* const dst, uint x1_blocks, uint y1_blocks, uint x2_blocks, uint y2_blocks);

        template <StateID ANY>
        friend class ConcreteState;

    private:
        ConcreteState<StateID::ENTRY> m_state;
        State* m_istate;

        JpegReader m_reader;
        Huffman m_huffman {};
        Dequantizer m_dequantizer {};
        ZigZag m_zigzag {};
        IDCT m_idct {};

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

        StateID parse_header();

        void level_transform(int (&block)[64]) noexcept;
};
