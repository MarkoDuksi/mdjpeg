#pragma once

#include "JpegReader.h"


class Dequantizer {
    public:
        size_t set_qtable(JpegReader& reader, const size_t max_read_length) noexcept;
        bool is_set() const noexcept;

        void transform(int (&block)[64]) noexcept;
        void transform(int& dc_coeff) noexcept;

    private:
        const uint8_t* m_qtable {nullptr};
};