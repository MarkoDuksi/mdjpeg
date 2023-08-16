#pragma once

#include "stdint.h"

#include <iostream>
#include <optional>
#include <cmath>


// marker values from:
// - https://github.com/dannye/jed/blob/master/src/jpg.h
// - https://webdocs.cs.ualberta.ca/~jag/courses/ImProc/lectures2001/lec26/Lec26jpegCompr.PDF

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


template<size_t SIZE>
struct HuffmanTable {
    const uint8_t* histogram {nullptr};
    const uint8_t* symbols {nullptr};
    uint16_t codes[SIZE] {0};
    bool is_set {false};
};


struct HuffmanTables {
    HuffmanTable<12> dc;
    HuffmanTable<162> ac;
};


struct CompressedData {
    private:
        const uint8_t* const m_buff_start {nullptr};
        const uint8_t* const m_buff_end {nullptr};
        const uint8_t* m_buff_start_of_ECS {nullptr};
        const uint8_t* m_buff_current_byte {nullptr};
        uint           m_current_bit_pos {7};

        uint16_t m_img_height {0};
        uint16_t m_img_width {0};
        bool     m_horiz_subsampling {false};

    public:
        CompressedData(const uint8_t* const buff, const size_t size) noexcept;
        
        size_t size_remaining() const noexcept;
        bool seek(const size_t rel_pos) noexcept;

        std::optional<uint8_t> peek(const size_t rel_pos = 0) const noexcept;
        std::optional<uint8_t> read_uint8() noexcept;
        std::optional<uint16_t> read_uint16() noexcept;
        std::optional<uint16_t> read_marker() noexcept;
        std::optional<uint16_t> read_size() noexcept;
        int read_bit() noexcept;

        friend class JpegDecoder;
        
        template <StateID ANY>
        friend class ConcreteState;
};


class State {
    protected:
        JpegDecoder* const m_decoder;
        CompressedData* const m_data;

    public:
        State(JpegDecoder* const decoder, CompressedData* const data) noexcept;
        virtual ~State();

        bool is_final_state() const noexcept;
        virtual StateID getID() const noexcept = 0;
        virtual void parse_header() = 0;
};


template <StateID state_id>
class ConcreteState final : public State {
    public:
        ConcreteState(JpegDecoder* const decoder, CompressedData* const data) noexcept :
            State(decoder, data)
            {}

        StateID getID() const noexcept override {
            return state_id;
        }
        
        void parse_header() override {};
};


class JpegDecoder {
    private:
        CompressedData m_data;
        ConcreteState<StateID::ENTRY> m_state;
        State* m_istate;

        const uint8_t m_zig_zag_map[64] {
             0,  1,  8, 16,  9,  2,  3, 10,
            17, 24, 32, 25, 18, 11,  4,  5,
            12, 19, 26, 33, 40, 48, 41, 34,
            27, 20, 13,  6,  7, 14, 21, 28,
            35, 42, 49, 56, 57, 50, 43, 36,
            29, 22, 15, 23, 30, 37, 44, 51,
            58, 59, 52, 45, 38, 31, 39, 46,
            53, 60, 61, 54, 47, 55, 62, 63
        };

        const uint8_t* m_qtable {nullptr};
        HuffmanTables m_htables[2];

        const float m0 = 2.0 * std::cos(1.0 / 16.0 * 2.0 * M_PI);
        const float m1 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
        const float m3 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
        const float m5 = 2.0 * std::cos(3.0 / 16.0 * 2.0 * M_PI);
        const float m2 = m0 - m5;
        const float m4 = m0 + m5;

        const float s0 = std::cos(0.0 / 16.0 * M_PI) / std::sqrt(8);
        const float s1 = std::cos(1.0 / 16.0 * M_PI) / 2.0;
        const float s2 = std::cos(2.0 / 16.0 * M_PI) / 2.0;
        const float s3 = std::cos(3.0 / 16.0 * M_PI) / 2.0;
        const float s4 = std::cos(4.0 / 16.0 * M_PI) / 2.0;
        const float s5 = std::cos(5.0 / 16.0 * M_PI) / 2.0;
        const float s6 = std::cos(6.0 / 16.0 * M_PI) / 2.0;
        const float s7 = std::cos(7.0 / 16.0 * M_PI) / 2.0;

        StateID parse_header();

        uint8_t get_dc_huff_symbol(const uint table_id) noexcept;
        uint8_t get_ac_huff_symbol(const uint table_id) noexcept;
        int16_t get_dct_coeff(const uint length) noexcept;

        uint set_qtable(const size_t max_read_length) noexcept;
        uint set_htable(size_t max_read_length) noexcept;

        bool huff_decode(int* const dst_block, int& previous_dc_coeff, const uint table_id) noexcept;
        void dequantize(int* const block) noexcept;
        void idct(int* const block) noexcept;
        void level_to_unsigned(int* const block) noexcept;

    public:
        JpegDecoder(const uint8_t* const buff, const size_t size) noexcept;

        bool decode(uint8_t* const dst, uint16_t x = 0, uint16_t y = 0, uint16_t width = 0, uint16_t height = 0) noexcept;

        template <StateID ANY>
        friend class ConcreteState;
};


template<>
void ConcreteState<StateID::ENTRY>::parse_header();

template<>
void ConcreteState<StateID::SOI>::parse_header();

template<>
void ConcreteState<StateID::APP0>::parse_header();

template<>
void ConcreteState<StateID::DQT>::parse_header();

template<>
void ConcreteState<StateID::EOI>::parse_header();
