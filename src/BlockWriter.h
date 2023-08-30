#pragma once

#include <stdint.h>
#include <sys/types.h>


class BlockWriter {
    public:
        virtual ~BlockWriter() = default;

        virtual void init_frame(uint8_t* const dst, uint src_width_px, [[maybe_unused]] uint src_height_px) = 0;
        virtual void write(int (&src_block)[64]) = 0;
};


class DirectBlockWriter : public BlockWriter {
    public:
        void init_frame(uint8_t* const dst, uint src_width_px, [[maybe_unused]] uint src_height_px) final {
            m_dst = dst;
            m_width_px = src_width_px;
            m_block_x = 0;
            m_block_y = 0;
        }
        
        void write(int (&src_block)[64]) final {
            uint offset = m_block_y * m_width_px + m_block_x;
            uint idx = 0;
            for (uint row = 0; row < 8; ++row) {
                for (uint col = 0; col < 8; ++col) {
                    m_dst[offset + col] = src_block[idx++];
                }
                offset += m_width_px;
            }

            m_block_x += 8;
            if (m_block_x == m_width_px) {
                m_block_x = 0;
                m_block_y += 8;
            }
        }

    private:
        uint8_t* m_dst {nullptr};
        uint m_width_px {};
        uint m_block_x {};
        uint m_block_y {};
};

// template <uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
// class ScalingBlockWriter : public BlockWriter {
//     public:
//         void write(uint8_t* dst, uint dst_x_blk, uint dst_y_blk, int (&src_block)[64], uint src_width_px, uint src_height_px) final {
//             for (uint y = 0; y < 8; ++y) {
//                 for (uint x = 0; x < 8; ++x) {
//                     dst[(8 * dst_y_blk + y) * DST_WIDTH_PX + 8 * dst_x_blk + x] = src_block[8 * y + x];
//                 }
//             }
//         }

//         void init(uint src_width_px, uint src_height_px) final {
//             m_dst = dst;
//             m_block_idx = 0;
//             m_src_width_px = src_width_px;
//             m_src_height_px = src_height_px;
//             m_horiz_scaling_factor = static_cast<float>(DST_WIDTH_PX) / src_width_px;
//             m_vert_scaling_factor = static_cast<float>(DST_HEIGHT_PX) / src_height_px;

//             for (uint i = 0; i < DST_WIDTH_PX * DST_HEIGHT_PX; ++i) {
//                 m_buffer[i] = 0;
//             }

//         }

//     private:
//         float m_horiz_scaling_factor {};
//         float m_vert_scaling_factor {};
//         float m_buffer[DST_WIDTH_PX + 8] {};
// };
