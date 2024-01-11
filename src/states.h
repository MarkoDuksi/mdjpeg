#pragma once

#include <stdint.h>


/// \brief All the different state IDs for identifying state machine's various states.
///
/// \note Marker values used for state IDs compiled from
/// - https://github.com/dannye/jed/blob/master/src/jpg.h
/// - https://webdocs.cs.ualberta.ca/~jag/courses/ImProc/lectures2001/lec26/Lec26jpegCompr.PDF
enum class StateID : uint16_t {

    HEADER_OK  = 0, ///< Valid (custom final state)
    ERROR_PEOB = 1, ///< Premature End of Buffer (custom final state)
    ERROR_UUM  = 2, ///< Unsupported or Unrecognized Marker (custom final state)
    ERROR_UPAR = 3, ///< Unsupported PARameter (custom final state)
    ERROR_CORR = 4, ///< CORRupted data (custom final state)
    ERROR_GEN  = 5, ///< GENeric invalid (custom final state)

    ENTRY  = 100, ///< Inital state (custom transient state)

    TEM = 0xff01, ///< Temporary, arithmetic coding

    // Reserved from 0xff02 to 0xffbf

    SOF0 = 0xffc0, ///< Start of Frame - Baseline
    SOF1 = 0xffc1, ///< Start of Frame - Extended sequential
    SOF2 = 0xffc2, ///< Start of Frame - Progressive
    SOF3 = 0xffc3, ///< Start of Frame - Lossless

    DHT = 0xffc4, ///< Define %Huffman Tables

    SOF5 = 0xffc5, ///< Start of Frame - Sequential, differential
    SOF6 = 0xffc6, ///< Start of Frame - Progressive, differential
    SOF7 = 0xffc7, ///< Start of Frame - Lossless, differential

    JPG = 0xffc8, ///< Reserved

    SOF9  = 0xffc9, ///< Start of Frame - Extended sequential, arithmetic coding
    SOF10 = 0xffca, ///< Start of Frame - Progressive, arithmetic coding
    SOF11 = 0xffcb, ///< Start of Frame - Lossless, arithmetic coding

    DAC = 0xffcc, ///< Define Arithmetic Conditions

    SOF13 = 0xffcd, ///< Start of Frame - Sequential, differential, arithmetic coding
    SOF14 = 0xffce, ///< Start of Frame - Progressive, differential, arithmetic coding
    SOF15 = 0xffcf, ///< Start of Frame - Lossless, differential, arithmetic coding

    RST0 = 0xffd0, ///< Reset
    RST1 = 0xffd1, ///< Reset
    RST2 = 0xffd2, ///< Reset
    RST3 = 0xffd3, ///< Reset
    RST4 = 0xffd4, ///< Reset
    RST5 = 0xffd5, ///< Reset
    RST6 = 0xffd6, ///< Reset
    RST7 = 0xffd7, ///< Reset

    SOI = 0xffd8, ///< Start of Image
    EOI = 0xffd9, ///< End of Image
    SOS = 0xffda, ///< Start of Scan
    DQT = 0xffdb, ///< Define Quantization Tables
    DNL = 0xffdc, ///< Define Number of Lines
    DRI = 0xffdd, ///< Define Restart Interval
    DHP = 0xffde, ///< Define Hierarchical Progression
    EXP = 0xffdf, ///< Expand Reference Components

    APP0  = 0xffe0, ///< Application Specific Data
    APP1  = 0xffe1, ///< Application Specific Data
    APP2  = 0xffe2, ///< Application Specific Data
    APP3  = 0xffe3, ///< Application Specific Data
    APP4  = 0xffe4, ///< Application Specific Data
    APP5  = 0xffe5, ///< Application Specific Data
    APP6  = 0xffe6, ///< Application Specific Data
    APP7  = 0xffe7, ///< Application Specific Data
    APP8  = 0xffe8, ///< Application Specific Data
    APP9  = 0xffe9, ///< Application Specific Data
    APP10 = 0xffea, ///< Application Specific Data
    APP11 = 0xffeb, ///< Application Specific Data
    APP12 = 0xffec, ///< Application Specific Data
    APP13 = 0xffed, ///< Application Specific Data
    APP14 = 0xffee, ///< Application Specific Data
    APP15 = 0xffef, ///< Application Specific Data

    JPG0  = 0xfff0, ///< Reserved
    JPG1  = 0xfff1, ///< Reserved
    JPG2  = 0xfff2, ///< Reserved
    JPG3  = 0xfff3, ///< Reserved
    JPG4  = 0xfff4, ///< Reserved
    JPG5  = 0xfff5, ///< Reserved
    JPG6  = 0xfff6, ///< Reserved
    JPG7  = 0xfff7, ///< Reserved
    JPG8  = 0xfff8, ///< Reserved
    JPG9  = 0xfff9, ///< Reserved
    JPG10 = 0xfffa, ///< Reserved
    JPG11 = 0xfffb, ///< Reserved
    JPG12 = 0xfffc, ///< Reserved
    JPG13 = 0xfffd, ///< Reserved

    COM = 0xfffe ///< Comment
};


class JpegDecoder;
class JpegReader;


/// \brief Abstract base class for ConcreteState of the state machine.
class State {

    protected:
        ~State() = default;

    public:

        explicit State(JpegDecoder* decoder) noexcept :
            m_decoder(decoder)
            {}

        State(const State& other) = delete;
        State& operator=(const State& other) = delete;
        State(State&& other) = delete;
        State& operator=(State&& other) = delete;

