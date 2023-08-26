#include "ZigZag.h"

#include <sys/types.h>


void ZigZag::transform(int (&block)[64]) const noexcept {
    int temp[64];
    for (uint i = 0; i < 64; ++i) {
        temp[i] = block[i];
    }

    for (uint i = 0; i < 64; ++i) {
        block[m_zig_zag_map[i]] = temp[i];
    }
}
