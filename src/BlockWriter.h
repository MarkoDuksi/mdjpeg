#pragma once

#include <stdint.h>
#include <sys/types.h>


/// \brief Abstract base class for block-wise writing of image data to memory.
class BlockWriter {

    public:

        virtual ~BlockWriter() = default;

        /// \brief Dispatches initialization.
        ///
        /// Every implementation should use this function's override to:
        /// - register a destination for their output
        /// - register basic information relevant to their task
        /// - (re)set initial conditions relevant to their task.
        ///
        /// \param dst            Raw pixel buffer for writing output to, min size
        ///                       depending on particular implementation.
        /// \param src_width_px   Width of the region of interest expressed in pixels.
        /// \param src_height_px  Height of the region of interest expressed in pixels.
        virtual void init(uint8_t* const dst, uint src_width_px, [[maybe_unused]] uint src_height_px) = 0;
        
        /// \brief Dispathes a single block write.
        ///
        /// Every implementation should use this function's override to iteratively
        /// perform the task of writing input blocks to their output destination.
        ///
        /// \param src_block  Input block.
        ///
        /// \note
        /// - Blocks from a particular region of interest are presumed served in the
        ///   order in which they appear in the entropy-coded segment.
        /// - It is not required for the writes to be unbuffered providing that all
        ///   expected output is written to the destination by the time the function
        ///   finishes with its last input block.
        virtual void write(int (&src_block)[64]) = 0;
};


/// \brief Implements BlockWriter for block-wise writing of input to output in 1:1 scale.
class BasicBlockWriter : public BlockWriter {

    public:

        /// \brief Performs initialization.
        ///
        /// It is called before write() is called for the first input block of every
        /// new region of interest and should not be called again until the last block
        /// of that region has been written to destination memory.
        ///
        /// \param dst            Raw pixel buffer for writing output to,
        ///                       minimum size is `src_width_px * src_height_px`.
        /// \param src_width_px   Width of the region of interest expressed in pixels.
        /// \param src_height_px  Height of the region of interest expressed in pixels.
        void init(uint8_t* const dst, uint src_width_px, [[maybe_unused]] uint src_height_px) final {

            m_dst = dst;
            m_src_width_px = src_width_px;
            m_block_x = 0;
            m_block_y = 0;
        }
        
        /// \brief Performs a single block write.
        ///
        /// Each call performs an unbuffered write of all input block pixels to
        /// destination memory.
        ///
        /// \param src_block  Input block.
        ///
        /// \note
        /// - Blocks from a particular region of interest are presumed served in the
        ///   order in which they appear in the entropy-coded segment.
        void write(int (&src_block)[64]) final {

            uint offset = m_block_y * m_src_width_px + m_block_x;
            uint src_idx = 0;

            for (uint row = 0; row < 8; ++row) {

                for (uint col = 0; col < 8; ++col) {

                    m_dst[offset + col] = src_block[src_idx++];
                }

                offset += m_src_width_px;
            }

            m_block_x += 8;

            if (m_block_x == m_src_width_px) {

                m_block_x = 0;
                m_block_y += 8;
            }
        }

    private:

        uint8_t* m_dst {nullptr};
        uint m_src_width_px {};
        uint m_block_x {};
        uint m_block_y {};
};

/// \brief Implements BlockWriter for block-wise writing with donwscaling.
///
/// \tparam DST_WIDTH_PX   Width of the destination image in pixels.
/// \tparam DST_HEIGHT_PX  Height of the destination image in pixels.
///
/// \note Output dimensions defined by these parameters need not be multiples of
/// 8 pixels. They must be greater than zero and no greater than corresponding
/// dimensions of source region of interest (see parameters to init()).
template <uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
// DST_HEIGHT_PX could have been passed as a regular parameter to `init`
// but is kept bundled here instead because of its close relation to
// DST_WIDTH_PX and no relation to `init` parameters. For all intended
// purposes the two template parameters are interdependent and no additional
// template instantiation occurs in normal use just because of the extra
// parameter.
class DownscalingBlockWriter : public BlockWriter {

    public:

