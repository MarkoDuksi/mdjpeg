#include "mdjpeg.h"

#include <cmath>

#include <fmt/core.h>


#define ECS_ERROR -1
#define SYMBOL_ERROR 0xff
#define COEF_ERROR 0x2000


/////////////////////////
// CompressedData public:

CompressedData::CompressedData(const uint8_t* const buff, const size_t size) noexcept :
    m_buff_start(buff),
    m_buff_end(m_buff_start + size),
    m_buff_current_byte(const_cast<const uint8_t*>(buff))
    {}

size_t CompressedData::size_remaining() const noexcept {
    return m_buff_end - m_buff_current_byte;
}

bool CompressedData::seek(const size_t rel_pos) noexcept {
    if (rel_pos < size_remaining()) {
        m_buff_current_byte += rel_pos;
        return true;
    }
    else {
        return false;
    }
}

std::optional<uint8_t> CompressedData::peek(const size_t rel_pos) const noexcept {
    if (rel_pos < size_remaining()) {
        return m_buff_current_byte[rel_pos];
    }
    else {
        return {};
    }
}

std::optional<uint8_t> CompressedData::read_uint8() noexcept {
    if (size_remaining() > 0) {
        return *(m_buff_current_byte++);
    }
    else {
        return {};
    }
}

std::optional<uint16_t> CompressedData::read_uint16() noexcept {
    if (size_remaining() >= 2) {
        const uint result = m_buff_current_byte[0] << 8 | m_buff_current_byte[1];
        m_buff_current_byte += 2;
        return result;
    }
    else {
        return {};
    }
}

std::optional<uint16_t> CompressedData::read_marker() noexcept {
    auto result = read_uint16();

    while (result && *result == 0xffff) {
        const auto next_byte = read_uint8();
        if (next_byte) {
            *result = *result & *next_byte;
        }
        else {
            result = std::nullopt;
        }
    }

    return result;
}

std::optional<uint16_t> CompressedData::read_size() noexcept {
    auto result = read_uint16();

    if (!result || *result < 2) {
        result = std::nullopt;
        }

    return *result - 2;
}

// todo: consider making read_bit() branchless
int CompressedData::read_bit() noexcept {
    // current byte is always valid, read current bit if not last (lowest, at position 0)
    if (m_current_bit_pos) {
        return *m_buff_current_byte >> m_current_bit_pos-- & 1;
    }

    // validate next byte before advancing current byte (next most common scenario)
    if (*m_buff_current_byte != 0xff && m_buff_current_byte[1] != 0xff) {
        m_current_bit_pos = 7;

        return *m_buff_current_byte++ & 1;
    }

    // allow reading next 0xff if byte-stuffed with 0x00
    if (m_buff_current_byte[1] == 0xff && m_buff_current_byte[2] == 0x00) {
        m_current_bit_pos = 7;

        return *m_buff_current_byte++ & 1;
    }

    // skip 0x00 after reading 0xff
    if (*m_buff_current_byte == 0xff) {
        m_current_bit_pos = 7;
        const int temp = *m_buff_current_byte & 1;
        m_buff_current_byte += 2;

        return temp;
    }


    // if reading last bit before expected EOI marker
    if (size_remaining() == 3) {
        return *m_buff_current_byte++ & 1;
    }

    // if reading last bit before invalid marker
    const auto marker = read_marker();
    if (marker) {
        std::cout << "unexpected marker in ECS: " << std::hex << *marker << "\n";
    }
    else {
        std::cout << "error reading unexpected marker in ECS\n";
    }

    return ECS_ERROR;
}




////////////////
// JpegDecoder public:

JpegDecoder::JpegDecoder(const uint8_t* const buff, const size_t size) noexcept :
    m_data(buff, size),
    m_state(ConcreteState<StateID::ENTRY>(this, &m_data)),
    m_istate(&m_state)
    {}

StateID JpegDecoder::parse_header() {
    while (!m_istate->is_final_state()) {
        m_istate->parse_header();
    }

    return m_istate->getID();
}

uint8_t JpegDecoder::get_dc_huff_symbol(const uint table_id) noexcept {
    uint curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {
        const int next_bit = m_data.read_bit();
        if (next_bit == ECS_ERROR) {
            return SYMBOL_ERROR;
        }

        curr_code |= next_bit;
        for (uint j = 0; j < m_htables[table_id].dc.histogram[i] && idx < 12; ++j) {
            if (curr_code == m_htables[table_id].dc.codes[idx]) {
                return m_htables[table_id].dc.symbols[idx];
            }
            ++idx;
        }
        curr_code <<= 1;
    }

    return 0xff;
}

