#include "mdjpeg.h"


// Jpeg public:
Jpeg::Jpeg(uint8_t *buff, size_t size) :
    m_state(SpecState<StateID::ENTRY>::get(this)),
    m_buff_start(buff),
    m_buff_current(buff),
    m_buff_end(buff + size)
    {}

size_t Jpeg::size_remaining() {
    return m_buff_end - m_buff_current;
}

uint8_t Jpeg::read_uint8() {
    return *(m_buff_current++);
}

uint16_t Jpeg::read_uint16() {
    return read_uint8() << 8 | read_uint8();
}

void Jpeg::parse_header() {
    while (m_state->getID() != StateID::EXIT_OK && m_state->getID() != StateID::ERROR) {
        m_state = m_state->parse();
    }
    if (m_state->getID() == StateID::EXIT_OK) {
        std::cout << "Finished in EXIT_OK state\n";
    }
    else {
        std::cout << "Finished in ERROR state\n";
    }
}


// State public:
State::State(Jpeg* context) :
    m_context(context)
    {}

State::~State() {}


// SpecState dependent behaviour:
template<>
State* SpecState<StateID::ENTRY>::parse() {
    std::cout << "Entered state ENTRY\n";
    uint16_t next;

    if (m_context->size_remaining() >= 2) {
        next = m_context->read_uint16();
    }
    else {
        std::cout << "Premature end of buffer\n";

        return SpecState<StateID::ERROR>::get(m_context);
    }

    if (next == static_cast<uint16_t>(StateID::SOI)) {
        std::cout << "Found marker: SOI\n";

        return SpecState<StateID::SOI>::get(m_context);
    }
    else {
        std::cout << "Unsupported/unrecognized marker: " << std::hex << next << "\n";

        return SpecState<StateID::ERROR>::get(m_context);
    }
}

template<>
State* SpecState<StateID::SOI>::parse(){
    std::cout << "Entered state SOI\n";
    uint16_t next;

    if (m_context->size_remaining() >= 2) {
        next = m_context->read_uint16();
    }
    else {
        std::cout << "Premature end of buffer\n";

        return SpecState<StateID::ERROR>::get(m_context);
    }

    if (next == static_cast<uint16_t>(StateID::APP0)) {
        std::cout << "Found marker: APP0\n";

        return SpecState<StateID::APP0>::get(m_context);
    }
    else {
        std::cout << "Unsupported/unrecognized marker: " << std::hex << next << "\n";

        return SpecState<StateID::ERROR>::get(m_context);
    }
}

template<>
State* SpecState<StateID::APP0>::parse() {
    std::cout << "Entered state APP0\n";
    uint16_t next;

    if (m_context->size_remaining() >= 2) {
        next = m_context->read_uint16();
    }
    else {
        std::cout << "Premature end of buffer\n";

        return SpecState<StateID::ERROR>::get(m_context);
    }

    if (next == 0x10) {
        std::cout << "Found: 0x10\n";

        return SpecState<StateID::EOI>::get(m_context);
    }
    else {
        std::cout << "Unsupported marker: 0x" << std::hex << next << "\n";

        return SpecState<StateID::ERROR>::get(m_context);
    }
}

template<>
State* SpecState<StateID::EOI>::parse() {
    std::cout << "Entered state EOI\n";
    std::cout << "Nothing to do here\n";

    return SpecState<StateID::EXIT_OK>::get(m_context);
}

template<>
State* SpecState<StateID::EXIT_OK>::parse() {
    // sentinel valid final state

    return this;
}

template<>
State* SpecState<StateID::ERROR>::parse() {
    // sentinel invalid final state

    return this;
}
