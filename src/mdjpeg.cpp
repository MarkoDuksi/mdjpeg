#include "mdjpeg.h"


/////////////////////////
// CompressedData public:

CompressedData::CompressedData(uint8_t* buff, size_t size) noexcept :
    m_buff_start(buff),
    m_buff_current(buff),
    m_buff_end(m_buff_start + size)
    {}

size_t CompressedData::size_remaining() const noexcept {
    return m_buff_end - m_buff_current;
}

bool CompressedData::seek(const size_t rel_pos) noexcept {
    if (rel_pos < size_remaining()) {
        m_buff_current += rel_pos;
        return true;
    }
    else {
        return false;
    }
}

std::optional<uint8_t> CompressedData::peek(const size_t rel_pos) const noexcept {
    if (rel_pos < size_remaining()) {
        return m_buff_current[rel_pos];
    }
    else {
        return {};
    }
}

std::optional<uint8_t> CompressedData::read_uint8() noexcept {
    if (size_remaining() > 0) {
        return *(m_buff_current++);
    }
    else {
        return {};
    }
}

std::optional<uint16_t> CompressedData::read_uint16() noexcept {
    if (size_remaining() >= 2) {
        uint16_t result = m_buff_current[0] << 8 | m_buff_current[1];
        m_buff_current += 2;
        return result;
    }
    else {
        return {};
    }
}