uint8_t JpegDecoder::get_ac_huff_symbol(const uint table_id) noexcept {
    uint curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {
        const int next_bit = m_data.read_bit();
        if (next_bit == ECS_ERROR) {
            return SYMBOL_ERROR;
        }

        curr_code = curr_code << 1 | next_bit;
        for (uint j = 0; j < m_htables[table_id].ac.histogram[i] && idx < 162; ++j) {
            if (curr_code == m_htables[table_id].ac.codes[idx]) {
                return m_htables[table_id].ac.symbols[idx];
            }
            ++idx;
        }
    }

    return 0xff;
}

int16_t JpegDecoder::get_dct_coeff(const uint length) noexcept {
    if (length > 16) {
        return COEF_ERROR;
    }

    int dct_coeff = 0;

    for (uint i = 0; i < length; ++i) {
        const int next_bit = m_data.read_bit();
        if (next_bit == ECS_ERROR) {
            return COEF_ERROR;
        }
        dct_coeff = dct_coeff << 1 | next_bit;
    }

    // recover negative values
    if (length && dct_coeff >> (length - 1) == 0) {
        return dct_coeff - (1 << length) + 1;
    }

    return dct_coeff;
}

uint JpegDecoder::set_qtable(const size_t max_read_length) noexcept {
    const uint next_byte = *m_data.m_buff_current_byte++;

    const uint precision = 1 + bool(next_byte >> 4);  // in bytes, not bits
    const uint table_id  = next_byte & 0xf;
    const uint table_size = 64 * precision;

    if (1 + table_size > max_read_length) {
        return 0;
    }

    // store only 8-bit luma quantization table pointer
    if (table_id == 0) {
        if (precision == 1) {
            m_qtable = m_data.m_buff_current_byte;
        }
        else {
            return 0;
        }
    }

    m_data.m_buff_current_byte += table_size;

    return 1 + table_size;
}

uint JpegDecoder::set_htable(size_t max_read_length) noexcept {
    const uint next_byte = *m_data.m_buff_current_byte;
    --max_read_length;

    const bool is_dc = !(next_byte >> 4);
    const uint table_id = next_byte & 0xf;

    uint symbols_count = 0;

    for (uint i = 1; i < 17; ++i) {
        if (!max_read_length || m_data.m_buff_current_byte[i] > 1 << i) {
            return 0;
        }
        symbols_count += m_data.m_buff_current_byte[i];
        --max_read_length;
    }

    if (table_id > 1 || max_read_length == 0
                     || symbols_count == 0
                     || (is_dc && symbols_count > 12)
                     || symbols_count > 162
                     || symbols_count > max_read_length) {
        return 0;
    }

    if (is_dc) {
        m_htables[table_id].dc.histogram = m_data.m_buff_current_byte + 1;
        m_htables[table_id].dc.symbols = m_data.m_buff_current_byte + 1 + 16;
        std::cout << "\nHuffman table id " << (int)table_id << " (DC):\n";
        std::cout << "(length: code -> symbol)\n";

        // generate Huffman codes for DC coeff length symbols
        uint curr_huff_code = 0;
        uint idx = 0;
        for (uint i = 0; i < 16; ++i) {
            curr_huff_code <<= 1;
            for (uint j = 0; j < m_htables[table_id].dc.histogram[i]; ++j) {
                fmt::print("  {: >2}: {:0>{}b} -> 0x{:0>2x}\n",
                            i + 1,
                            curr_huff_code, i + 1,
                            m_htables[table_id].dc.symbols[idx]);
                m_htables[table_id].dc.codes[idx++] = curr_huff_code++;
            }
        }
        m_htables[table_id].dc.is_set = true;
    }
    else {
        m_htables[table_id].ac.histogram = m_data.m_buff_current_byte + 1;
        m_htables[table_id].ac.symbols = m_data.m_buff_current_byte + 1 + 16;
        std::cout << "\nHuffman table id " << (int)table_id << " (AC):\n";
        std::cout << "(length: code -> symbol)\n";

        // generate Huffman codes for AC coeff length symbols
        uint curr_huff_code = 0;
        uint idx = 0;
        for (uint i = 0; i < 16; ++i) {
            curr_huff_code <<= 1;
            for (uint j = 0; j < m_htables[table_id].ac.histogram[i]; ++j) {
                fmt::print("  {: >2}: {:0>{}b} -> 0x{:0>2x}\n",
                            i + 1,
                            curr_huff_code, i + 1,
                            m_htables[table_id].ac.symbols[idx]);
                m_htables[table_id].ac.codes[idx++] = curr_huff_code++;
            }
        }
        m_htables[table_id].ac.is_set = true;
    }

    return 1 + 16 + symbols_count;
}

