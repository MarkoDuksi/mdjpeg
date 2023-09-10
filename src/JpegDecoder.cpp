#include "JpegDecoder.h"

#include <iostream>


JpegDecoder::JpegDecoder(const uint8_t* const buff, const size_t size) noexcept :
    m_state(ConcreteState<StateID::ENTRY>(this)),
    m_istate(&m_state),
    m_reader(buff, size)
    {}

bool JpegDecoder::decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk) {
    BasicBlockWriter writer;
    return decode(dst, x1_blk, y1_blk, x2_blk, y2_blk, writer);
}

bool JpegDecoder::decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk, BlockWriter& writer) {
    if (parse_header() == StateID::HEADER_OK) {
        #ifdef PRINT_STATES_FLOW
            std::cout << "\nFinished in state HEADER_OK\n\n";
        #endif
    }
    else {
        return false;
    }
    
    // check bounds
    if (x1_blk >= x2_blk || y1_blk >= y2_blk
                         || x2_blk - x1_blk > m_frame_info.width_blk
                         || y2_blk - y1_blk > m_frame_info.height_blk) {
        return false;
    }

    int block_8x8[64] {0};
    uint src_width_px = 8 * (x2_blk - x1_blk);
    uint src_height_px = 8 * (y2_blk - y1_blk);
    writer.init_frame(dst, src_width_px, src_height_px);

    for (uint row_blk = y1_blk; row_blk < y2_blk; ++row_blk) {
        uint luma_block_idx = row_blk * m_frame_info.width_blk + x1_blk;
        for (uint col_blk = x1_blk; col_blk < x2_blk; ++col_blk, ++luma_block_idx) {
            if (!m_huffman.decode_luma_block(m_reader, block_8x8, luma_block_idx, m_frame_info.horiz_chroma_subs_factor)) {
                std::cout << "\nHuffman decoding FAILED!" << "\n";
                return false;
            }

            m_dequantizer.transform(block_8x8);
            m_zigzag.transform(block_8x8);
            m_idct.transform(block_8x8);
            level_transform(block_8x8);
            writer.write(block_8x8);
        }
    }

    return true;
}

bool JpegDecoder::dc_decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk) {
    if (parse_header() == StateID::HEADER_OK) {
        #ifdef PRINT_STATES_FLOW
            std::cout << "\nFinished in state HEADER_OK\n\n";
        #endif
    }
    else {
        return false;
    }
    
    // check bounds
    if (x1_blk >= x2_blk || y1_blk >= y2_blk
                         || x2_blk - x1_blk > m_frame_info.width_blk
                         || y2_blk - y1_blk > m_frame_info.height_blk) {
        return false;
    }

    uint src_width_px = x2_blk - x1_blk;
    int block_8x8[64] {0};

    for (uint row_blk = y1_blk; row_blk < y2_blk; ++row_blk) {
        uint luma_block_idx = row_blk * m_frame_info.width_blk + x1_blk;
        for (uint col_blk = x1_blk; col_blk < x2_blk; ++col_blk, ++luma_block_idx) {
            if (!m_huffman.decode_luma_block(m_reader, block_8x8, luma_block_idx, m_frame_info.horiz_chroma_subs_factor)) {
                std::cout << "\nHuffman decoding FAILED!" << "\n";
                return false;
            }

            // dequantize only the DC coefficient
            m_dequantizer.transform(block_8x8[0]);

            // recover block-averaged LUMA value
            const int dc_luma = (block_8x8[0] + 1024) / 8;

            dst[(row_blk - y1_blk) * src_width_px + (col_blk - x1_blk)] = dc_luma;
        }
    }

    return true;
}

StateID JpegDecoder::parse_header() {
    while (!m_istate->is_final_state()) {
        m_istate->parse_header(m_reader);
    }

    return m_istate->getID();
}

void JpegDecoder::level_transform(int (&block)[64]) noexcept {
    for (uint i = 0; i < 64; ++i) {
        block[i] += 128;
    }
}
