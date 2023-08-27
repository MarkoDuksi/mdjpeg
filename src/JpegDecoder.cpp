#include "JpegDecoder.h"

#include <iostream>


JpegDecoder::JpegDecoder(const uint8_t* const buff, const size_t size) noexcept :
    m_state(ConcreteState<StateID::ENTRY>(this)),
    m_istate(&m_state),
    m_reader(buff, size)
    {}

bool JpegDecoder::decode(uint8_t* const dst, uint x1_blocks, uint y1_blocks, uint x2_blocks, uint y2_blocks) {
    if (parse_header() == StateID::HEADER_OK) {
        std::cout << "\nFinished in state HEADER_OK\n";
    }
    else {
        return false;
    }
    
    // decode from (x1_blocks, y2_blocks) to the bottom-right corner by default
    if (!x2_blocks) {
        x2_blocks = m_frame_info.width_blocks;
    }
    if (!y2_blocks) {
        y2_blocks = m_frame_info.height_blocks;
    }

    // check bounds
    if (x1_blocks >= x2_blocks || y1_blocks >= y2_blocks
                               || x2_blocks - x1_blocks > m_frame_info.width_blocks
                               || y2_blocks - y1_blocks > m_frame_info.height_blocks) {
        return false;
    }

    uint dst_width = 8 * (x2_blocks - x1_blocks);
    int block_8x8[64] {0};

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

            for (uint y = 0; y < 8; ++y) {
                for (uint x = 0; x < 8; ++x) {
                    dst[(8 * (row_blocks - y1_blocks) + y) * dst_width + 8 * (col_blocks - x1_blocks) + x] = block_8x8[8 * y + x];
                }
            }
        }
    }

    return true;
}

bool JpegDecoder::low_pass_decode(uint8_t* const dst, uint x1_blocks, uint y1_blocks, uint x2_blocks, uint y2_blocks) {
    if (parse_header() == StateID::HEADER_OK) {
        std::cout << "\nFinished in state HEADER_OK\n";
    }
    else {
        return false;
    }
    
    // decode from (x1_blocks, y2_blocks) to the bottom-right corner by default
    if (!x2_blocks) {
        x2_blocks = m_frame_info.width_blocks;
    }
    if (!y2_blocks) {
        y2_blocks = m_frame_info.height_blocks;
    }

    // check bounds
    if (x1_blocks >= x2_blocks || y1_blocks >= y2_blocks
                               || x2_blocks - x1_blocks > m_frame_info.width_blocks
                               || y2_blocks - y1_blocks > m_frame_info.height_blocks) {
        return false;
    }

    uint dst_width = x2_blocks - x1_blocks;
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
            const int low_pass_luma = (block_8x8[0] + 1024) / 8;

            dst[(row_blocks - y1_blocks) * dst_width + (col_blocks - x1_blocks)] = low_pass_luma;
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
