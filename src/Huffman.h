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

        /// \brief   Populates %Huffman tables starting at \c reader cursor.
        /// \return  Number of bytes read through from the JFIF segment
        uint16_t set_htable(JpegReader& reader, uint16_t max_read_length) noexcept;

        /// \brief   Checks if all (DC/AC-luma/chroma) %Huffman tables are validly set.
        bool is_set() const noexcept;

        /// \brief   Invalidates all (DC/AC-luma/chroma) %Huffman tables even if populated.
        void clear() noexcept;

        /// \brief   Decodes a luma block by its index.
        /// \retval  true on success.
        /// \retval  false on failure.
        bool decode_luma_block(JpegReader& reader, int (&dst_block)[64], uint32_t luma_block_idx, uint8_t horiz_chroma_subs_factor) noexcept;

    private:

        uint32_t m_block_idx {};
        uint32_t m_luma_block_idx {};
        int m_previous_luma_dc_coeff {};

        struct HuffmanTable {

            explicit HuffmanTable (uint16_t* const codes_buff) :
                codes(codes_buff)
                {}

            const uint8_t* histogram {nullptr};
            const uint8_t* symbols {nullptr};
            uint16_t* const codes {nullptr};

            bool is_set {false};
        };

        template<size_t size>
        struct StaticHuffmanTable : public HuffmanTable {

            StaticHuffmanTable() :
                HuffmanTable(&codes_buff[0])
                {}

            uint16_t codes_buff[size] {};
        };

        struct HuffmanTables {
            StaticHuffmanTable<12> dc {};
            StaticHuffmanTable<162> ac {};

            const HuffmanTable& operator[](uint8_t idx) const {

                if (idx == 0) {
                    
                    return dc;
                }

                return ac;
            }

            HuffmanTable& operator[](uint8_t idx) {

                if (idx == 0) {
                    
                    return dc;
                }

                return ac;
            }
        };

        HuffmanTables m_htables[2];

        // decodes next block from the ECS, be it luma or chroma (specified via `table_id`)
        bool decode_next_block(JpegReader& reader, int (&dst_block)[64], uint8_t table_id) const noexcept;

        uint8_t get_symbol(JpegReader& reader, uint8_t table_id, uint8_t is_ac) const noexcept;

        static int16_t get_dct_coeff(JpegReader& reader, uint8_t length) noexcept;
};
