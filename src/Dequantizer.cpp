#include "Dequantizer.h"

#include "JpegReader.h"


uint8_t Dequantizer::set_qtable(JpegReader& reader, const uint16_t max_read_length) noexcept {

    const uint8_t next_byte = *reader.read_uint8();

    const uint8_t precision = 1 + bool(next_byte >> 4);  // in bytes, not bits
    const uint8_t table_id  = next_byte & 0xf;
    const uint8_t table_size = 64 * precision;

    if (1 + table_size > max_read_length) {

        return 0;
    }

    // store only 8-bit luma quantization table pointer
    if (table_id == 0) {

        if (precision == 1) {

            m_qtable = reader.tell_ptr();
        }

        else {

            return 0;
        }
    }

    reader.seek(table_size);

    return 1 + table_size;
}

void Dequantizer::transform(int (&block)[64]) const noexcept {

    for (uint i = 0; i < 64; ++i) {

        block[i] *= m_qtable[i];
    }
}

void Dequantizer::transform(int& dc_coeff) const noexcept {

    dc_coeff *= m_qtable[0];
}
