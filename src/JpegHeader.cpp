#include "JpegHeader.h"

#include <fmt/core.h>


size_t JpegHeader::set_htable(JpegReader& reader, size_t max_read_length) noexcept {
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
        htables[table_id].dc.histogram = reader.tell_ptr() + 1;
        htables[table_id].dc.symbols = reader.tell_ptr() + 1 + 16;
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
        htables[table_id].ac.histogram = reader.tell_ptr() + 1;
        htables[table_id].ac.symbols = reader.tell_ptr() + 1 + 16;
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
    return (img_height && htables[0].dc.is_set
                       && htables[0].ac.is_set
                       && htables[1].dc.is_set
                       && htables[1].ac.is_set);
}