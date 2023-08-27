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
            idct(block_8x8);
            level_to_unsigned(block_8x8);

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

void JpegDecoder::idct(int (&block)[64]) noexcept {
// idct code from https://github.com/dannye/jed/blob/master/src/decoder.cpp
    for (uint i = 0; i < 8; ++i) {
        const float g0 = block[0 * 8 + i] * s0;
        const float g1 = block[4 * 8 + i] * s4;
        const float g2 = block[2 * 8 + i] * s2;
        const float g3 = block[6 * 8 + i] * s6;
        const float g4 = block[5 * 8 + i] * s5;
        const float g5 = block[1 * 8 + i] * s1;
        const float g6 = block[7 * 8 + i] * s7;
        const float g7 = block[3 * 8 + i] * s3;

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
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

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

        block[0 * 8 + i] = b0 + b7;
        block[1 * 8 + i] = b1 + b6;
        block[2 * 8 + i] = b2 + b5;
        block[3 * 8 + i] = b3 + b4;
        block[4 * 8 + i] = b3 - b4;
        block[5 * 8 + i] = b2 - b5;
        block[6 * 8 + i] = b1 - b6;
        block[7 * 8 + i] = b0 - b7;
    }

    for (uint i = 0; i < 8; ++i) {
        const float g0 = block[i * 8 + 0] * s0;
        const float g1 = block[i * 8 + 4] * s4;
        const float g2 = block[i * 8 + 2] * s2;
        const float g3 = block[i * 8 + 6] * s6;
        const float g4 = block[i * 8 + 5] * s5;
        const float g5 = block[i * 8 + 1] * s1;
        const float g6 = block[i * 8 + 7] * s7;
        const float g7 = block[i * 8 + 3] * s3;

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
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

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

        block[i * 8 + 0] = b0 + b7;
        block[i * 8 + 1] = b1 + b6;
        block[i * 8 + 2] = b2 + b5;
        block[i * 8 + 3] = b3 + b4;
        block[i * 8 + 4] = b3 - b4;
        block[i * 8 + 5] = b2 - b5;
        block[i * 8 + 6] = b1 - b6;
        block[i * 8 + 7] = b0 - b7;
    }
}

void JpegDecoder::level_to_unsigned(int (&block)[64]) noexcept {
    for (uint i = 0; i < 64; ++i) {
        block[i] += 128;
    }
}