bool JpegDecoder::huff_decode(int* const dst_block, int& previous_dc_coeff, const uint table_id) noexcept {
    if (m_istate->getID() != StateID::HEADER_OK) {
        return false;
    }

    // DC DCT coefficient
    const uint huff_symbol = get_dc_huff_symbol(table_id);
    if (huff_symbol > 11) {  // DC coefficient length out of range
        return false;
    }

    const int dct_coeff = get_dct_coeff(huff_symbol);
    if (dct_coeff == COEF_ERROR) {
        return false;
    }

    dst_block[0] = previous_dc_coeff + dct_coeff;

    previous_dc_coeff = dst_block[0];

    // AC DCT coefficient
    uint idx = 1;
    while (idx < 64) {
        uint pre_zeros_count;
        const uint huff_symbol = get_ac_huff_symbol(table_id);

        if (huff_symbol == COEF_ERROR) {
            return false;
        }
        else if (huff_symbol == 0x00) {  // the rest of coefficients are 0
            break;
        }
        else if (huff_symbol == 0xf0) {
            pre_zeros_count = 16;
        }
        else {
            pre_zeros_count = huff_symbol >> 4;
        }

        if (idx + pre_zeros_count >= 64) {  // prevent dst_block[64] overflow
            return false;
        }

        while (pre_zeros_count--) {
            dst_block[m_zig_zag_map[idx++]] = 0;
        }

        if (huff_symbol == 0xf0) {  // done with this symbol
            continue;
        }

        const uint dct_coeff_length = huff_symbol & 0xf;
        if (dct_coeff_length > 10) {  // AC coefficient length out of range
            return false;
        }

        const int dct_coeff = get_dct_coeff(dct_coeff_length);
        if (dct_coeff == COEF_ERROR) {
            return false;
        }

        dst_block[m_zig_zag_map[idx++]] = dct_coeff;
    }

    while (idx < 64) {
        dst_block[m_zig_zag_map[idx++]] = 0;
    }

    return true;
}


void JpegDecoder::dequantize(int* const block) noexcept {
    for (uint i = 0; i < 64; ++i) {
        block[m_zig_zag_map[i]] *= m_qtable[i];
    }
}


// idct code from:
// - https://github.com/dannye/jed/blob/master/src/jpg.h
void JpegDecoder::idct(int* const block) noexcept {
    for (uint i = 0; i < 8; ++i) {
        const float g0 = block[0 * 8 + i] * s0;
        const float g1 = block[4 * 8 + i] * s4;
        const float g2 = block[2 * 8 + i] * s2;
        const float g3 = block[6 * 8 + i] * s6;
        const float g4 = block[5 * 8 + i] * s5;
        const float g5 = block[1 * 8 + i] * s1;
        const float g6 = block[7 * 8 + i] * s7;
        const float g7 = block[3 * 8 + i] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        block[0 * 8 + i] = b0 + b7;
        block[1 * 8 + i] = b1 + b6;
        block[2 * 8 + i] = b2 + b5;
        block[3 * 8 + i] = b3 + b4;
        block[4 * 8 + i] = b3 - b4;
        block[5 * 8 + i] = b2 - b5;
        block[6 * 8 + i] = b1 - b6;
        block[7 * 8 + i] = b0 - b7;
    }

    for (uint i = 0; i < 8; ++i) {
        const float g0 = block[i * 8 + 0] * s0;
        const float g1 = block[i * 8 + 4] * s4;
        const float g2 = block[i * 8 + 2] * s2;
        const float g3 = block[i * 8 + 6] * s6;
        const float g4 = block[i * 8 + 5] * s5;
        const float g5 = block[i * 8 + 1] * s1;
        const float g6 = block[i * 8 + 7] * s7;
        const float g7 = block[i * 8 + 3] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        block[i * 8 + 0] = b0 + b7;
        block[i * 8 + 1] = b1 + b6;
        block[i * 8 + 2] = b2 + b5;
        block[i * 8 + 3] = b3 + b4;
        block[i * 8 + 4] = b3 - b4;
        block[i * 8 + 5] = b2 - b5;
        block[i * 8 + 6] = b1 - b6;
        block[i * 8 + 7] = b0 - b7;
    }
}


void JpegDecoder::level_to_unsigned(int* const block) noexcept {
    for (uint i = 0; i < 64; ++i) {
        block[i] += 128;
    }
}


