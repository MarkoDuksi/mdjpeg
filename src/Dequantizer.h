#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "JpegReader.h"


/// \brief Class for dequantizing DCT coefficients.
///
/// Uses quantization table data directly from JFIF header.
class Dequantizer {

    public:

        size_t set_qtable(JpegReader& reader, const size_t max_read_length) noexcept;
        bool is_set() const noexcept;

        void transform(int (&block)[64]) const noexcept;
        void transform(int& dc_coeff) const noexcept;

    private:

        const uint8_t* m_qtable {nullptr};
};
