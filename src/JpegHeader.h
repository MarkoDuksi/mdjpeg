#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <iostream>

#include "JpegReader.h"


template<size_t size>
struct HuffmanTable {
    const uint8_t* histogram {nullptr};
    const uint8_t* symbols {nullptr};
    uint16_t       codes[size] {0};

    bool is_set {false};
};


struct HuffmanTables {
    HuffmanTable<12>  dc;
    HuffmanTable<162> ac;
};


struct JpegHeader {
    uint16_t img_height {0};
    uint16_t img_width {0};
    uint16_t img_mcu_vert_count {0};
    uint16_t img_mcu_horiz_count {0};
    bool     horiz_subsampling {false};

    const uint8_t* qtable {nullptr};
    HuffmanTables  htables[2];

    size_t set_qtable(JpegReader& reader, const size_t max_read_length) noexcept;
    size_t set_htable(JpegReader& reader, size_t max_read_length) noexcept;

    bool is_set() const noexcept;
};
