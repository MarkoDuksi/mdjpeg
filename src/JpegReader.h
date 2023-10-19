#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <optional>


/// \brief Provides sequential reading facilites specific to JFIF data.
///
/// \note \e ECS in the following documentation refers to entropy-coded segment.
class JpegReader {

    public:

        /// \brief Constructor.
        ///
        /// \param buff  Start of memory segment containing JFIF data.
        /// \param size  Length of the memory segment in bytes.
        JpegReader(const uint8_t* const buff, const size_t size) noexcept;
        
        /// \brief Stores the cursor position as the start of ECS.
        void mark_start_of_ecs() noexcept;

        /// \brief Sets the cursor position to the start of ECS.
        void restart_ecs() noexcept;

        /// \brief Get the length of buffer available after the cursor.
        size_t size_remaining() const noexcept;

        /// \brief Accessor for the pointer to cursor.
        const uint8_t* tell_ptr() const noexcept;

        /// \brief Advance the cursor relative to current position. 
        bool seek(const size_t rel_pos) noexcept;

        /// \brief Get raw value from buffer at a specific position relative to cursor.
        std::optional<uint8_t> peek(const size_t rel_pos = 0) const noexcept;

        /// \brief Get raw value from buffer at cursor, advance cursor.
        std::optional<uint8_t> read_uint8() noexcept;

        /// \brief Get 2-byte big-endian value from buffer at cursor, advance cursor.
        std::optional<uint16_t> read_uint16() noexcept;

        /// \brief Get JFIF marker value from buffer at cursor, applying rules for reading markers, advance cursor.
        std::optional<uint16_t> read_marker() noexcept;

        /// \brief Get segment size from buffer at cursor, advance cursor.
        std::optional<uint16_t> read_segment_size() noexcept;

        /// \brief Get next bit from buffer at cursor, applying rules for reading ECS, advance cursor.
        int read_bit() noexcept;
        
    private:

        const uint8_t* const m_buff_start {nullptr};
        const uint8_t* const m_buff_end {nullptr};
        const uint8_t* m_buff_start_of_ECS {nullptr};
        const uint8_t* m_buff_current_byte {nullptr};
        uint m_current_bit_pos {7};  // zero-index of the current bit (highest by default) of the byte at cursor
};
