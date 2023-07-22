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
    m_internal_state(State<StateID::ENTRY>(this)),
    m_state(&m_internal_state),
    m_buff_start(buff),
    m_buff_current(buff),
    m_buff_end(buff + size)
    {}

StateID Jpeg::parse_header() {
    while (!m_state->is_final_state()) {
        m_state->parse_header();
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
void State<StateID::ENTRY>::parse_header() const {
    std::cout << "Entered state ENTRY\n";

    auto next_marker = m_jpeg->read_uint16();

    if (!next_marker) {
        m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_PEOB>(m_jpeg);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::SOI:
            std::cout << "Found marker: SOI (0x" << std::hex << *next_marker << ")\n";
            m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::SOI>(m_jpeg);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_UUM>(m_jpeg);
    }
}

template<>
void State<StateID::SOI>::parse_header() const {
    std::cout << "Entered state SOI\n";

    auto next_marker = m_jpeg->read_uint16();

    if (static_cast<StateID>(*next_marker) >= StateID::APP0 &&
        static_cast<StateID>(*next_marker) <= StateID::APP15) {
        std::cout << "Found marker: APPN (0x" << std::hex << *next_marker << ")\n";
        m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::APP0>(m_jpeg);
    }
    else {
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_UUM>(m_jpeg);
    }
}

template<>
void State<StateID::APP0>::parse_header() const {
    std::cout << "Entered state APP0\n";

    auto size = m_jpeg->read_uint16();

    if (!size || !m_jpeg->seek(*size - 2)) {
        m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_PEOB>(m_jpeg);
        return;
    }

    auto next_marker = m_jpeg->read_uint16();

    if (!next_marker) {
        m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_PEOB>(m_jpeg);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT (0x" << std::hex << *next_marker << ")\n";
            m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::DQT>(m_jpeg);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_UUM>(m_jpeg);
    }
}

template<>
void State<StateID::DQT>::parse_header() const {
    std::cout << "Entered state DQT\n";

    auto size = m_jpeg->read_uint16();

    if (!size || !m_jpeg->seek(*size - 2)) {
        m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_PEOB>(m_jpeg);
        return;
    }

    auto next_marker = m_jpeg->read_uint16();

    if (!next_marker) {
        m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_PEOB>(m_jpeg);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {
        case StateID::DQT:
            std::cout << "Found marker: DQT (0x" << std::hex << *next_marker << ")\n";
            m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::DQT>(m_jpeg);
            break;

        case StateID::DHT:
            std::cout << "Found marker: DHT (0x" << std::hex << *next_marker << ")\n";
            m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::EXIT_OK>(m_jpeg);
            break;

        default:
            std::cout << "Unexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::ERROR_UUM>(m_jpeg);
    }
}

template<>
void State<StateID::EOI>::parse_header() const {
    std::cout << "Entered state EOI\n";

    m_jpeg->m_state = new (m_jpeg->m_state) State<StateID::EXIT_OK>(m_jpeg);
}