std::optional<uint16_t> CompressedData::read_marker() noexcept {
    auto result = read_uint16();

    while (result && *result == 0xffff) {
        if (auto next_byte = read_uint8(); next_byte) {
            *result = (*result & 0xff00) | *next_byte;
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

uint16_t CompressedData::set_qtable(uint16_t segment_size) noexcept {
    const uint next_byte = *m_buff_current;
    --segment_size;

    const uint8_t precision = 1 + static_cast<bool>(next_byte >> 4);
    const uint8_t table_id  = next_byte & 0xf;

    if (const uint8_t table_size = 64 * precision; table_size <= segment_size) {
        if (table_id == 0 && precision == 1) {
            for (size_t i = 0; i < 64; ++i) {
                m_luma_qtable_buff[m_zig_zag_map[i]] = m_buff_current[i];
            }
            m_luma_qtable = &m_luma_qtable_buff[0];
        }
        return 1 + table_size;
    }
    else {
        return 0;
    }
}

uint16_t CompressedData::set_htable(uint16_t segment_size) noexcept {
    const uint next_byte = *m_buff_current;
    --segment_size;

    const uint8_t is_dc = !static_cast<bool>(next_byte >> 4);
    const uint8_t table_id  = next_byte & 0xf;

    uint16_t symbols_count = 0;

    for (size_t i = 1; i < 17 && segment_size; ++i, --segment_size) {
        symbols_count += m_buff_current[i];
    }

    if (segment_size && symbols_count && symbols_count <= segment_size) {
        if (table_id == 0) {
            if (is_dc) {
                m_luma_dc_htable = m_buff_current + 1;
            }
            else {
                m_luma_ac_htable = m_buff_current + 1;
            }
        }
        return 1 + 16 + symbols_count;
    }
    else {
        return 0;
    }
}

////////////////
// JpegDecoder public:

JpegDecoder::JpegDecoder(uint8_t *buff, size_t size) noexcept :
    m_data(buff, size),
    m_state(ConcreteState<StateID::ENTRY>(this, m_data)),
    m_state_ptr(&m_state)
    {}

StateID JpegDecoder::parse_header() {
    while (!m_state_ptr->is_final_state()) {
        m_state_ptr->parse_header();
    }

    return m_state_ptr->getID();
}




////////////////
// State public:

State::State(JpegDecoder* decoder, CompressedData& data) noexcept :
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

    const auto next_marker = m_data.read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::SOI:
            std::cout << "Found marker: SOI (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOI);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::SOI>::parse_header() {
    std::cout << "Entered state SOI\n";

    const auto next_marker = m_data.read_marker();

    if (static_cast<StateID>(*next_marker) >= StateID::APP0 &&
        static_cast<StateID>(*next_marker) <= StateID::APP15) {
        std::cout << "Found marker: APPN (0x" << std::hex << *next_marker << ")\n";
        SET_NEXT_STATE(StateID::APP0);
    }
    else {
        std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
        SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::APP0>::parse_header() {
    std::cout << "Entered state APP0\n";

    const auto size = m_data.read_size();

    if (!size || !m_data.seek(*size)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    const auto next_marker = m_data.read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DQT);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::DQT>::parse_header() {
    std::cout << "Entered state DQT\n";

    auto segment_size = m_data.read_size();

    if (!segment_size || *segment_size > m_data.size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    while (*segment_size) {
        uint16_t&& qtable_size = m_data.set_qtable(*segment_size);

        if (!qtable_size) {
            SET_NEXT_STATE(StateID::ERROR_PEOB);
            return;
        }

        m_data.seek(qtable_size);
        *segment_size -= qtable_size;
    }

    const auto next_marker = m_data.read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:
            std::cout << "Found marker: DHT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DHT);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::DHT>::parse_header() {
    std::cout << "Entered state DHT\n";

    auto segment_size = m_data.read_size();

    if (!segment_size || *segment_size > m_data.size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    while (*segment_size) {
        uint16_t&& htable_size = m_data.set_htable(*segment_size);

        if (!htable_size) {
            SET_NEXT_STATE(StateID::ERROR_SEGO);
            return;
        }

        m_data.seek(htable_size);
        *segment_size -= htable_size;
    }

    const auto next_marker = m_data.read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:
            std::cout << "Found marker: DHT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::SOF0:
            std::cout << "Found marker: SOF0 (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOF0);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::SOF0>::parse_header() {
    std::cout << "Entered state SOF0\n";

    const auto size = m_data.read_size();

    if (!size || !m_data.seek(*size)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    const auto next_marker = m_data.read_marker();

    if (!next_marker) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:
            std::cout << "Found marker: DHT (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::SOS:
            std::cout << "Found marker: SOS (0x" << std::hex << *next_marker << ")\n";
            SET_NEXT_STATE(StateID::SOS);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::SOS>::parse_header() {
    std::cout << "Entered state SOS\n";

    const auto size = m_data.read_size();

    if (!size || !m_data.seek(*size)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
    }
    else {
        SET_NEXT_STATE(StateID::STREAM);
    }
}

template<>
void ConcreteState<StateID::STREAM>::parse_header() {
    // std::cout << "Entered state STREAM\n";

    const auto current_byte = m_data.peek();

    if (!current_byte) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    if (*current_byte == 0xff) {
        const auto next_marker = m_data.read_marker();

        if (!next_marker) {
            SET_NEXT_STATE(StateID::ERROR_PEOB);
            return;
        }
        else if (*next_marker > 0xff00) {
            switch (static_cast<StateID>(*next_marker)) {
                case StateID::DQT:
                    std::cout << "Found marker: DQT (0x" << std::hex << *next_marker << ")\n";
                    SET_NEXT_STATE(StateID::DQT);
                    break;

                case StateID::DHT:
                    std::cout << "Found marker: DHT (0x" << std::hex << *next_marker << ")\n";
                    SET_NEXT_STATE(StateID::DHT);
                    break;

                case StateID::SOS:
                    std::cout << "Found marker: SOS (0x" << std::hex << *next_marker << ")\n";
                    SET_NEXT_STATE(StateID::SOS);
                    break;

                default:
                    std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
                    SET_NEXT_STATE(StateID::ERROR_UUM);
            }
            return;
        }
    }
    else {
        m_data.seek(1);
        // process the now validated byte current_byte
        // std::cout << "Processing byte from stream: 0x" << std::hex << static_cast<uint>(*current_byte) << "\n";
    }
}

template<>
void ConcreteState<StateID::EOI>::parse_header() {
    std::cout << "Entered state EOI\n";

    SET_NEXT_STATE(StateID::EXIT_OK);
}
