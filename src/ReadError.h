#pragma once


namespace ReadError {

/// \brief Error codes indicating failure when reading the ECS from buffer.
///
/// ECS (entropy-coded segment) is read through JpegReader::read_bit which only
/// reports a single bit read failure to its caller on running out of buffer
/// from which to read. The caller may have needed the requested bit to
/// construct its own return value. It now needs to communicate its own failure
/// using a different error code - one that fits into its return value type and
/// sits outside of its valid return value domain. The caller can also fail due
/// to being "successfully" served a bit from an otherwise corrupted ECS.
/// Therefore the need for individually tailored error codes to be returned by
/// different callers depending on which one was first to detect the error (even
/// if the underlying cause was simply a corrupted ECS).
enum ReadError {
    ECS_BIT = -1,        ///< Error code returned by JpegReader::read_bit
    HUFF_SYMBOL = 0xff,  ///< Error code returned by Huffman::get_symbol
    DCT_COEF = 0x2000    ///< Error code returned by Huffman::get_dct_coeff
};

}  // namespace ReadError
