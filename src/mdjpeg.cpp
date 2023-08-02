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

bool CompressedData::seek(size_t rel_pos) noexcept {
    if (rel_pos < size_remaining()) {
        m_buff_current += rel_pos;
        return true;
    }
    else {
        return false;
    }
}

std::optional<uint8_t> CompressedData::peek(size_t rel_pos) const noexcept {
    if (rel_pos < size_remaining()) {
        return *(m_buff_current + rel_pos);
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
        uint16_t result = *m_buff_current << 8 | *(m_buff_current + 1);
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

    auto next_marker = m_data.read_marker();

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

    auto next_marker = m_data.read_marker();

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

    auto size = m_data.read_uint16();

    if (!size || !m_data.seek(*size - 2)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    auto next_marker = m_data.read_marker();

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

    auto size = m_data.read_uint16();

    if (!size || !m_data.seek(*size - 2)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    auto next_marker = m_data.read_marker();

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

    auto size = m_data.read_uint16();

    if (!size || !m_data.seek(*size - 2)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    auto next_marker = m_data.read_marker();

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

    auto size = m_data.read_uint16();

    if (!size || !m_data.seek(*size - 2)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    auto next_marker = m_data.read_marker();

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

    auto size = m_data.read_uint16();

    if (!size || !m_data.seek(*size - 2)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
    }
    else {
        SET_NEXT_STATE(StateID::STREAM);
    }
}

template<>
void ConcreteState<StateID::STREAM>::parse_header() {
    // std::cout << "Entered state STREAM\n";

    auto current_byte = m_data.peek();

    if (!current_byte) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    if (*current_byte == 0xff) {
        auto next_marker = m_data.read_marker();

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
        std::cout << "Processing byte from stream: 0x" << std::hex << static_cast<uint>(*current_byte) << "\n";
    }
}

template<>
void ConcreteState<StateID::EOI>::parse_header() {
    std::cout << "Entered state EOI\n";

    SET_NEXT_STATE(StateID::EXIT_OK);
}
