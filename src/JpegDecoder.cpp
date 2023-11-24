#include "JpegDecoder.h"

#include "BlockWriter.h"
#include "transform.h"


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

bool JpegDecoder::luma_decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk) noexcept {

    BasicBlockWriter writer;

    return luma_decode(dst, x1_blk, y1_blk, x2_blk, y2_blk, writer);
}

bool JpegDecoder::luma_decode(uint8_t* const dst, const uint x1_blk, const uint y1_blk, const uint x2_blk, const uint y2_blk, BlockWriter& writer) noexcept {

    if (!m_has_valid_header) {

        return false;
    }

    if (!validate_bounds(x1_blk, y1_blk, x2_blk, y2_blk)) {

        return false;
    }

    int block_8x8[64] {0};
    const uint src_width_px = 8 * (x2_blk - x1_blk);
    const uint src_height_px = 8 * (y2_blk - y1_blk);

    writer.init(dst, src_width_px, src_height_px);

    for (uint row_blk = y1_blk; row_blk < y2_blk; ++row_blk) {

        uint luma_block_idx = row_blk * m_frame_info.width_blk + x1_blk;

        for (uint col_blk = x1_blk; col_blk < x2_blk; ++col_blk, ++luma_block_idx) {

            if (!m_huffman.decode_luma_block(m_reader, block_8x8, luma_block_idx, m_frame_info.horiz_chroma_subs_factor)) {

                return false;
            }

            m_dequantizer.transform(block_8x8);
            mdjpeg_transform::reverse_zig_zag(block_8x8);
            mdjpeg_transform::idct(block_8x8);
            mdjpeg_transform::range_normalize(block_8x8);
            writer.write(block_8x8);
        }
    }

    return true;
}

bool JpegDecoder::dc_luma_decode(uint8_t* const dst, uint x1_blk, uint y1_blk, uint x2_blk, uint y2_blk) noexcept {

    if (!m_has_valid_header) {

        return false;
    }
    
    if (!validate_bounds(x1_blk, y1_blk, x2_blk, y2_blk)) {

        return false;
    }

    const uint dst_width_px = x2_blk - x1_blk;
    int block_8x8[64] {0};

    for (uint row_blk = y1_blk; row_blk < y2_blk; ++row_blk) {

        uint luma_block_idx = row_blk * m_frame_info.width_blk + x1_blk;

        for (uint col_blk = x1_blk; col_blk < x2_blk; ++col_blk, ++luma_block_idx) {

            if (!m_huffman.decode_luma_block(m_reader, block_8x8, luma_block_idx, m_frame_info.horiz_chroma_subs_factor)) {

                return false;
            }

            // dequantize only the DC DCT coefficient
            m_dequantizer.transform(block_8x8[0]);

            // recover block-averaged luma value
            const int dc_luma = (block_8x8[0] + 1024) / 8;

            dst[(row_blk - y1_blk) * dst_width_px + (col_blk - x1_blk)] = dc_luma;
        }
    }

    return true;
}

StateID JpegDecoder::parse_header() noexcept {

    while (!m_istate->is_final_state()) {

        m_istate->parse_header(m_reader);
    }

    return m_istate->getID();
}

bool JpegDecoder::validate_bounds(const uint x1_blk, const uint y1_blk, const uint x2_blk, const uint y2_blk) const noexcept {

    return (x1_blk < x2_blk && y1_blk < y2_blk
                            && x2_blk <= m_frame_info.width_blk
                            && y2_blk <= m_frame_info.height_blk);
}

void JpegDecoder::FrameInfo::set(const uint16_t height_px, const uint16_t width_px, const uint8_t horiz_chroma_subs_factor) noexcept {

    this->height_px = height_px;
    this->width_px = width_px;
    height_blk = (height_px + 7) / 8;
    width_blk = (width_px + 7) / 8;
    this->horiz_chroma_subs_factor = horiz_chroma_subs_factor;
}