bool JpegDecoder::decode(uint8_t* const dst, uint16_t x, uint16_t y, uint16_t width, uint16_t height) noexcept {
    if (parse_header() == StateID::HEADER_OK) {
        std::cout << "\nFinished in state HEADER_OK\n";
    }
    else {
        return false;
    }
    
    // decode the whole image area by default
    if (!width) {
        width = m_data.m_img_width;
    }
    if (!height) {
        height = m_data.m_img_height;
    }

    int block_8x8[64] {0};
    int previous_dc_coeffs[3] {0};

    const uint blocks_width = (width + 7) / 8;
    const uint blocks_height = (height + 7) / 8;
    const uint subblocks_width = blocks_width * (3 - m_data.m_horiz_subsampling);
    const uint subblocks_height = blocks_height;
    const uint subblocks_count = subblocks_width * subblocks_height;

    std::cout << "\n";
    std::cout << "width:  " << std::dec << width << "\n";
    std::cout << "height: " << std::dec << height << "\n";
    std::cout << "subblocks_count: " << std::dec << subblocks_count << "\n";
    std::cout << "m_data.m_buff_start:           " << std::hex << (size_t)m_data.m_buff_start << "\n";
    std::cout << "m_data.m_buff_start_of_ECS:    " << std::hex << (size_t)m_data.m_buff_start_of_ECS << "\n";
    std::cout << "m_data.m_buff_end:             " << std::hex << (size_t)m_data.m_buff_end << "\n";
            
    uint luma_count = 0;
    for (uint i = 0; i < subblocks_count; ++i) {
        if ((i % (3 + m_data.m_horiz_subsampling)) <= m_data.m_horiz_subsampling) {  // LUMA component
            if (!huff_decode(block_8x8, previous_dc_coeffs[0], 0)) {
                std::cout << "\nHuffman decoding FAILED!" << "\n";
                return false;
            }

            dequantize(block_8x8);
            idct(block_8x8);
            level_to_unsigned(block_8x8);

            for (uint row = 0; row < blocks_height; ++row) {
                for (uint col = 0; col < blocks_width; ++col) {
                    for (uint y = 0; y < 8; ++y) {
                        for (uint x = 0; x < 8; ++x) {
                            dst[(8 * row + y) * m_data.m_img_width + 8 * col + x] = block_8x8[8 * y + x];
                        }
                    }
                }
            }

            ++luma_count;
        }
        else if ((i % (3 + m_data.m_horiz_subsampling)) == 1u + m_data.m_horiz_subsampling) {  // Cr component
            if (!huff_decode(block_8x8, previous_dc_coeffs[1], 1)) {
                std::cout << "\nHuffman decoding FAILED!" << "\n";
                return false;
            }
        }
        else {  // Cb component
            if (!huff_decode(block_8x8, previous_dc_coeffs[2], 1)) {
                std::cout << "\nHuffman Decoding FAILED!\n";
                return false;
            }
        }
    }

    std::cout << "\nDecoding SUCCESSFUL.\n\n";

    return true;
}




////////////////
// State public:

State::State(JpegDecoder* const decoder, CompressedData* const data) noexcept :
    m_decoder(decoder),
    m_data(data)
    {}

State::~State() {}

bool State::is_final_state() const noexcept {
    return getID() < StateID::ENTRY;
}




////////////////////////
// ConcreteState public:

#define SET_NEXT_STATE(state_id) m_decoder->m_istate = new (m_decoder->m_istate) ConcreteState<state_id>(m_decoder, m_data)

