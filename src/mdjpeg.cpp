#include "mdjpeg.h"


////////////////
// Jpeg private:
size_t Jpeg::size_remaining() const noexcept {
    return m_buff_end - m_buff_current;
}

bool Jpeg::seek(size_t rel_pos) noexcept {
    if (rel_pos < size_remaining()) {
        m_buff_current += rel_pos;
        return true;
    }
    else {
        return false;
    }
}

std::optional<uint8_t> Jpeg::peek(size_t rel_pos) const noexcept {
    if (rel_pos < size_remaining()) {
        return *(m_buff_current + rel_pos);
    }
    else {
        return {};
    }
}

std::optional<uint8_t> Jpeg::read_uint8() noexcept {
    if (size_remaining() > 0) {
        return *(m_buff_current++);
    }
    else {
        return {};
    }
}

std::optional<uint16_t> Jpeg::read_uint16() noexcept {
    if (size_remaining() >= 2) {
        uint16_t result = *m_buff_current << 8 | *(m_buff_current + 1);
        m_buff_current += 2;
        return result;
    }
    else {
        return {};
    }
}

// Jpeg public:
Jpeg::Jpeg(uint8_t *buff, size_t size) noexcept :
    m_state(SpecState<StateID::ENTRY>::get(this)),
    m_buff_start(buff),
    m_buff_current(buff),
    m_buff_end(buff + size)
    {}

StateID Jpeg::parse_header() {
    while (!m_state->is_final()) {
        m_state = m_state->parse_header();
    }

    return m_state->getID();
}




////////////////
// State public:
State::State(Jpeg* context) noexcept :
    m_context(context)
    {}

bool State::is_final() const noexcept {
    return getID() < StateID::ENTRY;
}

State::~State() {}




////////////////////
// SpecState public:
template<>
State* SpecState<StateID::ENTRY>::parse_header() const {
    std::cout << "Entered state ENTRY\n";

    auto next_marker = m_context->read_uint16();

    if (!next_marker) {
        return SpecState<StateID::ERROR_PEOB>::get(m_context);
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::SOI:
            std::cout << "Found marker: SOI\n";
            return SpecState<StateID::SOI>::get(m_context);

        default:
            std::cout << "Unexpected or unrecognized marker: " << std::hex << *next_marker << "\n";
            return SpecState<StateID::ERROR_UUM>::get(m_context);
    }
}

template<>
State* SpecState<StateID::SOI>::parse_header() const {
    std::cout << "Entered state SOI\n";

    auto next_marker = m_context->read_uint16();

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::APP0:
            std::cout << "Found marker: APP0\n";
            return SpecState<StateID::APP0>::get(m_context);

        default:
            std::cout << "Unexpected or unrecognized marker: " << std::hex << *next_marker << "\n";
            return SpecState<StateID::ERROR_UUM>::get(m_context);
    }
}

template<>
State* SpecState<StateID::APP0>::parse_header() const {
    std::cout << "Entered state APP0\n";

    auto size = m_context->read_uint16();

    if (!size || !m_context->seek(*size - 2)) {
        return SpecState<StateID::ERROR_PEOB>::get(m_context);
    }

    auto next_marker = m_context->read_uint16();

    if (!next_marker) {
        return SpecState<StateID::ERROR_PEOB>::get(m_context);
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT\n";
            return SpecState<StateID::DQT>::get(m_context);

        default:
            std::cout << "Unexpected or unrecognized marker: " << std::hex << *next_marker << "\n";
            return SpecState<StateID::ERROR_UUM>::get(m_context);
    }
}

template<>
State* SpecState<StateID::DQT>::parse_header() const {
    std::cout << "Entered state DQT\n";

    auto size = m_context->read_uint16();

    if (!size || !m_context->seek(*size - 2)) {
        return SpecState<StateID::ERROR_PEOB>::get(m_context);
    }

    auto next_marker = m_context->read_uint16();

    if (!next_marker) {
        return SpecState<StateID::ERROR_PEOB>::get(m_context);
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT\n";
            return SpecState<StateID::DQT>::get(m_context);

        case StateID::DHT:
            std::cout << "Found marker: DHT\n";
            return SpecState<StateID::EXIT_OK>::get(m_context);

        default:
            std::cout << "Unexpected or unrecognized marker: " << std::hex << *next_marker << "\n";
            return SpecState<StateID::ERROR_UUM>::get(m_context);
    }
}

template<>
State* SpecState<StateID::EOI>::parse_header() const {
    std::cout << "Entered state EOI\n";

    return SpecState<StateID::EXIT_OK>::get(m_context);
}

template<>
State* SpecState<StateID::EXIT_OK>::parse_header() const {
    return SpecState<StateID::EXIT_OK>::get(m_context);
}

template<>
State* SpecState<StateID::ERROR_PEOB>::parse_header() const {
    return SpecState<StateID::ERROR_PEOB>::get(m_context);
}

template<>
State* SpecState<StateID::ERROR_UUM>::parse_header() const {
    return SpecState<StateID::ERROR_UUM>::get(m_context);
}
