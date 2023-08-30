#include "JpegDecoder.h"

#include <iostream>


JpegDecoder::JpegDecoder(const uint8_t* const buff, const size_t size) noexcept :
    m_state(ConcreteState<StateID::ENTRY>(this)),
    m_istate(&m_state),
    m_reader(buff, size)
    {}

bool JpegDecoder::decode(uint8_t* const dst, uint x1_blocks, uint y1_blocks, uint x2_blocks, uint y2_blocks) {
    DirectBlockWriter writer;
    return decode(dst, x1_blocks, y1_blocks, x2_blocks, y2_blocks, writer);
}

bool JpegDecoder::decode(uint8_t* const dst, uint x1_blocks, uint y1_blocks, uint x2_blocks, uint y2_blocks, BlockWriter& writer) {
    if (parse_header() == StateID::HEADER_OK) {
        std::cout << "\nFinished in state HEADER_OK\n";
    }
    else {
        return false;
    }
    
    // check bounds
    if (x1_blocks >= x2_blocks || y1_blocks >= y2_blocks
                               || x2_blocks - x1_blocks > m_frame_info.width_blocks
                               || y2_blocks - y1_blocks > m_frame_info.height_blocks) {
        return false;
    }

    int block_8x8[64] {0};
    uint src_width_px = 8 * (x2_blocks - x1_blocks);
    uint src_height_px = 8 * (y2_blocks - y1_blocks);
    writer.init_frame(dst, src_width_px, src_height_px);

    for (uint row_blocks = y1_blocks; row_blocks < y2_blocks; ++row_blocks) {
        uint luma_block_idx = row_blocks * m_frame_info.width_blocks + x1_blocks;
        for (uint col_blocks = x1_blocks; col_blocks < x2_blocks; ++col_blocks, ++luma_block_idx) {
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

bool JpegDecoder::dc_decode(uint8_t* const dst, uint x1_blocks, uint y1_blocks, uint x2_blocks, uint y2_blocks) {
    if (parse_header() == StateID::HEADER_OK) {
        std::cout << "\nFinished in state HEADER_OK\n";
    }
    else {
        return false;
    }
    
    // check bounds
    if (x1_blocks >= x2_blocks || y1_blocks >= y2_blocks
                               || x2_blocks - x1_blocks > m_frame_info.width_blocks
                               || y2_blocks - y1_blocks > m_frame_info.height_blocks) {
        return false;
    }

    uint src_width_px = x2_blocks - x1_blocks;
    int block_8x8[64] {0};

    for (uint row_blocks = y1_blocks; row_blocks < y2_blocks; ++row_blocks) {
        uint luma_block_idx = row_blocks * m_frame_info.width_blocks + x1_blocks;
        for (uint col_blocks = x1_blocks; col_blocks < x2_blocks; ++col_blocks, ++luma_block_idx) {
            if (!m_huffman.decode_luma_block(m_reader, block_8x8, luma_block_idx, m_frame_info.horiz_chroma_subs_factor)) {
                std::cout << "\nHuffman decoding FAILED!" << "\n";
                return false;
            }

            // dequantize only the DC coefficient
            m_dequantizer.transform(block_8x8[0]);

            // recover block-averaged LUMA value
            const int dc_luma = (block_8x8[0] + 1024) / 8;

            dst[(row_blocks - y1_blocks) * src_width_px + (col_blocks - x1_blocks)] = dc_luma;
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
