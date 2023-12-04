#include <stdint.h>

#include "BlockWriter.h"


/// \brief Implements BlockWriter for block-wise writing of input to output in 1:1 scale.
class BasicBlockWriter : public BlockWriter {

    public:

        /// \brief Performs initialization.
        ///
        /// It is called before write() is called for the first input block of every
        /// new region of interest and should not be called again until the last block
        /// of that region has been written to destination buffer.
        ///
        /// \param dst            Raw pixel buffer for writing output to,
        ///                       minimum size is `src_width_px * src_height_px`.
        /// \param src_width_px   Width of the region of interest expressed in pixels.
        /// \param src_height_px  Height of the region of interest expressed in pixels.
        void init(uint8_t* const dst, const uint16_t src_width_px, [[maybe_unused]] const uint16_t src_height_px) noexcept override;
        
        /// \brief Performs a single block write.
        ///
        /// Each call performs an unbuffered write of all input block pixels to
        /// destination buffer.
        ///
        /// \param src_block  Input block.
        ///
        /// \note
        /// - Blocks from a particular region of interest are presumed served in the
        ///   order in which they appear in the entropy-coded segment.
        void write(int (&src_block)[64]) noexcept override;
};
