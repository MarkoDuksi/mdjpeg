#include "Huffman.h"

#include <iostream>
#include <fmt/core.h>


size_t Huffman::set_htable(JpegReader& reader, size_t max_read_length) noexcept {
    const uint next_byte = *reader.peek();
    --max_read_length;

    const bool is_dc = !(next_byte >> 4);
    const uint table_id = next_byte & 0xf;

    uint symbols_count = 0;

    for (uint i = 1; i < 17; ++i) {
        if (!max_read_length || *reader.peek(i) > 1 << i) {
            return 0;
        }
        symbols_count += *reader.peek(i);
        --max_read_length;
    }

    if (table_id > 1 || max_read_length == 0
                     || symbols_count == 0
                     || (is_dc && symbols_count > 12)
                     || symbols_count > 162
                     || symbols_count > max_read_length) {
        return 0;
    }

    if (is_dc) {
        m_htables[table_id].dc.histogram = reader.tell_ptr() + 1;
        m_htables[table_id].dc.symbols = reader.tell_ptr() + 1 + 16;
        std::cout << "\nHuffman table id " << (int)table_id << " (DC):\n";
        std::cout << "(length: code -> symbol)\n";

        // generate Huffman codes for DC coeff length symbols
        uint curr_huff_code = 0;
        uint idx = 0;
        for (uint i = 0; i < 16; ++i) {
            curr_huff_code <<= 1;
            for (uint j = 0; j < m_htables[table_id].dc.histogram[i]; ++j) {
                fmt::print("  {: >2}: {:0>{}b} -> 0x{:0>2x}\n",
                            i + 1,
                            curr_huff_code, i + 1,
                            m_htables[table_id].dc.symbols[idx]);
                m_htables[table_id].dc.codes[idx++] = curr_huff_code++;
            }
        }
        m_htables[table_id].dc.is_set = true;
    }
    else {
        m_htables[table_id].ac.histogram = reader.tell_ptr() + 1;
        m_htables[table_id].ac.symbols = reader.tell_ptr() + 1 + 16;
        std::cout << "\nHuffman table id " << (int)table_id << " (AC):\n";
        std::cout << "(length: code -> symbol)\n";

        // generate Huffman codes for AC coeff length symbols
        uint curr_huff_code = 0;
        uint idx = 0;
        for (uint i = 0; i < 16; ++i) {
            curr_huff_code <<= 1;
            for (uint j = 0; j < m_htables[table_id].ac.histogram[i]; ++j) {
                fmt::print("  {: >2}: {:0>{}b} -> 0x{:0>2x}\n",
                            i + 1,
                            curr_huff_code, i + 1,
                            m_htables[table_id].ac.symbols[idx]);
                m_htables[table_id].ac.codes[idx++] = curr_huff_code++;
            }
        }
        m_htables[table_id].ac.is_set = true;
    }

    return 1 + 16 + symbols_count;
}

bool Huffman::is_set() const noexcept {
    return (m_htables[0].dc.is_set && m_htables[0].ac.is_set
                                   && m_htables[1].dc.is_set
                                   && m_htables[1].ac.is_set);
}

uint8_t Huffman::get_dc_symbol(JpegReader& reader, const uint table_id) noexcept {
    uint curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {
        const int next_bit = reader.read_bit();
        if (next_bit == ECS_ERROR) {
            return SYMBOL_ERROR;
        }

        curr_code |= next_bit;
        for (uint j = 0; j < m_htables[table_id].dc.histogram[i] && idx < 12; ++j) {
            if (curr_code == m_htables[table_id].dc.codes[idx]) {
                return m_htables[table_id].dc.symbols[idx];
            }
            ++idx;
        }
        curr_code <<= 1;
    }

    return 0xff;
}

uint8_t Huffman::get_ac_symbol(JpegReader& reader, const uint table_id) noexcept {
    uint curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {
        const int next_bit = reader.read_bit();
        if (next_bit == ECS_ERROR) {
            return SYMBOL_ERROR;
        }

        curr_code = curr_code << 1 | next_bit;
        for (uint j = 0; j < m_htables[table_id].ac.histogram[i] && idx < 162; ++j) {
            if (curr_code == m_htables[table_id].ac.codes[idx]) {
                return m_htables[table_id].ac.symbols[idx];
            }
            ++idx;
        }
    }

    return 0xff;
}

int16_t Huffman::get_dct_coeff(JpegReader& reader, const uint length) noexcept {
    if (length > 16) {
        return COEF_ERROR;
    }

    int dct_coeff = 0;

    for (uint i = 0; i < length; ++i) {
        const int next_bit = reader.read_bit();
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

bool Huffman::decode_luma_block(JpegReader& reader, int (&dst_block)[64], const uint luma_block_idx, uint horiz_chroma_subs_factor) noexcept {
    // if already beyond the requested luma_block_idx
    if (m_luma_block_idx > luma_block_idx) {
        m_block_idx = 0;
        m_luma_block_idx = 0;
        m_previous_luma_dc_coeff = 0;
        reader.restart_ecs();
    }

    while (m_luma_block_idx <= luma_block_idx) {
        // following is true for any luma block in either 4:4:4 or 4:2:2 chroma subsampling modes
        if (m_block_idx % (2 + horiz_chroma_subs_factor) < horiz_chroma_subs_factor) {
            if (!decode_next_block(reader, dst_block, 0)) {
                return false;
            }
            dst_block[0] += m_previous_luma_dc_coeff;
            m_previous_luma_dc_coeff = dst_block[0];
            ++m_luma_block_idx;
        }
        // otherwise chroma block, read through and skip over
        else {
            if (!decode_next_block(reader, dst_block, 1)) {
                return false;
            }
        }
        ++m_block_idx;
    }

    return true;
}

bool Huffman::decode_next_block(JpegReader& reader, int (&dst_block)[64], const uint table_id) noexcept {
    // DC DCT coefficient
    const uint huff_symbol = get_dc_symbol(reader, table_id);
    if (huff_symbol > 11) {  // DC coefficient length out of range
        return false;
    }

    const int dct_coeff = get_dct_coeff(reader, huff_symbol);
    if (dct_coeff == COEF_ERROR) {
        return false;
    }

    dst_block[0] = dct_coeff;

    // AC DCT coefficient
    uint idx = 1;
    while (idx < 64) {
        uint pre_zeros_count;
        const uint huff_symbol = get_ac_symbol(reader, table_id);

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

        const int dct_coeff = get_dct_coeff(reader, dct_coeff_length);
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