        /// \brief Performs initialization.
        ///
        /// It is called before write() is called for the first input block of every
        /// new region of interest and should not be called again until the last block
        /// of that region has been written to destination memory.
        ///
        /// \param dst            Raw pixel buffer for writing output to,
        ///                       minimum size is `DST_WIDTH_PX * DST_HEIGHT_PX`.
        /// \param src_width_px   Width of the region of interest expressed in pixels.
        /// \param src_height_px  Height of the region of interest expressed in pixels.
        ///
        /// \note Both src dimensions must be at least as big as the corresponding
        /// destination dimensions. Otherwise it would not be a task of downscaling but
        /// rather upscaling. Additionally, since writing is done in blocks, both source
        /// dimensions must be multiples of 8 pixels.
        // `[[maybe_unused]]` added only to satisfy Doxygen (in fact, the parameter is allways used)
        void init(uint8_t* const dst, uint src_width_px, [[maybe_unused]] uint src_height_px) final {

            m_dst = dst;
            m_src_width_px = src_width_px;
            m_horiz_scaling_factor = static_cast<float>(DST_WIDTH_PX) / src_width_px;
            m_vert_scaling_factor = static_cast<float>(DST_HEIGHT_PX) / src_height_px;
            m_val_norm_factor = m_horiz_scaling_factor * m_vert_scaling_factor;
            m_epsilon_horiz = 1.0f / (src_width_px + 1);
            m_epsilon_vert = 1.0f / (src_height_px + 1);
            m_block_x = 0;
            m_block_y = 0;

            for (uint i = 0; i < DST_WIDTH_PX; ++i) {

                m_row_buffer[i] = 0.0f;
            }

            for (uint i = 0; i < 8; ++i) {

                m_column_buffer[i] = 0.0f;
            }

            m_edge_buffer = 0.0f;
        }

