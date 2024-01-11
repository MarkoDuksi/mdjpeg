#pragma once

#include <stdint.h>


/// \brief Stateless block-level transformation functions that don't need a class.
namespace mdjpeg_transform {

/// \brief Reverses zig-zag reordering of a block of values (in place).
void reverse_zig_zag(int (&block)[64]) noexcept;

/// \brief Computes %IDCT on a block of values (in place).
void idct(int (&block)[64]) noexcept;

/// \brief Increments block values by \c +128 and clips results to \c uint8_t range (in place).
void range_normalize(int (&block)[64]) noexcept;

}  // namespace mdjpeg_transform
