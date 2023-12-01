#include "JpegReader.h"

#include "ReadError.h"


void JpegReader::set(const uint8_t* const buff, const size_t size) noexcept {

    m_buff_start = buff;
    m_buff_end = m_buff_start + size;
    m_buff_current_byte = m_buff_start;
}

void JpegReader::mark_start_of_ecs() noexcept {

    m_buff_start_of_ECS = m_buff_current_byte;
    m_current_bit_pos = 7;  // zero-index of the current bit (7 => highest by default) of the byte at cursor
}

void JpegReader::restart_ecs() noexcept {

    m_buff_current_byte = m_buff_start_of_ECS;
    m_current_bit_pos = 7;  // zero-index of the current bit (7 => highest by default) of the byte at cursor
}

bool JpegReader::seek(const size_t rel_pos) noexcept {

    if (rel_pos < size_remaining()) {

        m_buff_current_byte += rel_pos;

        return true;
    }

    else {

        return false;
    }
}

std::optional<uint8_t> JpegReader::peek(const size_t rel_pos) const noexcept {

    std::optional<uint8_t> result {};

    if (rel_pos < size_remaining()) {

        result.emplace(m_buff_current_byte[rel_pos]);
    }

    return result;
}

std::optional<uint8_t> JpegReader::read_uint8() noexcept {

    std::optional<uint8_t> result {};

    if (size_remaining() > 0) {

        result.emplace(*(m_buff_current_byte++));
    }

    return result;
}

std::optional<uint16_t> JpegReader::read_uint16() noexcept {

    std::optional<uint16_t> result {};

    if (size_remaining() >= 2) {

        result.emplace(m_buff_current_byte[0] << 8 | m_buff_current_byte[1]);
        m_buff_current_byte += 2;
    }

    return result;
}

std::optional<uint16_t> JpegReader::read_marker() noexcept {

    auto result = read_uint16();

    while (result && *result == 0xffff) {

        if (const auto next_byte = read_uint8()) {

            result.emplace(*result & *next_byte);
        }

        else {

            result = std::nullopt;
        }
    }

    return result;
}

uint16_t JpegReader::read_segment_size() noexcept {

    auto result = read_uint16();

    if (result && *result >= 2) {

        return *result - 2;
    }

    return 0;
}

int8_t JpegReader::read_bit() noexcept {

    //////////////////////////////////////////////////////////
    // looking at a byte that has previously been read from //
    
    // read bit 0 (advance current byte)
    // mostly false (true on every 8-th evaluation)
    if (m_current_bit_pos == 0) {

        m_current_bit_pos = 7;

        return *m_buff_current_byte++ & 1;
    }

    // read bit 1, 2, 3, 4, 5 or 6
    // mostly true (false on every 7-th evaluation)
    if (m_current_bit_pos != 7) {

        return *m_buff_current_byte >> m_current_bit_pos-- & 1;
    }

    //////////////////////////////////////////////////////////
    // otherwise, looking at a new byte (validation needed) //

    // skip current (0x00) byte if previous one is 0xff
    // (if looking at the very first byte of ECS, previous byte is always 0x00)
    m_buff_current_byte += *(m_buff_current_byte - 1) == 0xff;

    // any valid byte must be followed by at least two more bytes
    // almost always false (true if ECS is corrupted)
    if (size_remaining() < 2) {

        return static_cast<int8_t>(ReadError::ECS_BIT);
    }

    // read bit 7 if current byte is not 0xff or is followed by 0x00
    // almost always true (false if looking at a marker)
    if ((m_buff_current_byte[0] << 8 | m_buff_current_byte[1]) <= 0xff00) {

        return *m_buff_current_byte >> m_current_bit_pos-- & 1;
    }

    // otherwise looking at EOI or some invalid marker
    // almost never reached (unless ECS is corrupted)
    return static_cast<int8_t>(ReadError::ECS_BIT);
}
