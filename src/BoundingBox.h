#pragma once

#include <stdint.h>
#include <algorithm>


struct BoundingBox {

    public:

        // resist adding more data members in order to maintain efficent random access to
        // elements of BoundingBox arrays (preferred element size of size 2^N)
        uint16_t topleft_X {};
        uint16_t topleft_Y {};
        uint16_t bottomright_X {};
        uint16_t bottomright_Y {};

        BoundingBox() = default;

        BoundingBox (const uint16_t topleft_X, const uint16_t topleft_Y) noexcept :
            topleft_X(topleft_X),
            topleft_Y(topleft_Y),
            bottomright_X(topleft_X + 1),
            bottomright_Y(topleft_Y + 1)
            {}

        BoundingBox (const uint16_t topleft_X, const uint16_t topleft_Y, const uint16_t bottomright_X, const uint16_t bottomright_Y) noexcept :
            topleft_X(topleft_X),
            topleft_Y(topleft_Y),
            bottomright_X(bottomright_X),
            bottomright_Y(bottomright_Y)
            {}

        explicit operator bool() const noexcept {

            return bottomright_X && bottomright_Y;
        }

        bool operator>(const BoundingBox& other) const noexcept {

            return std::min(width(), height()) > std::min(other.width(), other.height());
        }

        uint16_t width() const noexcept {

            return bottomright_X - topleft_X;
        }

        uint16_t height() const noexcept {

            return bottomright_Y - topleft_Y;
        }

        void merge(const BoundingBox& other) noexcept;

        bool expand_to_square(const BoundingBox& outer_bounds) noexcept;
};
