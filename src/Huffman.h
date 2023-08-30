#pragma once

#include <sys/types.h>
#include <stdint.h>

#include "JpegReader.h"


class Huffman {
    public:
        size_t set_htable(JpegReader& reader, size_t max_read_length) noexcept;
        bool is_set() const noexcept;

        bool decode_luma_block(JpegReader& reader, int (&dst_block)[64], const uint luma_block_idx, uint horiz_chroma_subs_factor) noexcept;

    private:
        uint m_block_idx {0};
        uint m_luma_block_idx {0};
        int m_previous_luma_dc_coeff {0};

        template<size_t size>
        struct HuffmanTable {
            const uint8_t* histogram {nullptr};
            const uint8_t* symbols {nullptr};
            uint16_t codes[size] {0};

            bool is_set {false};
        };

        struct HuffmanTables {
            HuffmanTable<12> dc {};
            HuffmanTable<162> ac {};
        };

        HuffmanTables m_htables[2];

        uint8_t get_dc_symbol(JpegReader& reader, const uint table_id) noexcept;
        uint8_t get_ac_symbol(JpegReader& reader, const uint table_id) noexcept;
        int16_t get_dct_coeff(JpegReader& reader, const uint length) noexcept;

        bool decode_next_block(JpegReader& reader, int (&dst_block)[64], const uint table_id) noexcept;
};
