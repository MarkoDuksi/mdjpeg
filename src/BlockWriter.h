#pragma once

#include <stdint.h>
#include <sys/types.h>


class BlockWriter {
    public:
        virtual ~BlockWriter() = default;

        virtual void init_frame(uint8_t* const dst, uint src_width_px, [[maybe_unused]] uint src_height_px) = 0;
        virtual void write(int (&src_block)[64]) = 0;
};


class BasicBlockWriter : public BlockWriter {
    public:
        void init_frame(uint8_t* const dst, uint src_width_px, [[maybe_unused]] uint src_height_px) final {
            m_dst = dst;
            m_src_width_px = src_width_px;
            m_block_x = 0;
            m_block_y = 0;
        }
        
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

template <uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
class DownscalingBlockWriter : public BlockWriter {
    public:
        void init_frame(uint8_t* const dst, uint src_width_px, uint src_height_px) final {
            m_dst = dst;
            m_src_width_px = src_width_px;
            m_src_height_px = src_height_px;
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

        void write(int (&src_block)[64]) final {
            // src block west border X-coord expressed in dst pixels
            float block_west = m_horiz_scaling_factor * m_block_x;

            // src block north border Y-coord expressed in dst pixels
            float north = m_vert_scaling_factor * m_block_y;

            // 1D index (0-63) of pixels within 8x8 src block
            uint src_idx = 0;

            // column buffer index used to store/read dst block's easternmost pixels' partial values
            uint col_buff_idx = 0;

            // index of first pixel in row buffer relevant to the current src block
            const uint floor_block_west = round_and_floor_horiz(block_west);

            for (uint row = 0; row < 8; ++row) {

                // (re)start row at block west border on each src row scan
                float west = block_west;

                // dst row index overlayed by north portion of src row
                const int floor_north = round_and_floor_vert(north);

                // current dst row index offset from start of array
                const int vert_offset = DST_WIDTH_PX * floor_north;

                // south border of src row expressed in dst pixels
                float south = north + m_vert_scaling_factor;

                // dst row index overlayed by south portion of src row
                const int floor_south = round_and_floor_vert(south);

                // rows alignment
                const bool rows_are_south_aligned = floor_south == south;
                const bool src_row_spans_next_dst_row = floor_south != floor_north;

                // determine fraction (proportion) of src row (partially) overlaying northern dst row
                const float north_fraction = (!rows_are_south_aligned && src_row_spans_next_dst_row)
                                           ? (static_cast<float>(floor_south) - north) / m_vert_scaling_factor : 1.0f;

                // if src and dst rows are aligned along their north borders or if starting new block
                if (floor_north == north || row == 0) {

                    // carry over from north position column buffer
                    m_row_buffer[floor_block_west] += m_column_buffer[col_buff_idx];
                    m_column_buffer[col_buff_idx] = 0.0f;
                }

                // if src row (partially) overlays 2 adjacent dst rows (A3, B3, or C3)
                if (north_fraction != 1) {

                    // carry over from south position in column buffer
                    m_edge_buffer = m_column_buffer[col_buff_idx + 1];
                    m_column_buffer[col_buff_idx + 1] = 0.0f;
                }

                for (uint col = 0; col < 8; ++col) {

                    // src column east border X-coord expressed in dst columns X-coords
                    float east = west + m_horiz_scaling_factor;

                    // X-coord of dst column overlayed by west portion of src column
                    const int floor_west = round_and_floor_horiz(west);

                    // X-coord of dst column overlayed by east portion of src column
                    const int floor_east = round_and_floor_horiz(east);

                    // total src pixel value to be weight-distributed across up to 4 dst pixels that it potentially overlays
                    const float val = static_cast<float>(src_block[src_idx++]);

                    const float west_val = floor_east == floor_west ? val : (static_cast<float>(floor_east) - west) * val / m_horiz_scaling_factor;
                    const float east_val = val - west_val;

                    const float north_west_val = north_fraction * west_val;
                    const float south_west_val = west_val - north_west_val;
                    const float north_east_val = north_fraction * east_val;
                    const float south_east_val = east_val - north_east_val;

                    m_row_buffer[floor_west] += north_west_val;
                    m_edge_buffer += south_west_val;

                    // east-aligned columns and src row reaches up to or into next dst row
                    if (east == floor_east && src_row_spans_next_dst_row) {

                        m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + m_row_buffer[floor_west] * m_val_norm_factor);
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

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + m_row_buffer[floor_west] * m_val_norm_factor);
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
        uint m_src_height_px {};
        uint m_block_x {};
        uint m_block_y {};

        float m_horiz_scaling_factor {};
        float m_vert_scaling_factor {};
        float m_val_norm_factor {};
        float m_epsilon_horiz {};
        float m_epsilon_vert {};

        float m_row_buffer[DST_WIDTH_PX] {};
        float m_column_buffer[9] {};
        float m_edge_buffer {};


        // conditionally round input (in place) to float representation of nearest integer within +/- epsilon and return its floor
        uint round_and_floor_horiz(float& input) {

            const uint floored = static_cast<uint>(input + m_epsilon_horiz);

            if (input != floored && input - floored < m_epsilon_horiz) {

                input = floored;
            }

            return floored;
        }

        // conditionally round input (in place) to float representation of nearest integer within +/- epsilon and return its floor
        uint round_and_floor_vert(float& input) {

            const uint floored = static_cast<uint>(input + m_epsilon_vert);

            if (input != floored && input - floored < m_epsilon_vert) {

                input = floored;
            }

            return floored;
        }

};
