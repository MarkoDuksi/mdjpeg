#include "JpegDecoder.h"

#include <iostream>


JpegDecoder::JpegDecoder(const uint8_t* const buff, const size_t size) noexcept :
    m_reader(buff, size),
    m_state(ConcreteState<StateID::ENTRY>(this)),
    m_istate(&m_state)
    {}

bool JpegDecoder::decode(uint8_t* const dst, uint16_t x1_mcu, uint16_t y1_mcu, uint16_t x2_mcu, uint16_t y2_mcu) {
    if (parse_header() == StateID::HEADER_OK) {
        std::cout << "\nFinished in state HEADER_OK\n";
    }
    else {
        return false;
    }
    
    // decode from (x1_mcu, y2_mcu) to the bottom-right corner by default
    if (!x2_mcu) {
        x2_mcu = m_header.img_mcu_horiz_count;
    }
    if (!y2_mcu) {
        y2_mcu = m_header.img_mcu_vert_count;
    }

    // check bounds
    if (x1_mcu >= x2_mcu || y1_mcu >= y2_mcu
                         || x2_mcu - x1_mcu > m_header.img_mcu_horiz_count
                         || y2_mcu - y1_mcu > m_header.img_mcu_vert_count) {
        return false;
    }

    uint dst_width = 8 * (x2_mcu - x1_mcu);
    int block_8x8[64] {0};

    for (uint mcu_row = y1_mcu; mcu_row < y2_mcu; ++mcu_row) {
        for (uint mcu_col = x1_mcu; mcu_col < x2_mcu; ++mcu_col) {
            if (!huff_decode_luma(block_8x8, mcu_row, mcu_col)) {
                std::cout << "\nHuffman decoding FAILED!" << "\n";
                return false;
            }

            m_dequantizer.transform(block_8x8);
            m_zigzag.transform(block_8x8);
            idct(block_8x8);
            level_to_unsigned(block_8x8);

            for (uint y = 0; y < 8; ++y) {
                for (uint x = 0; x < 8; ++x) {
                    dst[(8 * (mcu_row - y1_mcu) + y) * dst_width + 8 * (mcu_col - x1_mcu) + x] = block_8x8[8 * y + x];
                }
            }
        }
    }

    return true;
}

