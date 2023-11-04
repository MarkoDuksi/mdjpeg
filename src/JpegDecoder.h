#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "states.h"
#include "JpegReader.h"
#include "Huffman.h"
#include "Dequantizer.h"


class BlockWriter;


/// \brief Decoder class for JPEG File Interchange Format (JFIF) data.
///
/// Wraps a memory segment containing compressed image data with specific
/// decompression (conveniently, if erroneously referred to as decoding)
/// functionalities. Supports decompressing only part of the frame specified
/// by region of interest and allows per-block transformations on the fly
/// without previously decompressing and buffering the intermediate subframe
/// for the whole region of interest. Does not allocate memmory on the heap.
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
        /// \param buff  Start of memory segment containing JFIF data.
        /// \param size  Length of the memory segment in bytes.
        ///
        /// \attention:
        /// No data copying or ownership transfer takes place. The caller is
        /// responsible for
        /// - image data integrity for as long as it needs JpegDecoder to operate on it
        /// - making sure that the memory is not leaked afterwards if allocated on the heap.
        void assign(const uint8_t* buff, const size_t size) noexcept;
        
        /// \brief Decompresses the luma channel, writing to raw pixel buffer via BasicBlockWriter by default.
        /// \param dst     Raw pixel buffer for decompressed output, min size is `64 * (x2_blk - x1_blk) * (y2_blk - y1_blk)`.
        /// \param x1_blk  X-coordinate of the top-left corner of the region of interest expressed in 8x8 blocks.
        /// \param y1_blk  Y-coordinate of the top-left corner of the region of interest expressed in 8x8 blocks.
        /// \param x2_blk  X-coordinate of the bottom-right corner of the region of interest expressed in 8x8 blocks.
        /// \param y2_blk  Y-coordinate of the bottom-right corner of the region of interest expressed in 8x8 blocks.
        /// \retval        true on success.
        /// \retval        false on failure.
        bool luma_decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk);

        /// \brief Decompresses the luma channel writing to raw pixel buffer via specified BlockWriter.
        /// \param dst     Raw pixel buffer for decompressed output, min size depending on particular BlockWriter.
        /// \param x1_blk  X-coordinate of the top-left corner of the region of interest expressed in 8x8 blocks.
        /// \param y1_blk  Y-coordinate of the top-left corner of the region of interest expressed in 8x8 blocks.
        /// \param x2_blk  X-coordinate of the bottom-right corner of the region of interest expressed in 8x8 blocks.
        /// \param y2_blk  Y-coordinate of the bottom-right corner of the region of interest expressed in 8x8 blocks.
        /// \param writer  Specific implementation to use for writing decompressed data to raw pixel buffer.
        /// \retval        true on success.
        /// \retval        false on failure.
        bool luma_decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk, BlockWriter& writer);

        /// \brief Decompresses 1:8 scaled-down luma channel to raw pixel buffer using DC DCT coefficients only.
        /// \param dst     Raw pixel buffer for decompressed output, min size is `(x2_blk - x1_blk) * (y2_blk - y1_blk)`.
        /// \param x1_blk  X-coordinate of the top-left corner of the region of interest expressed in 8x8 blocks.
        /// \param y1_blk  Y-coordinate of the top-left corner of the region of interest expressed in 8x8 blocks.
        /// \param x2_blk  X-coordinate of the bottom-right corner of the region of interest expressed in 8x8 blocks.
        /// \param y2_blk  Y-coordinate of the bottom-right corner of the region of interest expressed in 8x8 blocks.
        /// \retval        true on success.
        /// \retval        false on failure.
        bool dc_luma_decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk);

        template <StateID ANY>
        friend class ConcreteState;

    private:

        // initial state for the state machine that parses header information from JFIF header
        ConcreteState<StateID::ENTRY> m_state {ConcreteState<StateID::ENTRY>(this)};

        // polymorphic handle, used as properly aligned pointer for transitioning through states via placement new
        State* m_istate {&m_state};

        JpegReader m_reader {};
        Huffman m_huffman {};
        Dequantizer m_dequantizer {};

        // image frame dimensions info as read from JFIF header
        struct FrameInfo {
            uint height_px {};
            uint width_px {};
            uint height_blk {};
            uint width_blk {};
            uint horiz_chroma_subs_factor {};

            void set_dims(uint height_px, uint width_px) {

                this->height_px = height_px;
                this->width_px = width_px;
                height_blk = (height_px + 7) / 8;
                width_blk = (width_px + 7) / 8;
            }

            bool is_set() const noexcept {

                return height_px;
            }
        } m_frame_info {};

        // step function for the state machine that parses JFIF header
        StateID parse_header();

        // set a specific state for the state machine that parses JFIF header
        template<StateID state_id>
        void set_state() noexcept {

            m_istate = new (m_istate) ConcreteState<state_id>(this);
        }
};
