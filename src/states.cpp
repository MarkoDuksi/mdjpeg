#include "states.h"

#include <iostream>

#include "JpegReader.h"
#include "JpegDecoder.h"


State::State(JpegDecoder* const decoder, JpegReader* const reader) noexcept :
    m_decoder(decoder),
    m_reader(reader)
    {}

State::~State() {}

bool State::is_final_state() const noexcept {
    return getID() < StateID::ENTRY;
}


#define SET_NEXT_STATE(state_id) m_decoder->m_istate = new (m_decoder->m_istate) ConcreteState<state_id>(m_decoder, m_reader)

template<>
void ConcreteState<StateID::ENTRY>::parse_header() {
    std::cout << "Entered state ENTRY\n";

    const auto next_marker = m_reader->read_marker();

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

    const auto next_marker = m_reader->read_marker();

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

    const auto segment_size = m_reader->read_size();

    // seek to the end of segment
    if (!segment_size || !m_reader->seek(*segment_size)) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    const auto next_marker = m_reader->read_marker();

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

    auto segment_size = m_reader->read_size();

    if (!segment_size || *segment_size > m_reader->size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    while (*segment_size) {
        const uint qtable_size = m_decoder->m_header.set_qtable(m_decoder->m_reader, *segment_size);

        // any invalid qtable_size gets returned as 0
        if (!qtable_size) {
            SET_NEXT_STATE(StateID::ERROR_CORR);
            return;
        }

        // any valid qtable_size is *always* lte *segment_size
        // (no integer overflow possible)
        *segment_size -= qtable_size;
    }

    const auto next_marker = m_reader->read_marker();

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

    auto segment_size = m_reader->read_size();

    if (!segment_size || *segment_size > m_reader->size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    while (*segment_size) {
        const uint htable_size = m_decoder->m_header.set_htable(m_decoder->m_reader, *segment_size);

        // any invalid htable_size gets returned as 0
        if (!htable_size) {
            SET_NEXT_STATE(StateID::ERROR_CORR);
            return;
        }

        // any valid htable_size is *always* lte *segment_size
        // no reading beyond buff + size - 1 (as provided to JpegDecoder constructor)
        m_reader->seek(htable_size);
        // no integer overflow possible
        *segment_size -= htable_size;
    }

    const auto next_marker = m_reader->read_marker();

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

    const auto segment_size = m_reader->read_size();

    if (!segment_size || *segment_size > m_reader->size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    // only the 3-component YUV color mode is supported
    if (*segment_size != 15) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    const auto precision = m_reader->read_uint8();
    const auto height = m_reader->read_uint16();
    const auto width = m_reader->read_uint16();
    auto components_count = m_reader->read_uint8();

    if (*precision != 8 || !*height
                        || !*width
                        || *components_count != 3) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    m_decoder->m_header.img_height = *height;
    m_decoder->m_header.img_width = *width;
    m_decoder->m_header.img_mcu_horiz_count = (*width + 7) / 8;
    m_decoder->m_header.img_mcu_vert_count = (*height + 7) / 8;

    while ((*components_count)--) {
        const auto component_id = m_reader->read_uint8();
        const auto sampling_factor = m_reader->read_uint8();
        const auto qtable_id = m_reader->read_uint8();

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
                m_decoder->m_header.horiz_subsampling = bool((*sampling_factor >> 4) - 1);
            }
        }
        // components 2 and 3 must use sampling factors 0x11
        if (*component_id > 1 && *sampling_factor != 0x11) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
    }

    const auto next_marker = m_reader->read_marker();

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

template<>
void ConcreteState<StateID::SOS>::parse_header() {
    std::cout << "Entered state SOS\n";

    // before this point, DQT, DHT and SOF0 segments must have been parsed
    if (!m_decoder->m_header.is_set()) {
        SET_NEXT_STATE(StateID::ERROR_UUM);
        return;
    }

    const auto segment_size = m_reader->read_size();

    if (!segment_size || *segment_size > m_reader->size_remaining()) {
        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    // only the 3-component YUV color mode is supported
    if (*segment_size != 10) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    auto components_count = m_reader->read_uint8();

    if (*components_count != 3) {
        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    while ((*components_count)--) {
        const auto component_id = m_reader->read_uint8();
        const auto dc_ac_table_ids = m_reader->read_uint8();

        // component 1 must use htables 0 (DC) and 0 (AC)
        if (*component_id == 1 && *dc_ac_table_ids != 0 ) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
    }

    const uint start_of_selection = *m_reader->read_uint8();
    const uint end_of_selection = *m_reader->read_uint8();
    const uint successive_approximation = *m_reader->read_uint8();

    // only baseline JPEG is supported
    if (start_of_selection != 0 || end_of_selection != 63
                                || successive_approximation != 0) {
            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
    }

    // set a mark for the begining of ECS
    m_reader->m_buff_start_of_ECS = m_reader->m_buff_current_byte;

    SET_NEXT_STATE(StateID::HEADER_OK);
}
