#pragma once

#include <stdint.h>


// marker values from:
//  https://github.com/dannye/jed/blob/master/src/jpg.h
//  https://webdocs.cs.ualberta.ca/~jag/courses/ImProc/lectures2001/lec26/Lec26jpegCompr.PDF

enum class StateID : uint16_t {
    // Custom final states
    HEADER_OK  = 0, // Valid final state
    ERROR_PEOB = 1, // Premature End of Buffer
    ERROR_UUM  = 2, // Unsuported or Unrecognized Marker
    ERROR_UPAR = 3, // Unsuported PARameter
    ERROR_CORR = 4, // CORRupted data

    // Custom transient states
    ENTRY  = 100, // Inital state

    // Temporary, arithmetic coding
    TEM = 0xff01,

    // Reserved from 0xff02 to 0xffbf

    // Start of Frame
    SOF0 = 0xffc0, // Baseline
    SOF1 = 0xffc1, // Extended sequential
    SOF2 = 0xffc2, // Progressive
    SOF3 = 0xffc3, // Lossless

    // Define Huffman Tables
    DHT = 0xffc4,

    // Start of Frame
    SOF5 = 0xffc5, // Sequential, differential
    SOF6 = 0xffc6, // Progressive, differential
    SOF7 = 0xffc7, // Lossless, differential

    // Reserved
    JPG = 0xffc8,

    // Start of Frame
    SOF9  = 0xffc9, // Extended sequential, arithmetic coding
    SOF10 = 0xffca, // Progressive, arithmetic coding
    SOF11 = 0xffcb, // Lossless, arithmetic coding

    // Define Arithmetic Conditions
    DAC = 0xffcc,

    // Start of Frame
    SOF13 = 0xffcd, // Sequential, differential, arithmetic coding
    SOF14 = 0xffce, // Progressive, differential, arithmetic coding
    SOF15 = 0xffcf, // Lossless, differential, arithmetic coding

    // Resets
    RST0 = 0xffd0, // Reset
    RST1 = 0xffd1, // Reset
    RST2 = 0xffd2, // Reset
    RST3 = 0xffd3, // Reset
    RST4 = 0xffd4, // Reset
    RST5 = 0xffd5, // Reset
    RST6 = 0xffd6, // Reset
    RST7 = 0xffd7, // Reset

    // Misc
    SOI = 0xffd8, // Start of Image
    EOI = 0xffd9, // End of Image
    SOS = 0xffda, // Start of Scan
    DQT = 0xffdb, // Define Quantization Tables
    DNL = 0xffdc, // Define Number of Lines
    DRI = 0xffdd, // Define Restart Interval
    DHP = 0xffde, // Define Hierarchical Progression
    EXP = 0xffdf, // Expand Reference Components

    APP0  = 0xffe0, // Aplication Specific Data
    APP1  = 0xffe1, // Aplication Specific Data
    APP2  = 0xffe2, // Aplication Specific Data
    APP3  = 0xffe3, // Aplication Specific Data
    APP4  = 0xffe4, // Aplication Specific Data
    APP5  = 0xffe5, // Aplication Specific Data
    APP6  = 0xffe6, // Aplication Specific Data
    APP7  = 0xffe7, // Aplication Specific Data
    APP8  = 0xffe8, // Aplication Specific Data
    APP9  = 0xffe9, // Aplication Specific Data
    APP10 = 0xffea, // Aplication Specific Data
    APP11 = 0xffeb, // Aplication Specific Data
    APP12 = 0xffec, // Aplication Specific Data
    APP13 = 0xffed, // Aplication Specific Data
    APP14 = 0xffee, // Aplication Specific Data
    APP15 = 0xffef, // Aplication Specific Data

    JPG0  = 0xfff0, // Reserved
    JPG1  = 0xfff1, // Reserved
    JPG2  = 0xfff2, // Reserved
    JPG3  = 0xfff3, // Reserved
    JPG4  = 0xfff4, // Reserved
    JPG5  = 0xfff5, // Reserved
    JPG6  = 0xfff6, // Reserved
    JPG7  = 0xfff7, // Reserved
    JPG8  = 0xfff8, // Reserved
    JPG9  = 0xfff9, // Reserved
    JPG10 = 0xfffa, // Reserved
    JPG11 = 0xfffb, // Reserved
    JPG12 = 0xfffc, // Reserved
    JPG13 = 0xfffd, // Reserved

    COM = 0xfffe // Comment
};


class JpegDecoder;
class JpegReader;


class State {
    public:
        State(JpegDecoder* const decoder, JpegReader* const reader) noexcept;
        virtual ~State();

        bool is_final_state() const noexcept;
        virtual StateID getID() const noexcept = 0;
        virtual void parse_header() = 0;

    protected:
        JpegDecoder* const m_decoder;
        JpegReader*  const m_reader;
};


template <StateID state_id>
class ConcreteState final : public State {
    public:
        ConcreteState(JpegDecoder* const decoder, JpegReader* const reader) noexcept :
            State(decoder, reader)
            {}

        StateID getID() const noexcept override {
            return state_id;
        }
        
        void parse_header() override {};
};
