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
    m_state(State<StateID::ENTRY>::instance(this)),
    m_buff_start(buff),
    m_buff_current(buff),
    m_buff_end(buff + size)
    {}

StateID Jpeg::parse_header() {
    while (!m_state->is_final_state()) {
        m_state = m_state->parse_header();
    }

    return m_state->getID();
}




////////////////
// IState public:
IState::IState(Jpeg* context) noexcept :
    m_jpeg(context)
    {}

IState::~IState() {}

bool IState::is_final_state() const noexcept {
    return getID() < StateID::ENTRY;
}





////////////////////
// State public:
template<>
IState* State<StateID::ENTRY>::parse_header() const {
    std::cout << "Entered state ENTRY\n";

    auto next_marker = m_jpeg->read_uint16();

    if (!next_marker) {
        return State<StateID::ERROR_PEOB>::instance(m_jpeg);
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::SOI:
            std::cout << "Found marker: SOI\n";
            return State<StateID::SOI>::instance(m_jpeg);

        default:
            std::cout << "Unexpected or unrecognized marker: " << std::hex << *next_marker << "\n";
            return State<StateID::ERROR_UUM>::instance(m_jpeg);
    }
}

template<>
IState* State<StateID::SOI>::parse_header() const {
    std::cout << "Entered state SOI\n";

    auto next_marker = m_jpeg->read_uint16();

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::APP0:
            std::cout << "Found marker: APP0\n";
            return State<StateID::APP0>::instance(m_jpeg);

        default:
            std::cout << "Unexpected or unrecognized marker: " << std::hex << *next_marker << "\n";
            return State<StateID::ERROR_UUM>::instance(m_jpeg);
    }
}

template<>
IState* State<StateID::APP0>::parse_header() const {
    std::cout << "Entered state APP0\n";

    auto size = m_jpeg->read_uint16();

    if (!size || !m_jpeg->seek(*size - 2)) {
        return State<StateID::ERROR_PEOB>::instance(m_jpeg);
    }

    auto next_marker = m_jpeg->read_uint16();

    if (!next_marker) {
        return State<StateID::ERROR_PEOB>::instance(m_jpeg);
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT\n";
            return State<StateID::DQT>::instance(m_jpeg);

        default:
            std::cout << "Unexpected or unrecognized marker: " << std::hex << *next_marker << "\n";
            return State<StateID::ERROR_UUM>::instance(m_jpeg);
    }
}

template<>
IState* State<StateID::DQT>::parse_header() const {
    std::cout << "Entered state DQT\n";

    auto size = m_jpeg->read_uint16();

    if (!size || !m_jpeg->seek(*size - 2)) {
        return State<StateID::ERROR_PEOB>::instance(m_jpeg);
    }

    auto next_marker = m_jpeg->read_uint16();

    if (!next_marker) {
        return State<StateID::ERROR_PEOB>::instance(m_jpeg);
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT\n";
            return State<StateID::DQT>::instance(m_jpeg);

        case StateID::DHT:
            std::cout << "Found marker: DHT\n";
            return State<StateID::EXIT_OK>::instance(m_jpeg);

        default:
            std::cout << "Unexpected or unrecognized marker: " << std::hex << *next_marker << "\n";
            return State<StateID::ERROR_UUM>::instance(m_jpeg);
    }
}

template<>
IState* State<StateID::EOI>::parse_header() const {
    std::cout << "Entered state EOI\n";

    return State<StateID::EXIT_OK>::instance(m_jpeg);
}

template<>
IState* State<StateID::EXIT_OK>::parse_header() const {
    return State<StateID::EXIT_OK>::instance(m_jpeg);
}

template<>
IState* State<StateID::ERROR_PEOB>::parse_header() const {
    return State<StateID::ERROR_PEOB>::instance(m_jpeg);
}

template<>
IState* State<StateID::ERROR_UUM>::parse_header() const {
    return State<StateID::ERROR_UUM>::instance(m_jpeg);
}
