#pragma once

#include <stdint.h>
#include <algorithm>


/// \brief POD class describing rectangular frame subregion.
struct BoundingBox {

    public:

        // resist adding more data members in order to maintain efficent random access to
        // elements of BoundingBox arrays (preferred element size of size 2^N)
        uint16_t topleft_X {};
        uint16_t topleft_Y {};
        uint16_t bottomright_X {};
        uint16_t bottomright_Y {};

        BoundingBox() = default;

        /// \brief Position-determined (1 x 1 size) bounding box constructor.
        ///
        /// \param topleft_X  X-coordinate of the top-left corner.
        /// \param topleft_Y  Y-coordinate of the top-left corner.
        BoundingBox (const uint16_t topleft_X, const uint16_t topleft_Y) noexcept :
            topleft_X(topleft_X),
            topleft_Y(topleft_Y),
            bottomright_X(topleft_X + 1),
            bottomright_Y(topleft_Y + 1)
            {}

        /// \brief Position- and size-determined bounding box constructor.
        ///
        /// \param topleft_X      X-coordinate of the top-left corner.
        /// \param topleft_Y      Y-coordinate of the top-left corner.
        /// \param bottomright_X  X-coordinate of the bottom-right corner, must be greater than `topleft_X`.
        /// \param bottomright_Y  Y-coordinate of the bottom-right corner, must be greater than `topleft_Y`.
        BoundingBox (const uint16_t topleft_X, const uint16_t topleft_Y, const uint16_t bottomright_X, const uint16_t bottomright_Y) noexcept :
            topleft_X(topleft_X),
            topleft_Y(topleft_Y),
            bottomright_X(bottomright_X),
            bottomright_Y(bottomright_Y)
            {}

        /// \brief Checks if bounding box is not invalidated.
        ///
        /// The bounding box is evaluated as invalid (false) if its bottom-right X-coordinate
        /// is zero. A non-zero value returns true but does *not* guarantee a valid bounding
        /// box unless the program logic makes it so (it cannont be used to detect flaws in
        /// program logic).
        explicit operator bool() const noexcept {

            return bottomright_X;
        }

        /// \brief Compares smaller of dimensions of each bounding box.
        bool operator>(const BoundingBox& other) const noexcept {

            return std::min(width(), height()) > std::min(other.width(), other.height());
        }

        /// \brief Calculates bounding box width.
        uint16_t width() const noexcept {

            return bottomright_X - topleft_X;
        }

        /// \brief Calculates bounding box height.
        uint16_t height() const noexcept {

            return bottomright_Y - topleft_Y;
        }

        /// \brief Expands bounding box to tightly fit around another.
        ///
        /// The box is expanded as little as neccessary for its new definition to tightly
        /// contain both original box and the one it expanded over. If the other box is
        /// already completely contained within the original one, it does nothing.
        ///
        /// \param other  The box to merge with / expand over.
        void merge(const BoundingBox& other) noexcept;

        /// \brief Expands a non-square bounding box into a square-shaped one.
        ///
        /// The smaller of dimensions is increased to match the larger one, provided the
        /// resulting square fits within `outer_bounds`. Otherwise, does nothing. The
        /// center of the expanded bounding box is kept as close as possible to its orginal
        /// center but translated if necessary for the entire bounding box to fit within
        /// `outer_bounds`.
        ///
        /// \param outer_bounds  Bounding box describing the allowed area to expand into.
        /// \retval              true if the squared bounding box fits within `outer_bounds`.
        /// \retval              false otherwise.
        bool expand_to_square(const BoundingBox& outer_bounds) noexcept;
};
