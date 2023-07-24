#include "mdjpeg.h"


////////////////
// JpegDecoder public:

JpegDecoder::JpegDecoder(uint8_t *buff, size_t size) noexcept :
    m_data{buff, buff, buff + size},
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
    m_data(data),
    m_decoder(decoder)
    {}

State::~State() {}

bool State::is_final_state() const noexcept {
    return getID() < StateID::ENTRY;
}

////////////////
// State protected:
size_t State::size_remaining() const noexcept {
    return m_data.m_buff_end - m_data.m_buff_current;
}

bool State::seek(size_t rel_pos) noexcept {
    if (rel_pos < size_remaining()) {
        m_data.m_buff_current += rel_pos;
        return true;
    }
    else {
        return false;
    }
}

std::optional<uint8_t> State::peek(size_t rel_pos) const noexcept {
    if (rel_pos < size_remaining()) {
        return *(m_data.m_buff_current + rel_pos);
    }
    else {
        return {};
    }
}

std::optional<uint8_t> State::read_uint8() noexcept {
    if (size_remaining() > 0) {
        return *(m_data.m_buff_current++);
    }
    else {
        return {};
    }
}

std::optional<uint16_t> State::read_uint16() noexcept {
    if (size_remaining() >= 2) {
        uint16_t result = *m_data.m_buff_current << 8 | *(m_data.m_buff_current + 1);
        m_data.m_buff_current += 2;
        return result;
    }
    else {
        return {};
    }
}

std::optional<uint16_t> State::read_marker() noexcept {
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




////////////////////////
// ConcreteState public:

#define SET_NEXT_STATE(state_id) m_decoder->m_state_ptr = new (m_decoder->m_state_ptr) ConcreteState<state_id>(m_decoder, m_data)

template<>
void ConcreteState<StateID::ENTRY>::parse_header() {
    std::cout << "Entered state ENTRY\n";

    auto next_marker = read_marker();

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

    auto next_marker = read_marker();

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

    auto size = read_uint16();

    if (!size || !seek(*size - 2)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    auto next_marker = read_marker();

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

    auto size = read_uint16();

    if (!size || !seek(*size - 2)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    auto next_marker = read_marker();

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
            SET_NEXT_STATE(StateID::EXIT_OK);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::EOI>::parse_header() {
    std::cout << "Entered state EOI\n";

    SET_NEXT_STATE(StateID::EXIT_OK);
}
