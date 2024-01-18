#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <cmath>


namespace mdjpeg {

/// \brief Abstract base class for block-wise writing of image data to memory.
class BlockWriter {

    protected:

        ~BlockWriter() = default;

    public:

        BlockWriter() = default;
        BlockWriter(const BlockWriter& other) = delete;
        BlockWriter& operator=(const BlockWriter& other) = delete;
        BlockWriter(BlockWriter&& other) = delete;
        BlockWriter& operator=(BlockWriter&& other) = delete;

        /// \brief Dispatches initialization.
        ///
        /// \param dst            Raw pixel buffer for writing output to, min size
        ///                       depending on particular implementation.
        /// \param src_width_px   Width of the region of interest expressed in pixels.
        /// \param src_height_px  Height of the region of interest expressed in pixels.
        ///
        /// Every implementation should use this function's override to:
        /// - register a destination for their output
        /// - register basic information relevant to their task
        /// - (re)set initial conditions relevant to their task.
        virtual void init(uint8_t* dst, uint16_t src_width_px, [[maybe_unused]] uint16_t src_height_px) noexcept = 0;

        /// \brief Dispatches a single block write.
        ///
        /// \param src_block  Input block.
        ///
        /// Every implementation should use this function's override to
        /// iteratively perform the task of writing input blocks to their output
        /// destination.
        ///
        /// \note
        /// - Blocks from a particular region of interest are presumed served in
        ///   the order in which they appear in the entropy-coded segment.
        /// - It is not required for the writes to be unbuffered providing that
        ///   all expected output is written to the destination by the time the
        ///   function finishes with its last input block.
        virtual void write(int (&src_block)[64]) noexcept = 0;

    protected:

        uint8_t* m_dst {nullptr};
        uint16_t m_src_width_px {};
        uint16_t m_block_x {};
        uint16_t m_block_y {};
};

}  // namespace mdjpeg
