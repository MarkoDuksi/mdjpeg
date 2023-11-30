#include "Huffman.h"

#ifdef PRINT_HUFFMAN_TABLES
    #include <iostream>
    #include <fmt/core.h>
#endif

#include "JpegReader.h"
#include "ReadError.h"


uint16_t Huffman::set_htable(JpegReader& reader, uint16_t max_read_length) noexcept {

    const uint8_t next_byte = *reader.read_uint8();
    --max_read_length;

    const uint8_t is_ac = next_byte >> 4;
    const uint8_t table_id = next_byte & 0xf;

    uint symbols_count = 0;

    for (uint i = 0; i < 16; ++i) {

        if (!max_read_length || *reader.peek(i) > 1 << (i + 1)) {

            return 0;
        }

        symbols_count += *reader.peek(i);
        --max_read_length;
    }

    if (table_id > 1 || max_read_length == 0
                     || symbols_count == 0
                     || (!is_ac && symbols_count > 12)
                     || symbols_count > 162
                     || symbols_count > max_read_length) {

        return 0;
    }

    HuffmanTable& huff_table = m_htables[table_id][is_ac];
    huff_table.histogram = reader.tell_ptr();
    reader.seek(16);
    huff_table.symbols = reader.tell_ptr();

    #ifdef PRINT_HUFFMAN_TABLES
        std::cout << "\nHuffman table id " << (int)table_id;

        if (is_ac) {

        std::cout << " (AC):\n";
        }

        else {

        std::cout << " (DC):\n";
        }

        std::cout << "(length: code -> symbol)\n";
    #endif

    // generate Huffman codes for DCT coefficient length symbols
    uint16_t curr_huff_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {

        curr_huff_code <<= 1;

        for (uint j = 0; j < huff_table.histogram[i]; ++j) {

            huff_table.codes[idx] = curr_huff_code;

            #ifdef PRINT_HUFFMAN_TABLES
                fmt::print("  {: >2}: {:0>{}b} -> 0x{:0>2x}\n",
                            i + 1,
                            curr_huff_code, i + 1,
                            huff_table.symbols[idx]);
            #endif

            ++idx;
            ++curr_huff_code;
        }
    }

    huff_table.is_set = true;

    reader.seek(symbols_count);

    return 1 + 16 + symbols_count;
}

bool Huffman::is_set() const noexcept {

    return (m_htables[0].dc.is_set && m_htables[0].ac.is_set
                                   && m_htables[1].dc.is_set
                                   && m_htables[1].ac.is_set);
}

void Huffman::clear() noexcept {
    m_htables[0].dc.is_set = false;
    m_htables[0].ac.is_set = false;
    m_htables[1].dc.is_set = false;
    m_htables[1].ac.is_set = false;

}

uint8_t Huffman::get_symbol(JpegReader& reader, const uint8_t table_id, const uint8_t is_ac) const noexcept {

    const HuffmanTable& huff_table = m_htables[table_id][is_ac];
    const uint8_t symbols_count = is_ac > 0 ? 162 : 12;

    uint16_t curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {

        const int8_t next_bit = reader.read_bit();

        if (next_bit == static_cast<int8_t>(ReadError::ECS_BIT)) {

            return static_cast<uint8_t>(ReadError::HUFF_SYMBOL);
        }

        curr_code = curr_code << 1 | next_bit;

        for (uint j = 0; j < huff_table.histogram[i] && idx < symbols_count; ++j) {

            if (curr_code == huff_table.codes[idx]) {

                return huff_table.symbols[idx];
            }

            ++idx;
        }
    }

    return static_cast<uint8_t>(ReadError::HUFF_SYMBOL);
}

int16_t Huffman::get_dct_coeff(JpegReader& reader, const uint8_t length) noexcept {

    if (length > 16) {

        return static_cast<int16_t>(ReadError::DCT_COEF);
    }

    int16_t dct_coeff = 0;

    for (uint i = 0; i < length; ++i) {

        const int8_t next_bit = reader.read_bit();

        if (next_bit == static_cast<int8_t>(ReadError::ECS_BIT)) {

            return static_cast<int16_t>(ReadError::DCT_COEF);
        }

        dct_coeff = dct_coeff << 1 | next_bit;
    }

    // recover negative values
    if (length && dct_coeff >> (length - 1) == 0) {

        return dct_coeff - (1 << length) + 1;
    }

    return dct_coeff;
}

bool Huffman::decode_luma_block(JpegReader& reader, int (&dst_block)[64], const uint32_t luma_block_idx, const uint8_t horiz_chroma_subs_factor) noexcept {

    // if already beyond the requested `luma_block_idx`
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
        else if (!decode_next_block(reader, dst_block, 1)) {

            return false;
        }

        ++m_block_idx;
    }

    return true;
}

bool Huffman::decode_next_block(JpegReader& reader, int (&dst_block)[64], const uint8_t table_id) const noexcept {

    const uint8_t dc = 0;
    const uint8_t ac = 1;

    ////////////////////////////////
    // process DC DCT coefficient //

    const uint8_t dc_huff_symbol = get_symbol(reader, table_id, dc);

    // DC DCT coefficient length out of range
    if (dc_huff_symbol > 11) {

        return false;
    }

    const int16_t dc_dct_coeff = get_dct_coeff(reader, dc_huff_symbol);

    if (dc_dct_coeff == static_cast<int16_t>(ReadError::DCT_COEF)) {

        return false;
    }

    dst_block[0] = dc_dct_coeff;

    /////////////////////////////////
    // process AC DCT coefficients //

    uint idx = 1;

    while (idx < 64) {

        const uint8_t ac_huff_symbol = get_symbol(reader, table_id, ac);

        if (ac_huff_symbol == static_cast<uint8_t>(ReadError::HUFF_SYMBOL)) {

            return false;
        }

        // 0x00 means the rest of coefficients are 0
        if (ac_huff_symbol == 0x00) {

            break;
        }

        uint8_t pre_zeros_count = ac_huff_symbol == 0xf0 ? 16 : ac_huff_symbol >> 4;

        // prevent `dst_block` overflow
        if (idx + pre_zeros_count >= 64) {

            return false;
        }

        while (pre_zeros_count--) {

            dst_block[idx++] = 0;
        }

        // nothing more to do for 0xf0
        if (ac_huff_symbol == 0xf0) {

            continue;
        }

        const uint8_t ac_dct_coeff_length = ac_huff_symbol & 0xf;

        // AC DCT coefficient length out of range
        if (ac_dct_coeff_length > 10) {

            return false;
        }

        const int16_t ac_dct_coeff = get_dct_coeff(reader, ac_dct_coeff_length);

        if (ac_dct_coeff == static_cast<int16_t>(ReadError::DCT_COEF)) {

            return false;
        }

        dst_block[idx++] = ac_dct_coeff;
    }

    // zero-fill the rest of the block after 0x00
    while (idx < 64) {

        dst_block[idx++] = 0;
    }

    return true;
}
