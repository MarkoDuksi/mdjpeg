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

            // index of first pixel in row buffer relevant for the current src block
            const uint floor_block_west = static_cast<uint>(block_west + float_max_error);

            // float values that are within float_max_error from nearest integral value are rounded to it
            if (block_west < floor_block_west || block_west - floor_block_west < float_max_error) {

                block_west = floor_block_west;
            }

            for (uint row = 0; row < 8; ++row) {

                // dst row index overlayed by north portion of src row
                const int floor_north = static_cast<uint>(north + float_max_error);

                // float values that are within float_max_error from nearest integral value are rounded to it
                if (north < floor_north || north - floor_north < float_max_error) {

                    north = floor_north;
                }

                // current dst row index offset from start of array
                const int vert_offset = DST_WIDTH_PX * floor_north;

                // south border of src row expressed in dst pixels
                float south = north + m_vert_scaling_factor;

                // dst row index overlayed by south portion of src row
                const int floor_south = static_cast<uint>(south + float_max_error);

                // float values that are within float_max_error from nearest integral value are rounded to it
                if (south < floor_south || south - floor_south < float_max_error) {

                    south = floor_south;
                }

                // (re)start row at block west border on each src row scan
                float west = block_west;

                // determine fraction (proportion) of src row (partially) overlaying northern dst row
                const float north_fraction = (floor_south != south && floor_south != floor_north)  // TODO: floor_north could be just north?
                                           ? (static_cast<float>(floor_south) - north) / m_vert_scaling_factor : 1;

                // if src and dst rows are aligned along their north borders (A0, B0, or C0) or
                // if starting scanning new block
                if (floor_north == north || row == 0) {

                    // carry over from column buffer
                    m_row_buffer[floor_block_west] += m_column_buffer[col_buff_idx];
                    m_column_buffer[col_buff_idx] = 0;
                }

                // if src row (partially) overlays 2 adjacent dst rows (A3, B3, or C3)
                if (north_fraction != 1) {

                    // carry over from south position in column buffer
                    m_edge_buffer = m_column_buffer[col_buff_idx + 1];
                    m_column_buffer[col_buff_idx + 1] = 0;
                }

                for (uint col = 0; col < 8; ++col) {

                    // src column east border X-coord expressed in dst columns X-coords
                    float east = west + m_horiz_scaling_factor;

                    // X-coord of dst column overlayed by west portion of src column
                    const int floor_west = static_cast<uint>(west + float_max_error);

                    // float values that are within float_max_error from nearest integral value are rounded to it
                    if (west < floor_west || west - floor_west < float_max_error) {

                        west = floor_west;
                    }

                    // X-coord of dst column overlayed by east portion of src column
                    const int floor_east = static_cast<uint>(east + float_max_error);

                    // float values that are within float_max_error from nearest integral value are rounded to it
                    if (east < floor_east || east - floor_east < float_max_error) {

                        east = floor_east;
                    }

                    // total src pixel value to be weight-distributed between up to 4 dst pixels that it potentially overlays
                    const float val = static_cast<float>(src_block[src_idx++]);

                    // if src and dst columns are aligned along their east borders (A)
                    if (east == floor_east) {

                        // if src and dst rows are aligned along their west borders (A1)
                        if (south == floor_south) {  // TODO: bool south_aligned = (south == floor_south)

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + (m_row_buffer[floor_west] + val) * m_val_norm_factor);
                            m_row_buffer[floor_west] = 0;
                            col_buff_idx += col == 7;
                        }

                        // if src row is otherwise contained within single dst row (A2)
                        else if (north_fraction == 1) {

                            m_row_buffer[floor_west] += val;
                        }

                        // else src row (partially) overlays 2 adjacent dst rows (A3)
                        else {

                            const float north_val = north_fraction * val;
                            const float south_val = val - north_val;

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + (m_row_buffer[floor_west] + north_val) * m_val_norm_factor);
                            m_row_buffer[floor_west] = m_edge_buffer + south_val;
                            m_edge_buffer = 0;
                            col_buff_idx += col == 7;
                        }
                    }

                    // if src column is otherwise contained within single dst column (B)
                    else if (floor_west == floor_east) {

                        // if src row is contained within single dst row (B1 or B2)
                        // TODO: refactor
                        if (north_fraction == 1) {

                            // if src and dst rows are aligned along their south borders (B1) *and* src pixel is last one in its row
                            if (south == floor_south && col == 7) {

                                m_column_buffer[col_buff_idx++] = m_row_buffer[floor_west] + val;
                                m_row_buffer[floor_west] = 0;
                            }

                            // otherwise B1 or B2 *and* src pixel is last one in its block
                            else if (row == 7 && col == 7) {

                                m_column_buffer[col_buff_idx++] = m_row_buffer[floor_west] + val;
                                m_row_buffer[floor_west] = 0;
                            }

                            // all other B1 or B2 cases
                            else {

                                m_row_buffer[floor_west] += val;
                            }
                        }

                        // else src row (partially) overlays 2 adjacent dst rows (B3)
                        else {

                            const float north_val = north_fraction * val;
                            const float south_val = val - north_val;

                            if (col == 7) {

                                m_column_buffer[col_buff_idx++] = m_row_buffer[floor_west] + north_val;
                                m_row_buffer[floor_west] = m_edge_buffer + south_val;
                                m_edge_buffer = 0;
                                
                                if (row == 7) {
                                    m_column_buffer[col_buff_idx] = m_row_buffer[floor_west];
                                    m_row_buffer[floor_west] = 0;
                                }
                            }
                            else {

                                m_row_buffer[floor_west] += north_val;
                                m_edge_buffer += south_val;
                            }
                        }
                    }

                    // else src column (partially) overlays 2 adjacent dst columns (C)
                    else {

                        const float west_val = (static_cast<float>(floor_east) - west) * val / m_horiz_scaling_factor;
                        const float east_val = val - west_val;

                        // TODO: C1 and C2 share most of their logic, can be combined
                        
                        // if src and dst rows are aligned along their south borders (C1)
                        if (south == floor_south) {

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + (m_row_buffer[floor_west] + west_val) * m_val_norm_factor);
                            m_row_buffer[floor_west] = 0;

                            if (col == 7) {

                                m_column_buffer[col_buff_idx++] += east_val;
                            }
                            else {

                                m_row_buffer[floor_east] += east_val;
                            }
                        }

                        // if src row is otherwise contained within single dst row (C2)
                        else if (north_fraction == 1) {

                            m_row_buffer[floor_west] += west_val;

                            if (col == 7) {

                                m_column_buffer[col_buff_idx] += east_val;
                            }
                            else {

                                m_row_buffer[floor_east] += east_val;
                            }
                        }

                        // else src row (partially) overlays 2 adjacent dst rows (C3)
                        else {

                            const float north_west_val = north_fraction * west_val;
                            const float south_west_val = west_val - north_west_val;
                            const float north_east_val = north_fraction * east_val;
                            const float south_east_val = east_val - north_east_val;

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + (m_row_buffer[floor_west] + north_west_val) * m_val_norm_factor);
                            m_row_buffer[floor_west] = m_edge_buffer + south_west_val;

                            if (col == 7) {

                                m_edge_buffer = 0;
                                m_column_buffer[col_buff_idx++] += north_east_val;
                                m_column_buffer[col_buff_idx] = south_east_val;
                            }
                            else {

                                m_row_buffer[floor_east] += north_east_val;
                                m_edge_buffer = south_east_val;
                            }
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

        float m_row_buffer[DST_WIDTH_PX] {};
        float m_column_buffer[8] {};
        float m_edge_buffer {};

        // errors smaller than this are expected with floating point math
        // and are checked for in certain places
        const float float_max_error = 0.0001f;
};
