#include "mdjpeg.h"

#include <fmt/core.h>


#define ECS_ERROR -1
#define SYMBOL_ERROR 0xff
#define COEF_ERROR 0x2000


/////////////////////////
// CompressedData public:

CompressedData::CompressedData(uint8_t* buff, size_t size) noexcept :
    m_buff_start(buff),
    m_buff_current_byte(buff),
    m_buff_end(m_buff_start + size)
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

uint CompressedData::set_qtable(size_t max_read_length) noexcept {
    const uint next_byte = *m_buff_current_byte++;

    const uint precision = 1 + bool(next_byte >> 4);  // in bytes, not bits
    const uint table_id  = next_byte & 0xf;
    const uint table_size = 64 * precision;

    if (1 + table_size > max_read_length) {
        return 0;
    }

    // store only 8-bit luma quantization table pointer
    if (table_id == 0) {
        if (precision == 1) {
            m_qtable = m_buff_current_byte;
        }
        else {
            return 0;
        }
    }

    m_buff_current_byte += table_size;

    return 1 + table_size;
}

uint CompressedData::set_htable(size_t max_read_length) noexcept {
    const uint next_byte = *m_buff_current_byte;
    --max_read_length;

    const bool is_dc = !(next_byte >> 4);
    const uint table_id = next_byte & 0xf;

    uint symbols_count = 0;

    for (uint i = 1; i < 17; ++i) {
        if (!max_read_length || m_buff_current_byte[i] > 1 << i) {
            return 0;
        }
        symbols_count += m_buff_current_byte[i];
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
        m_htables[table_id].dc.histogram = m_buff_current_byte + 1;
        m_htables[table_id].dc.symbols = m_buff_current_byte + 1 + 16;
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
        m_htables[table_id].ac.histogram = m_buff_current_byte + 1;
        m_htables[table_id].ac.symbols = m_buff_current_byte + 1 + 16;
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




////////////////
// JpegDecoder public:

JpegDecoder::JpegDecoder(uint8_t *buff, size_t size) noexcept :
    m_data(buff, size),
    m_state(ConcreteState<StateID::ENTRY>(this, &m_data)),
    m_state_ptr(&m_state)
    {}

StateID JpegDecoder::parse_header() {
    while (!m_state_ptr->is_final_state()) {
        m_state_ptr->parse_header();
    }

    return m_state_ptr->getID();
}

uint8_t JpegDecoder::get_dc_huff_symbol(uint table_id) noexcept {
    uint curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {
        const int next_bit = m_data.read_bit();
        if (next_bit == ECS_ERROR) {
            return SYMBOL_ERROR;
        }

        curr_code |= next_bit;
        for (uint j = 0; j < m_data.m_htables[table_id].dc.histogram[i] && idx < 12; ++j) {
            if (curr_code == m_data.m_htables[table_id].dc.codes[idx]) {
                return m_data.m_htables[table_id].dc.symbols[idx];
            }
            ++idx;
        }
        curr_code <<= 1;
    }

    return 0xff;
}

uint8_t JpegDecoder::get_ac_huff_symbol(uint table_id) noexcept {
    uint curr_code = 0;
    uint idx = 0;

    for (uint i = 0; i < 16; ++i) {
        const int next_bit = m_data.read_bit();
        if (next_bit == ECS_ERROR) {
            return SYMBOL_ERROR;
        }

        curr_code = curr_code << 1 | next_bit;
        for (uint j = 0; j < m_data.m_htables[table_id].ac.histogram[i] && idx < 162; ++j) {
            if (curr_code == m_data.m_htables[table_id].ac.codes[idx]) {
                return m_data.m_htables[table_id].ac.symbols[idx];
            }
            ++idx;
        }
    }

    return 0xff;
}

int16_t JpegDecoder::get_dct_coeff(uint length) noexcept {
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
    if (dct_coeff && dct_coeff >> (length - 1) == 0) {
        return dct_coeff - (1 << length) + 1;
    }

    return dct_coeff;
}

bool JpegDecoder::huff_decode_block(int* dst_block, int& previous_dc_coeff, uint table_id) noexcept {
    if (m_state_ptr->getID() != StateID::HEADER_OK) {
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
            dst_block[m_data.m_zig_zag_map[idx++]] = 0;
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

        dst_block[m_data.m_zig_zag_map[idx++]] = dct_coeff;
    }

    while (idx < 64) {
        dst_block[m_data.m_zig_zag_map[idx++]] = 0;
    }

    return true;
}

bool JpegDecoder::decode(int* dst, uint16_t x, uint16_t y, uint16_t width, uint16_t height) noexcept {
    if (m_state_ptr->getID() != StateID::HEADER_OK) {
        return false;
    }
    
    // decode the whole image area by default
    if (!width) {
        width = m_data.img_width;
    }
    if (!height) {
        height = m_data.img_height;
    }

    int block_8x8[64] {0};
    int previous_dc_coeffs[3] {0};

    const uint blocks_width = (width + 7) / 8;
    const uint blocks_height = (height + 7) / 8;
    const uint subblocks_width = 2 * blocks_width;
    const uint subblocks_height = blocks_height;
    const uint subblocks_count = subblocks_width * subblocks_height;

    std::cout << "width:  " << std::dec << width << "\n";
    std::cout << "height: " << std::dec << height << "\n";
    std::cout << "subblocks_count: " << std::dec << subblocks_count << "\n";
    std::cout << "m_data.m_buff_start:           " << std::hex << (size_t)m_data.m_buff_start << "\n";
    std::cout << "m_data.m_buff_start_of_ECS:    " << std::hex << (size_t)m_data.m_buff_start_of_ECS << "\n";
    std::cout << "m_data.m_buff_current_byte:    " << std::hex << (size_t)m_data.m_buff_current_byte << "\n";
    std::cout << "m_data.m_buff_end:             " << std::hex << (size_t)m_data.m_buff_end << "\n";
            
    uint mcu_count = 1;
    for (uint i = 0; i < subblocks_count; ++i) {
        if ((i & 3) <= 1) {
            std::cout << "\nY (MCU #" << std::dec << mcu_count << ")\n";
            if (!huff_decode_block(block_8x8, previous_dc_coeffs[0], 0)) {
                std::cout << "Huffman decoding FAILED!" << "\n";
                std::cout << "width:  " << std::dec << width << "\n";
                std::cout << "height: " << std::dec << height << "\n";
                std::cout << "subblocks_count: " << std::dec << subblocks_count << "\n";
                std::cout << "m_data.m_buff_start:           " << std::hex << (size_t)m_data.m_buff_start << "\n";
                std::cout << "m_data.m_buff_start_of_ECS:    " << std::hex << (size_t)m_data.m_buff_start_of_ECS << "\n";
                std::cout << "m_data.m_buff_current_byte:    " << std::hex << (size_t)m_data.m_buff_current_byte << "\n";
                std::cout << "m_data.m_buff_end:             " << std::hex << (size_t)m_data.m_buff_end << "\n";
                return false;
            }
        }
        else if ((i & 3) == 2) {
            std::cout << "\nCb (MCU #" << std::dec << mcu_count << ")\n";
            if (!huff_decode_block(block_8x8, previous_dc_coeffs[1], 1)) {
                std::cout << "Huffman decoding FAILED!" << "\n";
                std::cout << "width:  " << std::dec << width << "\n";
                std::cout << "height: " << std::dec << height << "\n";
                std::cout << "subblocks_count: " << std::dec << subblocks_count << "\n";
                std::cout << "m_data.m_buff_start:           " << std::hex << (size_t)m_data.m_buff_start << "\n";
                std::cout << "m_data.m_buff_start_of_ECS:    " << std::hex << (size_t)m_data.m_buff_start_of_ECS << "\n";
                std::cout << "m_data.m_buff_current_byte:    " << std::hex << (size_t)m_data.m_buff_current_byte << "\n";
                std::cout << "m_data.m_buff_end:             " << std::hex << (size_t)m_data.m_buff_end << "\n";
                return false;
            }
        }
        else {
            std::cout << "\nCr (MCU #" << std::dec << mcu_count++ << ")\n";
            if (!huff_decode_block(block_8x8, previous_dc_coeffs[2], 1)) {
                std::cout << "\nHuffman Decoding FAILED!\n";
                std::cout << "width:  " << std::dec << width << "\n";
                std::cout << "height: " << std::dec << height << "\n";
                std::cout << "subblocks_count: " << std::dec << subblocks_count << "\n";
                std::cout << "m_data.m_buff_start:           " << std::hex << (size_t)m_data.m_buff_start << "\n";
                std::cout << "m_data.m_buff_start_of_ECS:    " << std::hex << (size_t)m_data.m_buff_start_of_ECS << "\n";
                std::cout << "m_data.m_buff_current_byte:    " << std::hex << (size_t)m_data.m_buff_current_byte << "\n";
                std::cout << "m_data.m_buff_end:             " << std::hex << (size_t)m_data.m_buff_end << "\n";
                return false;
            }
        }

        for (uint i = 0; i < 8; ++i) {
            for (uint j = 0; j < 8; ++j) {
                std::cout << std::dec << block_8x8[8 * i + j] << " ";
            }
            std::cout << "\n";
        }

    }

    std::cout << "\nDecoding SUCCESSFUL.\n\n";

    for (uint i = 0; i < 64; ++i) {
        dst[i] = block_8x8[i];
    }

    return true;
}




////////////////
// State public:

State::State(JpegDecoder* decoder, CompressedData* data) noexcept :
    m_decoder(decoder),
    m_data(data)
    {}

State::~State() {}

bool State::is_final_state() const noexcept {
    return getID() < StateID::ENTRY;
}




////////////////////////
// ConcreteState public:

#define SET_NEXT_STATE(state_id) m_decoder->m_state_ptr = new (m_decoder->m_state_ptr) ConcreteState<state_id>(m_decoder, m_data)

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
        const uint qtable_size = m_data->set_qtable(*segment_size);

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
        const uint htable_size = m_data->set_htable(*segment_size);

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

    m_data->img_height = *height;
    m_data->img_width = *width;

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
