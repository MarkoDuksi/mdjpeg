#pragma once

#include "stdint.h"

#include <iostream>
#include <optional>

// marker values from: https://github.com/dannye/jed/blob/master/src/jpg.h
enum class StateID : uint16_t {
    // custom final states
    EXIT_OK = 0, // Valid final state
    ERROR_PEOB = 1, // Premature End of Buffer
    ERROR_UUM = 2, // Unexpected or Unrecognized Marker

    // custom transient states
    ENTRY = 10, // Inital state

    // Start of Frame, non-differential, Huffman coding
    SOF0 = 0xffc0, // Baseline DCT
    SOF1 = 0xffc1, // Extended sequential DCT
    SOF2 = 0xffc2, // Progressive DCT
    SOF3 = 0xffc3, // Lossless (sequential)

    // Start of Frame, differential, Huffman coding
    SOF5 = 0xffc5, // Differential sequential DCT
    SOF6 = 0xffc6, // Differential progressive DCT
    SOF7 = 0xffc7, // Differential lossless (sequential)

    // Start of Frame, non-differential, arithmetic coding
    SOF9 = 0xffc9, // Extended sequential DCT
    SOF10 = 0xffca, // Progressive DCT
    SOF11 = 0xffcb, // Lossless (sequential)

    // Start of Frame, differential, arithmetic coding
    SOF13 = 0xffcd, // Differential sequential DCT
    SOF14 = 0xffce, // Differential progressive DCT
    SOF15 = 0xffcf, // Differential lossless (sequential)

    // Define Huffman Table(s)
    DHT = 0xffc4,

    // JPEG extensions
    JPG = 0xffc8,

    // Define Arithmetic Coding Conditioning(s)
    DAC = 0xffcc,

    // Restart Interval
    RST0 = 0xffd0,
    RST1 = 0xffd1,
    RST2 = 0xffd2,
    RST3 = 0xffd3,
    RST4 = 0xffd4,
    RST5 = 0xffd5,
    RST6 = 0xffd6,
    RST7 = 0xffd7,

    // other
    SOI = 0xffd8, // Start of Image
    EOI = 0xffd9, // End of Image
    SOS = 0xffda, // Start of Scan
    DQT = 0xffdb, // Define Quantization Table(s)
    DNL = 0xffdc, // Define Number of Lines
    DRI = 0xffdd, // Define Restart Interval
    DHP = 0xffde, // Define Hierarchical Progression
    EXP = 0xffdf, // Expand Reference Component(s)

    // APPN
    APP0 = 0xffe0,
    APP1 = 0xffe1,
    APP2 = 0xffe2,
    APP3 = 0xffe3,
    APP4 = 0xffe4,
    APP5 = 0xffe5,
    APP6 = 0xffe6,
    APP7 = 0xffe7,
    APP8 = 0xffe8,
    APP9 = 0xffe9,
    APP10 = 0xffea,
    APP11 = 0xffeb,
    APP12 = 0xffec,
    APP13 = 0xffed,
    APP14 = 0xffee,
    APP15 = 0xffef,

    // misc
    JPG0 = 0xfff0,
    JPG1 = 0xfff1,
    JPG2 = 0xfff2,
    JPG3 = 0xfff3,
    JPG4 = 0xfff4,
    JPG5 = 0xfff5,
    JPG6 = 0xfff6,
    JPG7 = 0xfff7,
    JPG8 = 0xfff8,
    JPG9 = 0xfff9,
    JPG10 = 0xfffa,
    JPG11 = 0xfffb,
    JPG12 = 0xfffc,
    JPG13 = 0xfffd,
    COM = 0xfffe,
    TEM = 0xff01
};


class IState;

class Jpeg {
    private:
        IState* m_state;
        uint8_t* m_buff_start;
        uint8_t* m_buff_current;
        uint8_t* m_buff_end;

        size_t size_remaining() const noexcept;
        bool seek(size_t rel_pos) noexcept;
        std::optional<uint8_t> peek(size_t rel_pos = 0) const noexcept;
        std::optional<uint8_t> read_uint8() noexcept;
        std::optional<uint16_t> read_uint16() noexcept;

    public:
        Jpeg(uint8_t* buff, size_t size) noexcept;

        StateID parse_header();

        template <StateID ANY>
        friend class State;
};


class IState {
    protected:
        Jpeg* m_jpeg;

    public:
        IState(Jpeg* context) noexcept;
        virtual ~IState();

        IState(const IState& other) = delete;
        IState& operator=(const IState& other) = delete;
        IState(IState&& other) = delete;
        IState& operator=(IState&& other) = delete;

        bool is_final_state() const noexcept;

        virtual IState* parse_header() const = 0;
        virtual StateID getID() const noexcept = 0;
};


template <StateID state_id>
class State final : public IState {
    private:
        StateID m_state_id;

        State(Jpeg* context) noexcept :
            IState(context),
            m_state_id(state_id)
            {}
        ~State() = default;

    public:
        State(const State& other) = delete;
        State& operator=(const State& other) = delete;
        State(State&& other) = delete;
        State& operator=(State&& other) = delete;

        static State* instance(Jpeg* context) noexcept {
            static State<state_id> instance(context);

            return &instance;
        }

        StateID getID() const noexcept override {
            return m_state_id;
        }
        IState* parse_header() const override;
};


template<>
IState* State<StateID::ENTRY>::parse_header() const;

template<>
IState* State<StateID::SOI>::parse_header() const;

template<>
IState* State<StateID::APP0>::parse_header() const;

template<>
IState* State<StateID::DQT>::parse_header() const;

template<>
IState* State<StateID::EOI>::parse_header() const;

template<>
IState* State<StateID::EXIT_OK>::parse_header() const;

template<>
IState* State<StateID::ERROR_PEOB>::parse_header() const;

template<>
IState* State<StateID::ERROR_UUM>::parse_header() const;
