#include "JpegReader.h"

#include <iostream>


JpegReader::JpegReader(const uint8_t* const buff, const size_t size) noexcept :
    m_buff_start(buff),
    m_buff_end(m_buff_start + size),
    m_buff_current_byte(const_cast<const uint8_t*>(buff))
    {}

void JpegReader::mark_start_of_ecs() noexcept {
    m_buff_start_of_ECS = m_buff_current_byte;
}

void JpegReader::restart_ecs() {
    m_buff_current_byte = m_buff_start_of_ECS;
    m_current_bit_pos = 7;
}

size_t JpegReader::size_remaining() const noexcept {
    return m_buff_end - m_buff_current_byte;
}

const uint8_t* JpegReader::tell_ptr() const noexcept {
    return m_buff_current_byte;
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
    if (rel_pos < size_remaining()) {
        return m_buff_current_byte[rel_pos];
    }
    else {
        return {};
    }
}

std::optional<uint8_t> JpegReader::read_uint8() noexcept {
    if (size_remaining() > 0) {
        return *(m_buff_current_byte++);
    }
    else {
        return {};
    }
}

std::optional<uint16_t> JpegReader::read_uint16() noexcept {
    if (size_remaining() >= 2) {
        const uint result = m_buff_current_byte[0] << 8 | m_buff_current_byte[1];
        m_buff_current_byte += 2;
        return result;
    }
    else {
        return {};
    }
}

std::optional<uint16_t> JpegReader::read_marker() noexcept {
    auto result = read_uint16();

    while (result && *result == 0xffff) {
        const auto next_byte = read_uint8();
        if (next_byte) {
            *result = *result & *next_byte;
        }
        else {
            result = std::nullopt;
        }
    }

    return result;
}

std::optional<uint16_t> JpegReader::read_segment_size() noexcept {
    auto result = read_uint16();

    if (!result || *result < 2) {
        result = std::nullopt;
        }

    return *result - 2;
}

int JpegReader::read_bit() noexcept {
    // current byte is always valid, read current bit if not last (lowest, at position 0)
    if (m_current_bit_pos) {
        return *m_buff_current_byte >> m_current_bit_pos-- & 1;
    }

    // validate next byte before advancing current byte (next most common scenario)
    if (*m_buff_current_byte != 0xff && m_buff_current_byte[1] != 0xff) {
        m_current_bit_pos = 7;

        return *m_buff_current_byte++ & 1;
    }

    // allow reading next 0xff if byte-stuffed with 0x00
    if (m_buff_current_byte[1] == 0xff && m_buff_current_byte[2] == 0x00) {
        m_current_bit_pos = 7;

        return *m_buff_current_byte++ & 1;
    }

    // skip 0x00 after reading 0xff
    if (*m_buff_current_byte == 0xff) {
        m_current_bit_pos = 7;
        const int temp = *m_buff_current_byte & 1;
        m_buff_current_byte += 2;

        return temp;
    }


    // if reading last bit before expected EOI marker
    if (size_remaining() == 3) {
        return *m_buff_current_byte++ & 1;
    }

    // if reading last bit before invalid marker
    const auto marker = read_marker();
    if (marker) {
        std::cout << "unexpected marker in ECS: " << std::hex << *marker << "\n";
    }
    else {
        std::cout << "error reading unexpected marker in ECS\n";
    }

    return ECS_ERROR;
}
