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
            const float block_west = m_horiz_scaling_factor * m_block_x;

            // src block north border Y-coord expressed in dst pixels
            float north = m_vert_scaling_factor * m_block_y;

            // 1D index (0-63) of pixels within 8x8 src block
            uint src_idx = 0;

            // column buffer index used to store/read block's easternmost pixels' partial values
            uint col_buff_idx = 0;

            // index of first dst pixel in row buffer (partially) overlayed by current block
            const uint floor_block_west = static_cast<uint>(block_west);

            for (uint row = 0; row < 8; ++row) {

                // dst row index overlayed by north portion of src row
                const int floor_north = static_cast<uint>(north);

                // current dst row index offset from start of array
                const int vert_offset = DST_WIDTH_PX * floor_north;

                // south border of src row expressed in dst pixels
                const float south = north + m_vert_scaling_factor;

                // dst row index overlayed by south portion of src row
                const int floor_south = static_cast<uint>(south);

                // determine fraction (proportion) of src row (partially) overlaying northern dst row
                const float north_fraction = (floor_south != south && floor_south != floor_north)
                                           ? (static_cast<float>(floor_south) - north) / m_vert_scaling_factor : 1;

                // if initial scan of dst row (subsequent scans of same dst row should skip this)
                if (north - floor_north < m_vert_scaling_factor) {  // TODO: bug?

                    // carryover from column buffer to row buffer
                    m_row_buffer[floor_block_west] += m_column_buffer[col_buff_idx];
                    m_column_buffer[col_buff_idx] = 0;  // TODO: is this line neccessary?
                }

                // (re)start row at block west border on each horizontal src scan
                float west = block_west;

                for (uint col = 0; col < 8; ++col) {

                    // src pixel east border X-coord expressed in dst pixels
                    const float east = west + m_horiz_scaling_factor;

                    // X-coord of dst pixel overlayed by west portion of src pixel
                    const int floor_west = static_cast<uint>(west);

                    // X-coord of dst pixel overlayed by east portion of src pixel
                    const int floor_east = static_cast<uint>(east);

                    // total src pixel value to be weight-distributed between up to 4 dst pixels that it potentially overlays
                    const float val = static_cast<float>(src_block[src_idx++]);

                    // if src and dst pixels are aligned along their east borders (A)
                    if (east == floor_east) {

                        // if src and dst pixels are aligned along their south borders (A1)
                        if (south == floor_south) {

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + (m_row_buffer[floor_west] + val) * m_val_norm_factor);
                            m_row_buffer[floor_west] = 0;
                        }

                        // if src pixel otherwise overlays a single dst row vertically (A2)
                        else if (north_fraction == 1) {

                            m_row_buffer[floor_west] += val;
                        }

                        // else src pixel (partially) overlays 2 dst rows vertically (A3)
                        else {

                            const float north_val = north_fraction * val;
                            const float south_val = val - north_val;

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + (m_row_buffer[floor_west] + north_val) * m_val_norm_factor);
                            m_row_buffer[floor_west] = m_edge_buffer + south_val;
                            m_edge_buffer = 0;
                        }
                    }

                    // if src pixel otherwise overlays a single dst pixel horizontally (B)
                    else if (floor_west == floor_east) {

                        // if src pixel overlays a single dst row vertically (B1 & B2)
                        if (north_fraction == 1) {

                            // if src and dst pixels are aligned along their south borders (B1) *and* src pixel is last one in its row
                            if (south == floor_south && col == 7) {

                                m_column_buffer[col_buff_idx++] = m_row_buffer[floor_west] + val;
                                m_row_buffer[floor_west] = 0;                                
                            }
                            else {

                                m_row_buffer[floor_west] += val;
                            }
                        }

                        // else src pixel (partially) overlays 2 dst rows vertically (B3)
                        else {

                            const float north_val = north_fraction * val;
                            const float south_val = val - north_val;

                            // if src pixel is last one in its row
                            if (col == 7) {

                                m_column_buffer[col_buff_idx++] = m_row_buffer[floor_west] + north_val;
                                m_row_buffer[floor_west] = m_edge_buffer + south_val;
                                m_edge_buffer = 0;
                            }
                            else {

                                m_row_buffer[floor_west] += north_val;
                                m_edge_buffer += south_val;
                            }
                        }
                    }

                    // else src pixel (partially) overlays 2 dst pixels horizontally (C)
                    else {

                        const float west_val =(static_cast<float>(floor_east) - west) * val / m_horiz_scaling_factor;
                        const float east_val = val - west_val;

                        // TODO: C1 and C2 share most of their logic, can be combined
                        
                        // if src and dst pixels are aligned along their south borders (C1)
                        if (south == floor_south) {

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + (m_row_buffer[floor_west] + west_val) * m_val_norm_factor);
                            m_row_buffer[floor_west] = 0;

                            // if src pixel is last one in its row
                            if (col == 7) {

                                m_column_buffer[col_buff_idx++] += east_val;
                            }

                            else {

                                m_row_buffer[floor_east] += east_val;
                            }
                        }

                        // if src pixel otherwise overlays a single dst row vertically (C2)
                        else if (north_fraction == 1) {

                            m_row_buffer[floor_west] += west_val;

                            // if src pixel is last one in its row
                            if (col == 7) {

                                m_column_buffer[col_buff_idx] += east_val;
                            }
                            else {

                                m_row_buffer[floor_east] += east_val;
                            }
                        }

                        // else src pixel (partially) overlays 2 dst rows vertically (C3)
                        else {

                            const float north_west_val = north_fraction * west_val;
                            const float south_west_val = west_val - north_west_val;
                            const float north_east_val = north_fraction * east_val;
                            const float south_east_val = east_val - north_east_val;

                            m_dst[vert_offset + floor_west] = static_cast<uint8_t>(0.5f + (m_row_buffer[floor_west] + north_west_val) * m_val_norm_factor);
                            m_row_buffer[floor_west] = m_edge_buffer + south_west_val;

                            // if src pixel is last one in its row
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

                    // advance dst pixel west border
                    west = east;
                }

                // advance dst pixel north border
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
};