template<>
void ConcreteState<StateID::ENTRY>::parse_header() {
    std::cout << "Entered state ENTRY\n";

    const auto next_marker = m_data->read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::SOI:
            std::cout << "\nFound marker: SOI (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOI);
            break;

        default:
            std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::SOI>::parse_header() {
    std::cout << "Entered state SOI\n";

    const auto next_marker = m_data->read_marker();

    if (static_cast<StateID>(*next_marker) >= StateID::APP0 && static_cast<StateID>(*next_marker) <= StateID::APP15) {
        std::cout << "\nFound marker: APPN (0x" << std::hex << *next_marker << ")\n";
        SET_NEXT_STATE(StateID::APP0);
    }
    else {
        std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
        SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::APP0>::parse_header() {
    std::cout << "Entered state APP0\n";

    const auto segment_size = m_data->read_size();

    // seek to the end of segment
    if (!segment_size || !m_data->seek(*segment_size)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    const auto next_marker = m_data->read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "\nFound marker: DQT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:
            std::cout << "\nFound marker: DHT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::SOF0:
            std::cout << "\nFound marker: SOF0 (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOF0);
            break;

        default:
            std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::DQT>::parse_header() {
    std::cout << "Entered state DQT\n";

    auto segment_size = m_data->read_size();

    if (!segment_size || *segment_size > m_data->size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    while (*segment_size) {
        const uint qtable_size = m_decoder->set_qtable(*segment_size);

        // any invalid qtable_size gets returned as 0
        if (!qtable_size) {
            SET_NEXT_STATE(StateID::ERROR_CORR);
            return;
        }

        // any valid qtable_size is *always* lte *segment_size
        // (no integer overflow possible)
        *segment_size -= qtable_size;
    }

    const auto next_marker = m_data->read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "\nFound marker: DQT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:
            std::cout << "\nFound marker: DHT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::SOF0:
            std::cout << "\nFound marker: SOF0 (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOF0);
            break;

        case StateID::SOS:
            std::cout << "\nFound marker: SOS (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOS);
            break;

        default:
            std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::DHT>::parse_header() {
    std::cout << "Entered state DHT\n";

    auto segment_size = m_data->read_size();

    if (!segment_size || *segment_size > m_data->size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    while (*segment_size) {
        const uint htable_size = m_decoder->set_htable(*segment_size);

        // any invalid htable_size gets returned as 0
        if (!htable_size) {
            SET_NEXT_STATE(StateID::ERROR_CORR);
            return;
        }

        // any valid htable_size is *always* lte *segment_size
        // no reading beyond buff + size - 1 (as provided to JpegDecoder constructor)
        m_data->seek(htable_size);
        // no integer overflow possible
        *segment_size -= htable_size;
    }

    const auto next_marker = m_data->read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DHT:
            std::cout << "\nFound marker: DHT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::DQT:
            std::cout << "\nFound marker: DQT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::SOF0:
            std::cout << "\nFound marker: SOF0 (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOF0);
            break;

        case StateID::SOS:
            std::cout << "\nFound marker: SOS (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOS);
            break;

        default:
            std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::SOF0>::parse_header() {
    std::cout << "Entered state SOF0\n";

    const auto segment_size = m_data->read_size();

    if (!segment_size || *segment_size > m_data->size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    // only the 3-component YUV color mode is supported
    if (*segment_size != 15) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    const auto precision = m_data->read_uint8();
    const auto height = m_data->read_uint16();
    const auto width = m_data->read_uint16();
    auto components_count = m_data->read_uint8();

    if (*precision != 8 || !*height
                        || !*width
                        || *components_count != 3) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    m_data->m_img_height = *height;
    m_data->m_img_width = *width;

    while ((*components_count)--) {
        const auto component_id = m_data->read_uint8();
        const auto sampling_factor = m_data->read_uint8();
        const auto qtable_id = m_data->read_uint8();

        // supported component IDs: 1, 2 and 3 (Y, U and V channel, respectively)
        if (*component_id == 0 || *component_id > 3 ) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
        if (*component_id == 1) {
            // component 1 must use qtable 0
            if (*qtable_id != 0) {
                SET_NEXT_STATE(StateID::ERROR_UPAR);
                return;
            }
            // component 1 must use sampling factors 0x11 or 0x21
            if (*sampling_factor != 0x11 && *sampling_factor != 0x21 ) {
                SET_NEXT_STATE(StateID::ERROR_UPAR);
                return;
            }
            else {
                m_data->m_horiz_subsampling = bool((*sampling_factor >> 4) - 1);
            }
        }
        // components 2 and 3 must use sampling factors 0x11
        if (*component_id > 1 && *sampling_factor != 0x11) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
    }

    const auto next_marker = m_data->read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "\nFound marker: DQT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:
            std::cout << "\nFound marker: DHT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::SOS:
            std::cout << "\nFound marker: SOS (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOS);
            break;

        default:
            std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

// todo: check that htables and qtables are set
template<>
void ConcreteState<StateID::SOS>::parse_header() {
    std::cout << "Entered state SOS\n";

    const auto segment_size = m_data->read_size();

    if (!segment_size || *segment_size > m_data->size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    // only the 3-component YUV color mode is supported
    if (*segment_size != 10) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    auto components_count = m_data->read_uint8();

    if (*components_count != 3) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    while ((*components_count)--) {
        const auto component_id = m_data->read_uint8();
        const auto dc_ac_table_ids = m_data->read_uint8();

        // component 1 must use htables 0 (DC) and 0 (AC)
        if (*component_id == 1 && *dc_ac_table_ids != 0 ) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
    }

    // only baseline JPEG is supported, skip the rest of SOS segment
    m_data->seek(3);  // todo: check instead of blindly skipping

    // mark the begining of ECS
    m_data->m_buff_start_of_ECS = m_data->m_buff_current_byte;

    SET_NEXT_STATE(StateID::HEADER_OK);
}