bool JpegDecoder::low_pass_decode(uint8_t* const dst, uint16_t x1_mcu, uint16_t y1_mcu, uint16_t x2_mcu, uint16_t y2_mcu) {
    if (parse_header() == StateID::HEADER_OK) {
        std::cout << "\nFinished in state HEADER_OK\n";
    }
    else {
        return false;
    }
    
    // decode from (x1_mcu, y2_mcu) to the bottom-right corner by default
    if (!x2_mcu) {
        x2_mcu = m_header.img_mcu_horiz_count;
    }
    if (!y2_mcu) {
        y2_mcu = m_header.img_mcu_vert_count;
    }

    // check bounds
    if (x1_mcu >= x2_mcu || y1_mcu >= y2_mcu
                         || x2_mcu - x1_mcu > m_header.img_mcu_horiz_count
                         || y2_mcu - y1_mcu > m_header.img_mcu_vert_count) {
        return false;
    }

    uint dst_width = x2_mcu - x1_mcu;
    int block_8x8[64] {0};

    for (uint mcu_row = y1_mcu; mcu_row < y2_mcu; ++mcu_row) {
        for (uint mcu_col = x1_mcu; mcu_col < x2_mcu; ++mcu_col) {
            if (!huff_decode_luma(block_8x8, mcu_row, mcu_col)) {
                std::cout << "\nHuffman decoding FAILED!" << "\n";
                return false;
            }

            // dequantize only the DC coefficient
            m_dequantizer.transform(block_8x8[0]);

            // recover block-averaged LUMA value
            const int low_pass_luma = (block_8x8[0] + 1024) / 8;

            dst[(mcu_row - y1_mcu) * dst_width + (mcu_col - x1_mcu)] = low_pass_luma;
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

uint8_t JpegDecoder::get_dc_huff_symbol(const uint table_id) noexcept {
    uint curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {
        const int next_bit = m_reader.read_bit();
        if (next_bit == ECS_ERROR) {
            return SYMBOL_ERROR;
        }

        curr_code |= next_bit;
        for (uint j = 0; j < m_header.htables[table_id].dc.histogram[i] && idx < 12; ++j) {
            if (curr_code == m_header.htables[table_id].dc.codes[idx]) {
                return m_header.htables[table_id].dc.symbols[idx];
            }
            ++idx;
        }
        curr_code <<= 1;
    }

    return 0xff;
}

uint8_t JpegDecoder::get_ac_huff_symbol(const uint table_id) noexcept {
    uint curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {
        const int next_bit = m_reader.read_bit();
        if (next_bit == ECS_ERROR) {
            return SYMBOL_ERROR;
        }

        curr_code = curr_code << 1 | next_bit;
        for (uint j = 0; j < m_header.htables[table_id].ac.histogram[i] && idx < 162; ++j) {
            if (curr_code == m_header.htables[table_id].ac.codes[idx]) {
                return m_header.htables[table_id].ac.symbols[idx];
            }
            ++idx;
        }
    }

    return 0xff;
}

int16_t JpegDecoder::get_dct_coeff(const uint length) noexcept {
    if (length > 16) {
        return COEF_ERROR;
    }

    int dct_coeff = 0;

    for (uint i = 0; i < length; ++i) {
        const int next_bit = m_reader.read_bit();
        if (next_bit == ECS_ERROR) {
            return COEF_ERROR;
        }
        dct_coeff = dct_coeff << 1 | next_bit;
    }

    // recover negative values
    if (length && dct_coeff >> (length - 1) == 0) {
        return dct_coeff - (1 << length) + 1;
    }

    return dct_coeff;
}

bool JpegDecoder::huff_decode_luma(int (&dst_block)[64], const uint mcu_row, const uint mcu_col) noexcept {
    const uint mcu_idx = mcu_row * m_header.img_mcu_horiz_count + mcu_col;

    // if already beyond the requested mcu_idx
    if (m_reader.m_mcu_idx > mcu_idx) {
        m_reader.restart_ecs();
    }

    while (m_reader.m_mcu_idx <= mcu_idx) {
        // following is true for any luma block in either 4:4:4 or 4:2:2 chroma subsampling modes
        if (m_reader.m_block_idx % (3 + m_header.horiz_subsampling) <= m_header.horiz_subsampling) {
            if (!huff_decode_block(dst_block, 0)) {
                return false;
            }
            dst_block[0] += m_reader.m_previous_luma_dc_coeff;
            m_reader.m_previous_luma_dc_coeff = dst_block[0];
            ++m_reader.m_mcu_idx;
        }
        // otherwise chroma block, read through and skip over
        else {
            if (!huff_decode_block(dst_block, 1)) {
                return false;
            }
        }
        ++m_reader.m_block_idx;
    }

    return true;
}

bool JpegDecoder::huff_decode_block(int (&dst_block)[64], const uint table_id) noexcept {
    // DC DCT coefficient
    const uint huff_symbol = get_dc_huff_symbol(table_id);
    if (huff_symbol > 11) {  // DC coefficient length out of range
        return false;
    }

    const int dct_coeff = get_dct_coeff(huff_symbol);
    if (dct_coeff == COEF_ERROR) {
        return false;
    }

    dst_block[0] = dct_coeff;

    // AC DCT coefficient
    uint idx = 1;
    while (idx < 64) {
        uint pre_zeros_count;
        const uint huff_symbol = get_ac_huff_symbol(table_id);

        if (huff_symbol == COEF_ERROR) {
            return false;
        }
        else if (huff_symbol == 0x00) {  // the rest of coefficients are 0
            break;
        }
        else if (huff_symbol == 0xf0) {
            pre_zeros_count = 16;
        }
        else {
            pre_zeros_count = huff_symbol >> 4;
        }

        if (idx + pre_zeros_count >= 64) {  // prevent dst_block[64] overflow
            return false;
        }

        while (pre_zeros_count--) {
            dst_block[idx++] = 0;
        }

        if (huff_symbol == 0xf0) {  // done with this symbol
            continue;
        }

        const uint dct_coeff_length = huff_symbol & 0xf;
        if (dct_coeff_length > 10) {  // AC coefficient length out of range
            return false;
        }

        const int dct_coeff = get_dct_coeff(dct_coeff_length);
        if (dct_coeff == COEF_ERROR) {
            return false;
        }

        dst_block[idx++] = dct_coeff;
    }

    while (idx < 64) {
        dst_block[idx++] = 0;
    }

    return true;
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
