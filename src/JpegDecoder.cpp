#include "JpegDecoder.h"

#include <algorithm>

#include "BasicBlockWriter.h"
#include "transform.h"


using namespace mdjpeg;

bool JpegDecoder::assign(const uint8_t* const buff, const size_t size) noexcept {

    m_reader.set(buff, size);

    m_dequantizer.clear();
    m_huffman.clear();
    m_frame_info.clear();

    set_state<StateID::ENTRY>();
    m_has_valid_header = parse_header() == StateID::HEADER_OK;

    if (!m_has_valid_header) {

        m_frame_info.clear();
    }

    return m_has_valid_header;
}

bool JpegDecoder::luma_decode(uint8_t* const dst, const BoundingBox& roi_blk) noexcept {

    BasicBlockWriter writer;

    return luma_decode(dst, roi_blk, writer);
}

bool JpegDecoder::luma_decode(uint8_t* const dst, const BoundingBox& roi_blk, BlockWriter& writer) noexcept {

    if (!m_has_valid_header) {

        return false;
    }

    writer.init(dst, 8 * roi_blk.width(), 8 * roi_blk.height());

    int block_8x8[64] {0};

    const uint16_t src_width_blk = static_cast<uint16_t>(m_frame_info.width_px + 7) / 8;
    uint32_t row_blk_idx = roi_blk.topleft_Y * src_width_blk + roi_blk.topleft_X;

    for (uint16_t row = roi_blk.topleft_Y; row < roi_blk.bottomright_Y; ++row) {

        uint32_t luma_block_idx = row_blk_idx;

        for (uint16_t col = roi_blk.topleft_X; col < roi_blk.bottomright_X; ++col, ++luma_block_idx) {

            if (!m_huffman.decode_luma_block(m_reader, block_8x8, luma_block_idx, m_frame_info.horiz_chroma_subs_factor)) {

                return false;
            }

            m_dequantizer.transform(block_8x8);
            transform::reverse_zig_zag(block_8x8);
            transform::idct(block_8x8);
            transform::range_normalize(block_8x8);
            writer.write(block_8x8);
        }

        row_blk_idx += src_width_blk;
    }

    return true;
}

bool JpegDecoder::dc_luma_decode(uint8_t* const dst, const BoundingBox& roi_blk) noexcept {

    if (!m_has_valid_header) {

        return false;
    }
   
    int block_8x8[64] {0};

    const uint16_t src_width_blk = static_cast<uint16_t>(m_frame_info.width_px + 7) / 8;
    uint32_t row_blk_idx = roi_blk.topleft_Y * src_width_blk + roi_blk.topleft_X;

    for (uint16_t row = roi_blk.topleft_Y; row < roi_blk.bottomright_Y; ++row) {

        uint32_t luma_block_idx = row_blk_idx;

        for (uint16_t col = roi_blk.topleft_X; col < roi_blk.bottomright_X; ++col, ++luma_block_idx) {
            if (!m_huffman.decode_luma_block(m_reader, block_8x8, luma_block_idx, m_frame_info.horiz_chroma_subs_factor)) {

                return false;
            }

            // dequantize only the DC DCT coefficient
            m_dequantizer.transform(block_8x8[0]);

            // recover block-averaged luma value
            const uint8_t dc_luma = std::min(255u, static_cast<uint>(std::max(0, block_8x8[0] + 1024)) / 8);

            dst[(row - roi_blk.topleft_Y) * roi_blk.width() + (col - roi_blk.topleft_X)] = dc_luma;
        }

        row_blk_idx += src_width_blk;
    }

    return true;
}

StateID JpegDecoder::parse_header() noexcept {

    while (!m_istate->is_final_state()) {

        m_istate->parse_header(m_reader);
    }

    return m_istate->getID();
}

void JpegDecoder::FrameInfo::set(const uint16_t height_px, const uint16_t width_px, const uint8_t horiz_chroma_subs_factor) noexcept {

    this->height_px = height_px;
    this->width_px = width_px;
    this->horiz_chroma_subs_factor = horiz_chroma_subs_factor;
}
