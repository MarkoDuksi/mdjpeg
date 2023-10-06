#include "states.h"

#ifdef PRINT_STATES_FLOW
    #include <iostream>
#endif

#include <sys/types.h>

#include "JpegReader.h"
#include "JpegDecoder.h"


State::State(JpegDecoder* const decoder) noexcept :
    m_decoder(decoder)
    {}

State::~State() {}

bool State::is_final_state() const noexcept {

    return getID() < StateID::ENTRY;
}


#define SET_NEXT_STATE(state_id) m_decoder->m_istate = new (m_decoder->m_istate) ConcreteState<state_id>(m_decoder)

template<>
void ConcreteState<StateID::ENTRY>::parse_header(JpegReader& reader) {

    #ifdef PRINT_STATES_FLOW
        std::cout << "Entered state ENTRY\n";
    #endif

    const auto next_marker = reader.read_marker();

    if (!next_marker) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {

        case StateID::SOI:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: SOI (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::SOI);
            break;

        default:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            #endif

            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::SOI>::parse_header(JpegReader& reader) {

    #ifdef PRINT_STATES_FLOW
        std::cout << "Entered state SOI\n";
    #endif

    const auto next_marker = reader.read_marker();

    if (static_cast<StateID>(*next_marker) >= StateID::APP0 && static_cast<StateID>(*next_marker) <= StateID::APP15) {

        #ifdef PRINT_STATES_FLOW
            std::cout << "\nFound marker: APPN (0x" << std::hex << *next_marker << ")\n";
        #endif

        SET_NEXT_STATE(StateID::APP0);
    }

    else {

        #ifdef PRINT_STATES_FLOW
            std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
        #endif

        SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::APP0>::parse_header(JpegReader& reader) {

    #ifdef PRINT_STATES_FLOW
        std::cout << "Entered state APP0\n";
    #endif

    const auto segment_size = reader.read_segment_size();

    // seek to the end of segment
    if (!segment_size || !reader.seek(*segment_size)) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    const auto next_marker = reader.read_marker();

    if (!next_marker) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {

        case StateID::DQT:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: DQT (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: DHT (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::SOF0:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: SOF0 (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::SOF0);
            break;

        default:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            #endif

            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::DQT>::parse_header(JpegReader& reader) {

    #ifdef PRINT_STATES_FLOW
        std::cout << "Entered state DQT\n";
    #endif

    auto segment_size = reader.read_segment_size();

    if (!segment_size || *segment_size > reader.size_remaining()) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    while (*segment_size) {

        const uint qtable_size = m_decoder->m_dequantizer.set_qtable(reader, *segment_size);

        // any invalid qtable_size gets returned as 0
        if (!qtable_size) {

            SET_NEXT_STATE(StateID::ERROR_CORR);
            return;
        }

        // any valid qtable_size is *always* lte *segment_size
        // no integer overflow possible
        *segment_size -= qtable_size;
    }

    const auto next_marker = reader.read_marker();

    if (!next_marker) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {

        case StateID::DQT:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: DQT (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: DHT (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::SOF0:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: SOF0 (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::SOF0);
            break;

        case StateID::SOS:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: SOS (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::SOS);
            break;

        default:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            #endif

            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::DHT>::parse_header(JpegReader& reader) {

    #ifdef PRINT_STATES_FLOW
        std::cout << "Entered state DHT\n";
    #endif

    auto segment_size = reader.read_segment_size();

    if (!segment_size || *segment_size > reader.size_remaining()) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    while (*segment_size) {

        const uint htable_size = m_decoder->m_huffman.set_htable(reader, *segment_size);

        // any invalid htable_size gets returned as 0
        if (!htable_size) {

            SET_NEXT_STATE(StateID::ERROR_CORR);
            return;
        }

        // any valid htable_size is *always* lte *segment_size
        // no reading beyond buff + size - 1 (as provided to JpegDecoder constructor)
        reader.seek(htable_size);

        // no integer overflow possible
        *segment_size -= htable_size;
    }

    const auto next_marker = reader.read_marker();

    if (!next_marker) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {

        case StateID::DHT:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: DHT (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::DQT:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: DQT (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::SOF0:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: SOF0 (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::SOF0);
            break;

        case StateID::SOS:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: SOS (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::SOS);
            break;

        default:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            #endif

            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::SOF0>::parse_header(JpegReader& reader) {

    #ifdef PRINT_STATES_FLOW
        std::cout << "Entered state SOF0\n";
    #endif

    const auto segment_size = reader.read_segment_size();

    if (!segment_size || *segment_size > reader.size_remaining()) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    // only the 3-component YUV color mode is supported
    if (*segment_size != 15) {

        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    const auto precision = reader.read_uint8();
    const auto height_px = reader.read_uint16();
    const auto width_px = reader.read_uint16();
    auto components_count = reader.read_uint8();

    if (*precision != 8 || !*height_px
                        || !*width_px
                        || *components_count != 3) {

        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    m_decoder->m_frame_info.set_dims(*height_px, *width_px);

    while ((*components_count)--) {

        const auto component_id = reader.read_uint8();
        const auto sampling_factor = reader.read_uint8();
        const auto qtable_id = reader.read_uint8();

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

                m_decoder->m_frame_info.horiz_chroma_subs_factor = *sampling_factor >> 4;
            }
        }

        // components 2 and 3 must use sampling factors 0x11
        if (*component_id > 1 && *sampling_factor != 0x11) {

            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
    }

    const auto next_marker = reader.read_marker();

    if (!next_marker) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    switch (static_cast<StateID>(*next_marker)) {

        case StateID::DQT:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: DQT (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::DQT);
            break;

        case StateID::DHT:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: DHT (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::DHT);
            break;

        case StateID::SOS:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nFound marker: SOS (0x" << std::hex << *next_marker << ")\n";
            #endif

            SET_NEXT_STATE(StateID::SOS);
            break;

        default:

            #ifdef PRINT_STATES_FLOW
                std::cout << "\nUnexpected or unrecognized marker: 0x" << std::hex << *next_marker << "\n";
            #endif

            SET_NEXT_STATE(StateID::ERROR_UUM);
    }
}

template<>
void ConcreteState<StateID::SOS>::parse_header(JpegReader& reader) {

    #ifdef PRINT_STATES_FLOW
        std::cout << "Entered state SOS\n";
    #endif

    // at this point, DQT, DHT and SOF0 segments must have been parsed
    if (!m_decoder->m_dequantizer.is_set() || !m_decoder->m_huffman.is_set()
                                           || !m_decoder->m_frame_info.is_set()) {

        SET_NEXT_STATE(StateID::ERROR_UUM);
        return;
    }

    const auto segment_size = reader.read_segment_size();

    if (!segment_size || *segment_size > reader.size_remaining()) {

        SET_NEXT_STATE(StateID::ERROR_PEOB);
        return;
    }

    // only the 3-component YUV color mode is supported
    if (*segment_size != 10) {

        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    auto components_count = reader.read_uint8();

    if (*components_count != 3) {

        SET_NEXT_STATE(StateID::ERROR_UPAR);
        return;
    }

    while ((*components_count)--) {

        const auto component_id = reader.read_uint8();
        const auto dc_ac_table_ids = reader.read_uint8();

        // component 1 must use htables 0 (DC) and 0 (AC)
        if (*component_id == 1 && *dc_ac_table_ids != 0 ) {

            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
        }
    }

    const uint start_of_selection = *reader.read_uint8();
    const uint end_of_selection = *reader.read_uint8();
    const uint successive_approximation = *reader.read_uint8();

    // only baseline JPEG is supported
    if (start_of_selection != 0 || end_of_selection != 63
                                || successive_approximation != 0) {

            SET_NEXT_STATE(StateID::ERROR_UPAR);
            return;
    }

    reader.mark_start_of_ecs();

    SET_NEXT_STATE(StateID::HEADER_OK);
}
