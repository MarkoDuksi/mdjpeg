#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <optional>

#include "states.h"


#define ECS_ERROR -1
#define SYMBOL_ERROR 0xff
#define COEF_ERROR 0x2000


/// \brief Class providing reading facilites specific to JFIF data.
class JpegReader {

    public:

        JpegReader(const uint8_t* const buff, const size_t size) noexcept;
        
        void mark_start_of_ecs() noexcept;
        void restart_ecs() noexcept;
        size_t size_remaining() const noexcept;

        const uint8_t* tell_ptr() const noexcept;
        bool seek(const size_t rel_pos) noexcept;

        std::optional<uint8_t> peek(const size_t rel_pos = 0) const noexcept;
        std::optional<uint8_t> read_uint8() noexcept;
        std::optional<uint16_t> read_uint16() noexcept;
        std::optional<uint16_t> read_marker() noexcept;
        std::optional<uint16_t> read_segment_size() noexcept;
        int read_bit() noexcept;
        
    private:

        const uint8_t* const m_buff_start {nullptr};
        const uint8_t* const m_buff_end {nullptr};
        const uint8_t* m_buff_start_of_ECS {nullptr};
        const uint8_t* m_buff_current_byte {nullptr};
        uint m_current_bit_pos {7};  // zero-index of the highest bit in a byte
};