        /// \brief Performs a single block write with downscaling.
        ///
        /// Each call performs a partially buffered write of input block pixels to
        /// destination memory, downscaling the output according to specified
        /// source and destination dimensions. No source information is discarded
        /// in the downscaling process.
        ///
        /// \par Implementation Details
        ///
        /// Current downscaling algorithm is analogous to block-averaging but not
        /// limited to integral downscaling factors. Instead, true rational downscaling
        /// factors (horizontal and vertical separately) are approximated by floating
        /// point values with corrections to rounding errors if necessary.
        ///
        /// \param src_block  Input block.
        ///
        /// \note
        /// - Blocks from a particular region of interest are presumed served in the
        ///   order in which they appear in the entropy-coded segment.
        /// - All expected output is written to the destination by the time the function
        ///   finishes with its last input block.
        void write(int (&src_block)[64]) final {

            // src block west border X-coord expressed in dst pixels
            float block_west = m_horiz_scaling_factor * m_block_x;

            // src block north border Y-coord expressed in dst pixels
            float north = m_vert_scaling_factor * m_block_y;

            // correct minor floating point error
            north = snap_to_vert_grid(north);

            // 1D index (0-63) of pixels within 8x8 src block
            uint src_idx = 0;

            // column buffer index used for dst block's easternmost pixels' partial values
            uint col_buff_idx = 0;

            // correct minor floating point error
            block_west = snap_to_horiz_grid(block_west);

            // index of first pixel in row buffer relevant to the current src block
            const uint floor_block_west = static_cast<uint>(block_west);

            for (uint row = 0; row < 8; ++row) {

                // (re)start row at block west border on each src row scan
                float west = block_west;

                // dst row index overlayed by north portion of src row
                const int floor_north = static_cast<uint>(north);

                // current dst row index offset from start of array
                const int vert_offset = DST_WIDTH_PX * floor_north;

                // south border of src row expressed in dst pixels
                float south = north + m_vert_scaling_factor;

                // correct minor floating point error
                south = snap_to_vert_grid(south);

                // dst row index overlayed by south portion of src row
                const int floor_south = static_cast<uint>(south);

                // rows alignment
                const bool src_row_spans_next_dst_row = floor_south != floor_north;

                // determine fraction (proportion) of src col (partially) overlaying western dst col
                const float north_fraction = floor_south == floor_north || floor_south == south ?
                                             1.0f : (floor_south - north) / m_vert_scaling_factor;

                // if src and dst rows are aligned along their north borders or if starting new block
                if (floor_north == north || row == 0) {

                    // carry over from north position column buffer
                    m_row_buffer[floor_block_west] += m_column_buffer[col_buff_idx];
                    m_column_buffer[col_buff_idx] = 0.0f;
                }

                // if src row (partially) overlays 2 adjacent dst rows
                if (north_fraction != 1) {

                    // carry over from south position in column buffer
                    m_edge_buffer = m_column_buffer[col_buff_idx + 1];
                    m_column_buffer[col_buff_idx + 1] = 0.0f;
                }

                for (uint col = 0; col < 8; ++col) {

                    // src column east border X-coord expressed in dst columns X-coords
                    float east = west + m_horiz_scaling_factor;

                    // X-coord of dst column overlayed by west portion of src column
                    const uint floor_west = static_cast<uint>(west);

                    // correct minor floating point error
                    east = snap_to_horiz_grid(east);

                    // X-coord of dst column overlayed by east portion of src column
                    const uint floor_east = static_cast<uint>(east);

                    // total src pixel value to be weight-distributed across up to 4 dst pixels that it potentially overlays
                    const float val = src_block[src_idx++];

                    // determine fraction (proportion) of src col (partially) overlaying western dst col
                    const float west_fraction = floor_east == floor_west ?
                                                1.0f : (floor_east - west) / m_horiz_scaling_factor;

                    // distribute value horizontally
                    const float west_val = west_fraction * val;
                    const float east_val = val - west_val;

                    // further distribute the pair of values vertically
                    const float north_west_val = north_fraction * west_val;
                    const float south_west_val = west_val - north_west_val;
                    const float north_east_val = north_fraction * east_val;
                    const float south_east_val = east_val - north_east_val;

                    m_row_buffer[floor_west] += north_west_val;
                    m_edge_buffer += south_west_val;

                    // east-aligned columns *and* src row reaches up to or into next dst row
                    if (east == floor_east && src_row_spans_next_dst_row) {

                        const int dst_val = static_cast<int>(0.5f + m_row_buffer[floor_west] * m_val_norm_factor);
                        m_dst[vert_offset + floor_west] = dst_val <= 255 ? dst_val : 255;
                        m_row_buffer[floor_west] = m_edge_buffer;
                        m_edge_buffer = 0.0f;
                        col_buff_idx += col == 7;
                    }

                    // src column is otherwise contained within single dst column
                    else if (floor_west == floor_east) {

                        if  (col == 7) {

                            if (src_row_spans_next_dst_row || row == 7) {

                                m_column_buffer[col_buff_idx++] = m_row_buffer[floor_west];
                                m_row_buffer[floor_west] = m_edge_buffer;
                                m_edge_buffer = 0.0f;
                            }

                            if (north_fraction != 1 && row == 7) {

                                m_column_buffer[col_buff_idx] = m_row_buffer[floor_west];
                                m_row_buffer[floor_west] = 0.0f;
                            }
                        }
                    }

                    // src column (partially) overlays 2 adjacent dst columns
                    else {

                        if (src_row_spans_next_dst_row) {

                            const int dst_val = static_cast<int>(0.5f + m_row_buffer[floor_west] * m_val_norm_factor);
                            m_dst[vert_offset + floor_west] = dst_val <= 255 ? dst_val : 255;
                            m_row_buffer[floor_west] = m_edge_buffer;
                        }

                        if (col == 7) {

                            m_edge_buffer = 0.0f;
                            m_column_buffer[col_buff_idx] += north_east_val;
                            col_buff_idx += src_row_spans_next_dst_row;
                            m_column_buffer[col_buff_idx] += south_east_val;
                        }

                        else {

                            m_row_buffer[floor_east] += north_east_val;
                            m_edge_buffer = south_east_val;
                        }
                    }

                    // advance dst pixel west border by horizontal scaling factor
                    west = east;
                }

                // advance dst pixel north border by vertical scaling factor
                north = south;
            }

            m_block_x += 8;

            if (m_block_x == m_src_width_px) {

                m_block_x = 0;
                m_block_y += 8;
            }
        }

    private:

        uint8_t* m_dst {nullptr};
        uint m_src_width_px {};
        uint m_block_x {};
        uint m_block_y {};

        float m_horiz_scaling_factor {};
        float m_vert_scaling_factor {};
        float m_val_norm_factor {};
        float m_epsilon_horiz {};
        float m_epsilon_vert {};

        float m_row_buffer[DST_WIDTH_PX] {};
        float m_column_buffer[9] {};  // size is 9 because it's one greater than block height in pixels
        float m_edge_buffer {};

        // snap floating point input to integer grid if within `m_epsilon_horiz` proximity
        float snap_to_horiz_grid(float input) const {

            const uint floored = static_cast<uint>(input + m_epsilon_horiz);

            return (input != floored && input - floored < m_epsilon_horiz) ? floored : input;
        }

        // snap floating point input to integer grid if within `m_epsilon_vert` proximity
        float snap_to_vert_grid(float input) const {

            const uint floored = static_cast<uint>(input + m_epsilon_vert);

            return (input != floored && input - floored < m_epsilon_vert) ? floored : input;
        }
};
