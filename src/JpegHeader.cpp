#include "JpegHeader.h"

#include <fmt/core.h>


size_t JpegHeader::set_qtable(JpegReader& reader, const size_t max_read_length) noexcept {
    const uint next_byte = *reader.get_current_ptr();
    reader.seek(1);

    const uint precision = 1 + bool(next_byte >> 4);  // in bytes, not bits
    const uint table_id  = next_byte & 0xf;
    const uint table_size = 64 * precision;

    if (1 + table_size > max_read_length) {
        return 0;
    }

    // store only 8-bit luma quantization table pointer
    if (table_id == 0) {
        if (precision == 1) {
            qtable = reader.get_current_ptr();
        }
        else {
            return 0;
        }
    }

    reader.seek(table_size);

    return 1 + table_size;
}

size_t JpegHeader::set_htable(JpegReader& reader, size_t max_read_length) noexcept {
    const uint next_byte = *reader.get_current_ptr();
    --max_read_length;

    const bool is_dc = !(next_byte >> 4);
    const uint table_id = next_byte & 0xf;

    uint symbols_count = 0;

    for (uint i = 1; i < 17; ++i) {
        if (!max_read_length || reader.get_current_ptr()[i] > 1 << i) {
            return 0;
        }
        symbols_count += reader.get_current_ptr()[i];
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
        htables[table_id].dc.histogram = reader.get_current_ptr() + 1;
        htables[table_id].dc.symbols = reader.get_current_ptr() + 1 + 16;
        std::cout << "\nHuffman table id " << (int)table_id << " (DC):\n";
        std::cout << "(length: code -> symbol)\n";

        // generate Huffman codes for DC coeff length symbols
        uint curr_huff_code = 0;
        uint idx = 0;
        for (uint i = 0; i < 16; ++i) {
            curr_huff_code <<= 1;
            for (uint j = 0; j < htables[table_id].dc.histogram[i]; ++j) {
                fmt::print("  {: >2}: {:0>{}b} -> 0x{:0>2x}\n",
                            i + 1,
                            curr_huff_code, i + 1,
                            htables[table_id].dc.symbols[idx]);
                htables[table_id].dc.codes[idx++] = curr_huff_code++;
            }
        }
        htables[table_id].dc.is_set = true;
    }
    else {
        htables[table_id].ac.histogram = reader.get_current_ptr() + 1;
        htables[table_id].ac.symbols = reader.get_current_ptr() + 1 + 16;
        std::cout << "\nHuffman table id " << (int)table_id << " (AC):\n";
        std::cout << "(length: code -> symbol)\n";

        // generate Huffman codes for AC coeff length symbols
        uint curr_huff_code = 0;
        uint idx = 0;
        for (uint i = 0; i < 16; ++i) {
            curr_huff_code <<= 1;
            for (uint j = 0; j < htables[table_id].ac.histogram[i]; ++j) {
                fmt::print("  {: >2}: {:0>{}b} -> 0x{:0>2x}\n",
                            i + 1,
                            curr_huff_code, i + 1,
                            htables[table_id].ac.symbols[idx]);
                htables[table_id].ac.codes[idx++] = curr_huff_code++;
            }
        }
        htables[table_id].ac.is_set = true;
    }

    return 1 + 16 + symbols_count;
}

bool JpegHeader::is_set() const noexcept {
    return (qtable && htables[0].dc.is_set
                   && htables[0].ac.is_set
                   && htables[1].dc.is_set
                   && htables[1].ac.is_set
                   && img_height);
}
