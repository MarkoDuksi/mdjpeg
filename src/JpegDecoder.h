#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <new>

#include "states.h"
#include "JpegReader.h"
#include "Huffman.h"
#include "Dequantizer.h"
#include "BoundingBox.h"


class BlockWriter;


/// \brief Decoder class for JPEG File Interchange Format (JFIF) data.
///
/// Wraps a memory block containing compressed image data with specific
/// decompression (conveniently, if erroneously referred to as decoding)
/// functionalities. Supports decompressing only part of the frame specified
/// by region of interest and allows per-block transformations on the fly
/// without previously decompressing and buffering the intermediate subframe
/// for the whole region of interest. Does not allocate memory on the heap.
///
/// \note
/// - Supported mode is baseline JPEG.
/// - Supported color space is Y'CbCr.
/// - Supported chroma subsampling schemes are 4:4:4 and 4:2:2.
/// - Both spacial image dimensions must be multiples of 8 pixels.
/// - Region of interest must be defined using (8x8) blocks, not pixels.
/// - Output is luma channel only.
class JpegDecoder {

    public:

        /// \brief Sets its view on a compressed image data.
        ///
        /// \param buff  Start of memory block containing JFIF data.
        /// \param size  Size of the memory block in bytes.
        /// \retval      true on success.
        /// \retval      false on failure.
        ///
        /// Assignment is considered successful if and only if parsing of image
        /// JFIF header succeeds. Entropy-coded segment is not read during
        /// assignment.
        /// \attention:
        /// No data copying or ownership transfer takes place. The caller is
        /// responsible for
        /// - image data integrity for as long as it needs JpegDecoder to
        /// operate on it
        /// - making sure that the memory is not leaked afterwards (if allocated
        /// on the heap).
        bool assign(const uint8_t* buff, size_t size) noexcept;

        /// \brief Queries image width as read from the JFIF header.
        /// \return  Width in pixels if JFIF header is valid, 0 otherwise.
        uint16_t get_width() const noexcept {

           return m_has_valid_header ? m_frame_info.width_px : 0;
        }

        /// \brief Queries image height as read from the JFIF header.
        ///
        /// \return  Height in pixels if JFIF header is valid, 0 otherwise.
        uint16_t get_height() const noexcept {

            return m_has_valid_header ? m_frame_info.height_px : 0;
        }
    
        /// \brief Decompresses the luma channel, writing to raw pixel buffer via BasicBlockWriter by default.
        ///
        /// \param dst     Raw pixel buffer for decompressed output, min size is `64 * (x2_blk - x1_blk) * (y2_blk - y1_blk)`.
        /// \param roi_blk Coordinates for the region of interest expressed in 8x8 blocks.
        /// \retval        true on success.
        /// \retval        false on failure.
        bool luma_decode(uint8_t* dst, const BoundingBox& roi_blk) noexcept;

        /// \brief Decompresses the luma channel writing to raw pixel buffer via specified BlockWriter.
        ///
        /// \param dst     Raw pixel buffer for decompressed output, min size depending on particular BlockWriter.
        /// \param roi_blk Coordinates for the region of interest expressed in 8x8 blocks.
        /// \param writer  Specific implementation to use for writing decompressed data to raw pixel buffer.
        /// \retval        true on success.
        /// \retval        false on failure.
        bool luma_decode(uint8_t* dst, const BoundingBox& roi_blk, BlockWriter& writer) noexcept;

        /// \brief Decompresses 1:8 scaled-down luma channel to raw pixel buffer using DC DCT coefficients only.
        ///
        /// \param dst     Raw pixel buffer for decompressed output, min size is `(x2_blk - x1_blk) * (y2_blk - y1_blk)`.
        /// \param roi_blk Coordinates for the region of interest expressed in 8x8 blocks.
        /// \retval        true on success.
        /// \retval        false on failure.
        bool dc_luma_decode(uint8_t* dst, const BoundingBox& roi_blk) noexcept;

        template <StateID ANY>
        friend class ConcreteState;

    private:

        // image frame info as read from JFIF header
        struct FrameInfo {
            uint16_t height_px {};
            uint16_t width_px {};
            uint8_t  horiz_chroma_subs_factor {};

            void set(uint16_t height_px, uint16_t width_px, uint8_t horiz_chroma_subs_factor) noexcept;

            bool is_set() const noexcept {

                return horiz_chroma_subs_factor;
            }

            void clear() noexcept {

                height_px = 0;
                width_px = 0;
                horiz_chroma_subs_factor = 0;
            };

        } m_frame_info {};

        // indication of successful assignment
        bool m_has_valid_header {false};

        // initial state for the state machine that parses header information from JFIF header
        ConcreteState<StateID::ENTRY> m_state{this};  // 1 ptr

        // polymorphic handle, used as properly aligned pointer for transitioning through states via placement new
        State* m_istate {&m_state};

        // misc decoding utilities
        JpegReader m_reader {};        // 4 x ptr + 1 uint8
        Dequantizer m_dequantizer {};  // 1 ptr
        Huffman m_huffman {};          // large object (keep it last)

        // step function for the state machine that parses JFIF header
        StateID parse_header() noexcept;

        // set a specific state for the state machine that parses JFIF header
        template<StateID state_id>
        void set_state() noexcept {

            m_istate = new (m_istate) ConcreteState<state_id>(this);
        }
};
