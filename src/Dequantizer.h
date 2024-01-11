#pragma once

#include <stdint.h>
#include <sys/types.h>


class JpegReader;


/// \brief Class for dequantizing JFIF DCT coefficients.
///
/// Dequantizes (actually, denormalizes since quantization step is lossy and
/// irreversible) quantized DCT coefficients.
class Dequantizer {

    public:

        /// \brief Populates quantization table from \c reader buffer starting at cursor.
        ///
        /// \return  Number of bytes read through from the JFIF segment
        uint8_t set_qtable(JpegReader& reader, uint16_t max_read_length) noexcept;

        /// \brief Checks if the quantization table is validly set.
        bool is_set() const noexcept {

            return m_qtable;
        }

        /// \brief Invalidates quantization table.
        void clear() noexcept {

            m_qtable = nullptr;
        }

        /// \brief Dequantizes a block of DCT coefficients (in place).
        void transform(int (&block)[64]) const noexcept;

        /// \brief Dequantizes a single (DC) DCT coefficient (in place).
        void transform(int& dc_coeff) const noexcept;

    private:

        const uint8_t* m_qtable {nullptr};
};
