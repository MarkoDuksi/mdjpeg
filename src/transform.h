#pragma once

#include <stdint.h>


namespace transform {

void zig_zag(int (&block)[64]) noexcept;

void idct(int (&block)[64]) noexcept;

// inplace shift all block values by +128 and clip the result to uint8_t range
void range_normalize(int (&block)[64]) noexcept;

}  // namespace transform