        /// \brief Checks if the current ConcreteState is a final state.
        bool is_final_state() const noexcept {

            return getID() < StateID::ENTRY;
        }

        /// \brief Dispatches getting the current ConcreteState StateID.
        virtual StateID getID() const noexcept = 0;

        /// \brief Dispatches parsing step to the current ConcreteState.
        virtual void parse_header(JpegReader& reader) noexcept = 0;

    protected:

        JpegDecoder* const m_decoder;
};


/// \brief Provides generic ConcreteState facilities.
///
/// \note Doxygen (1.9.1 as well as 1.9.8) still cannot correctly handle
/// documentation for member function specializations of a class template.
/// This class contains \ref temp_specs "seven of them in a group" but not all
/// of their documentation gets generated by Doxygen, some of it is duplicated
/// and all of it lacks any readability. For better overview check the raw
/// comments in the source code of states.h.
template <StateID state_id>
class ConcreteState : public State {

    public:

        explicit ConcreteState(JpegDecoder* decoder) noexcept :
            State(decoder)
            {}

        /// \brief Gets the actual StateID when called through State::getID.
        StateID getID() const noexcept override {

            return state_id;
        }
        
        /// \brief Generic version of the step function.
        ///
        /// \note Empty body is provided only to satisfy the linker but
        /// otherwise not useful unless \ref temp_specs "specialized" to a
        /// particular StateID.
        void parse_header([[maybe_unused]] JpegReader& reader) noexcept override {}
};


/// \addtogroup temp_specs Specialized ConcreteState member functions
/// \brief Step function specialization for the state defined by StateID::ENTRY.
///
/// Validates the next expected marker: \e SOI. Sets the next state of the state
/// machine to the one defined by StateID::SOI or one of the invalid final
/// states, depending on the validation outcome.
template<>
void ConcreteState<StateID::ENTRY>::parse_header(JpegReader& reader) noexcept;

/// \addtogroup temp_specs
/// \brief Step function specialization for the state defined by StateID::SOI.
///
/// Validates the next expected marker: \e APP0. Sets the next state of the
/// state machine to the one defined by StateID::APP0 or one of the invalid
/// final states, depending on the validation outcome.
///
/// \note Other APP markers are not supported.
template<>
void ConcreteState<StateID::SOI>::parse_header(JpegReader& reader) noexcept;

/// \addtogroup temp_specs
/// \brief Step function specialization for the state defined by StateID::APP0.
///
/// Skips through \e APP0 segment. Validates the next expected marker: \e DQT,
/// \e DHT or \e SOF0. Sets the next state of the state machine to the one
/// defined by StateID::DQT, StateID::DHT, StateID::SOF0 or one of the invalid
/// final states, depending on the validation outcome.
///
/// \note Other SOF markers are not supported.
template<>
void ConcreteState<StateID::APP0>::parse_header(JpegReader& reader) noexcept;

/// \addtogroup temp_specs
/// \brief Step function specialization for the state defined by StateID::DQT.
///
/// Sets luma quantization table, skips over chroma quantization table.
/// Validates the next expected marker: \e DQT, \e DHT, \e SOF0 or \e SOS. Sets
/// the next state of the state machine to the one defined by StateID::DQT,
/// StateID::DHT, StateID::SOF0, StateID::SOS or one of the invalid final
/// states, depending on the validation outcome.
///
/// \note Other SOF markers are not supported.
template<>
void ConcreteState<StateID::DQT>::parse_header(JpegReader& reader) noexcept;

/// \addtogroup temp_specs
/// \brief Step function specialization for the state defined by StateID::DHT.
///
/// Sets %Huffman tables. Validates the next expected marker: \e DQT, \e DHT,
/// \e SOF0 or \e SOS. Sets the next state of the state machine to the one
/// defined by StateID::DQT, StateID::DHT, StateID::SOF0, StateID::SOS or one of
/// the invalid final states, depending on the validation outcome.
///
/// \note Other SOF markers are not supported.
template<>
void ConcreteState<StateID::DHT>::parse_header(JpegReader& reader) noexcept;

/// \addtogroup temp_specs
/// \brief Step function specialization for the state defined by StateID::SOF0.
///
/// Validates and stores image metadata (quantization table precision, height,
/// width, color space and chroma subsampling scheme). Validates the next
/// expected marker: \e DQT, \e DHT or \e SOS. Sets the next state of the state
/// machine to the one defined by StateID::DQT, StateID::DHT, StateID::SOS or
/// one of the invalid final states, depending on validations outcome.
///
/// \note Quantization table precision other than 8 bits is not supported. Color
/// spaces other than Y'CbCr are not supported. Chroma subsampling schemes other
/// than 4:4:4 and 4:2:2 are not supported.
template<>
void ConcreteState<StateID::SOF0>::parse_header(JpegReader& reader) noexcept;

/// \addtogroup temp_specs
/// \brief Step function specialization for the state defined by StateID::SOS.
///
/// Validates that all prerequisite metadata has been found and set. Validates
/// that \e SOS segment describes the supported baseline JPEG mode. Marks the
/// start of entropy coded segment. Sets the next state of the state machine to
/// the one defined by StateID::HEADER_OK or one of the invalid final states,
/// depending on validations outcome.
///
/// \note Modes other than baseline JPEG are not supported.
template<>
void ConcreteState<StateID::SOS>::parse_header(JpegReader& reader) noexcept;
