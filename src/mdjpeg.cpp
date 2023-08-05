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

uint8_t CompressedData::set_qtable(uint16_t segment_size) noexcept {
    const uint next_byte = *m_buff_current;
    --segment_size;

    // precision in bytes, not bits
    const uint8_t precision = 1 + static_cast<bool>(next_byte >> 4);
    const uint8_t table_id  = next_byte & 0xf;

    const uint8_t table_size = 64 * precision;

    if (table_size > segment_size) {
        return 0;
    }

    if (table_id == 0 && precision == 1) {
        for (size_t i = 0; i < 64; ++i) {
            m_qtable_buff[m_zig_zag_map[i]] = m_buff_current[i];
        }
        m_qtable = &m_qtable_buff[0];
    }

    return 1 + table_size;
}

uint16_t CompressedData::set_htable(uint16_t segment_size) noexcept {
    const uint next_byte = *m_buff_current;
    --segment_size;

    const uint8_t is_dc = !static_cast<bool>(next_byte >> 4);
    const uint8_t table_id = next_byte & 0xf;

    uint16_t symbols_count = 0;

    for (size_t i = 1; i < 17 && segment_size; ++i, --segment_size) {
        if (m_buff_current[i] > 1 << i) {
            return 0;
        }
        symbols_count += m_buff_current[i];
    }

    if (!segment_size || !symbols_count
                      || (is_dc && symbols_count > 12)
                      || symbols_count > 162
                      || symbols_count > segment_size) {
        return 0;
    }

    if (table_id == 0) {
        if (is_dc) {
            m_dc_htable_histogram = m_buff_current + 1;
            m_dc_htable_symbols = m_buff_current + 1 + 16;
        }
        else {
            m_ac_htable_histogram = m_buff_current + 1;
            m_ac_htable_symbols = m_buff_current + 1 + 16;
        }
    }

    return 1 + 16 + symbols_count;
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

    const auto segment_size = m_data.read_size();

    // seek to the end of segment
    if (!segment_size || !m_data.seek(*segment_size)) {
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
        const uint8_t qtable_size = m_data.set_qtable(*segment_size);

        // any invalid qtable_size gets returned as 0
        if (!qtable_size) {
            SET_NEXT_STATE(StateID::ERROR_CORR);
            return;
        }

        // note that a valid qtable_size is *always* lte *segment_size
        // (no integer overflow possible)
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
        const uint16_t htable_size = m_data.set_htable(*segment_size);

        // any invalid htable_size gets returned as 0
        if (!htable_size) {
            SET_NEXT_STATE(StateID::ERROR_CORR);
            return;
        }

        // note that a valid htable_size is *always* lte *segment_size
        // (no integer overflow possible)
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

    auto segment_size = m_data.read_size();

    if (!segment_size || *segment_size > m_data.size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    // only the 3-component YUV color mode is supported
    if (*segment_size != 15) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    const auto precision = m_data.read_uint8();
    const auto height = m_data.read_uint16();
    const auto width = m_data.read_uint16();
    auto components_count = m_data.read_uint8();

    if (*precision != 8 || !*height
                        || !*width
                        || *components_count != 3) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    m_data.m_height = *height;
    m_data.m_width = *width;

    while ((*components_count)--) {
        const auto component_id = m_data.read_uint8();
        const auto sampling_factor = m_data.read_uint8();
        const auto qtable_id = m_data.read_uint8();

        // supported component IDs: 1, 2 and 3 (Y, U and V channel, respectively)
        if (*component_id == 0 || *component_id > 3 ) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
        // component 1 must use qtable 0 and sampling factor 0x21
        if (*component_id == 1 && (*qtable_id != 0 || *sampling_factor != 0x21)) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
        // components 2 and 3 must use sampling factor 0x11
        if (*component_id > 1 && *sampling_factor != 0x11) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
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

    const auto segment_size = m_data.read_size();

    if (!segment_size || !m_data.seek(*segment_size)) {
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
