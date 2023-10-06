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

        bool decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk);
        bool decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk, BlockWriter& writer);
        bool dc_decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk);

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
            uint height_px {};
            uint width_px {};
            uint height_blk {};
            uint width_blk {};
            uint horiz_chroma_subs_factor {};

            void set_dims(uint height_px, uint width_px) {

                this->height_px = height_px;
                this->width_px = width_px;
                height_blk = (height_px + 7) / 8;
                width_blk = (width_px + 7) / 8;
            }

            bool is_set() const noexcept {

                return height_px;
            }
        } m_frame_info {};

        StateID parse_header();

        // inplace shift all block values by +128 and clip the result to uint8_t range
        void range_normalize(int (&block)[64]) noexcept;
};
