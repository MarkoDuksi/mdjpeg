#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "JpegReader.h"


/// \brief Class for dequantizing JFIF DCT coefficients.
///
/// Dequantizes (actually, denormalizes since quantization step is lossy and
/// irreversible) quantized DCT coefficients.
class Dequantizer {

    public:

        /// \brief Populates quantization table from \c reader buffer starting at cursor.
        size_t set_qtable(JpegReader& reader, const size_t max_read_length) noexcept;

        /// \brief Checks if the quantization table is populated.
        bool is_set() const noexcept;

        /// \brief Dequantizes a block of DCT coefficients (in place).
        void transform(int (&block)[64]) const noexcept;

        /// \brief Dequantizes a single (DC) DCT coefficient (in place).
        void transform(int& dc_coeff) const noexcept;

    private:

        const uint8_t* m_qtable {nullptr};
};
