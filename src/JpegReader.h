#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <optional>


/// \brief Provides sequential reading facilites specific to JFIF data.
///
/// \note \e ECS in the following documentation refers to entropy-coded segment.
class JpegReader {

    public:

        /// \brief Sets its view on the JFIF data memory block.
        ///
        /// \param buff  Start of memory block containing JFIF data.
        /// \param size  Size of the memory block in bytes.
        void set(const uint8_t* buff, size_t size) noexcept;

        /// \brief Stores the current cursor position as the start of ECS.
        void mark_start_of_ecs() noexcept;

        /// \brief Restores the cursor position to the start of ECS.
        void restart_ecs() noexcept;

        /// \brief Calculates the size in bytes of the buffer available after the cursor.
        size_t size_remaining() const noexcept {

            return m_buff_end - m_buff_current_byte;
        }

        /// \brief Accessor for the pointer to cursor.
        const uint8_t* tell_ptr() const noexcept {

            return m_buff_current_byte;
        }

        /// \brief Advances the cursor relative to current position. 
        /// \retval  true on success.
        /// \retval  false on failure.
        bool seek(size_t rel_pos) noexcept;

        /// \brief Gets raw value from buffer at a specific position relative to cursor.
        std::optional<uint8_t> peek(size_t rel_pos = 0) const noexcept;

        /// \brief Gets raw value from buffer at cursor, advances the cursor.
        std::optional<uint8_t> read_uint8() noexcept;

        /// \brief Gets 2-byte big-endian value from buffer at cursor, advances the cursor.
        std::optional<uint16_t> read_uint16() noexcept;

        /// \brief Gets JFIF marker value from buffer at cursor, applying rules for reading markers, advances the cursor.
        std::optional<uint16_t> read_marker() noexcept;

        /// \brief Gets JFIF segment size from buffer at cursor, advances the cursor.
        uint16_t read_segment_size() noexcept;

        /// \brief Gets next bit from buffer at cursor, applying rules for reading ECS, advances the cursor.
        /// \return  Bit value on success, ReadError::ECS_BIT otherwise.
        int8_t read_bit() noexcept;
        
    private:

        const uint8_t* m_buff_start {nullptr};
        const uint8_t* m_buff_end {nullptr};
        const uint8_t* m_buff_start_of_ECS {nullptr};
        const uint8_t* m_buff_current_byte {nullptr};
        uint8_t m_current_bit_pos {};
};
