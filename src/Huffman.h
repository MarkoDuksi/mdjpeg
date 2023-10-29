#pragma once

#include <sys/types.h>
#include <stdint.h>


class JpegReader;


/// \brief Provides %Huffman decoding facilities specific to JFIF data.
///
/// Reads %Huffman symbols from JFIF header and stores (pointers to) them.
/// Generates %Huffman codes. Decodes JFIF %Huffman stream block by block.
class Huffman {

    public:

        /// \brief Populates %Huffman tables starting at \c reader cursor.
        size_t set_htable(JpegReader& reader, size_t max_read_length) noexcept;

        /// \brief Checks if all DC/AC-luma/chroma %Huffman tables are populated.
        bool is_set() const noexcept;

        /// \brief Decodes a luma block by its index.
        bool decode_luma_block(JpegReader& reader, int (&dst_block)[64], const uint luma_block_idx, const uint horiz_chroma_subs_factor) noexcept;

    private:

        uint m_block_idx {};
        uint m_luma_block_idx {};
        int m_previous_luma_dc_coeff {};

        template<size_t size>
        struct HuffmanTable {
            const uint8_t* histogram {nullptr};
            const uint8_t* symbols {nullptr};
            uint16_t codes[size] {};

            bool is_set {false};
        };

        struct HuffmanTables {
            HuffmanTable<12> dc {};
            HuffmanTable<162> ac {};
        };

        HuffmanTables m_htables[2];

        uint8_t get_dc_symbol(JpegReader& reader, const uint table_id) const noexcept;
        uint8_t get_ac_symbol(JpegReader& reader, const uint table_id) const noexcept;
        int16_t get_dct_coeff(JpegReader& reader, const uint length) const noexcept;

        // decodes next block from the ECS, be it luma or chroma (specified via `table_id`)
        bool decode_next_block(JpegReader& reader, int (&dst_block)[64], const uint table_id) const noexcept;
};
