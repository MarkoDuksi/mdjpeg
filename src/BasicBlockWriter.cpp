#include "BasicBlockWriter.h"

#include <stdint.h>
#include <sys/types.h>


void BasicBlockWriter::init(uint8_t* const dst, const uint16_t src_width_px, [[maybe_unused]] const uint16_t src_height_px) noexcept {

    m_dst = dst;
    m_src_width_px = src_width_px;
    m_block_x = 0;
    m_block_y = 0;
}
        
void BasicBlockWriter::write(int (&src_block)[64]) noexcept {

    uint32_t offset = m_block_y * m_src_width_px + m_block_x;
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
