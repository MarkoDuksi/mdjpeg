#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <vector>
#include <utility>
#include <filesystem>


namespace mdjpeg_test_utils {

struct Dimensions {
    Dimensions(uint16_t width_px, uint16_t height_px) :
        width_px(width_px),
        height_px(height_px),
        width_blk(width_px / 8),
        height_blk(height_px / 8)
        {}

    uint16_t width_px {};
    uint16_t height_px {};
    uint16_t width_blk {};
    uint16_t height_blk {};

    bool is_8x8_multiple() const noexcept {
        return (width_blk * 8 == width_px && height_blk * 8 == height_px);
    }

    std::string to_str() const noexcept {
        return std::to_string(width_px) + "x" + std::to_string(height_px);
    }
};

std::vector<std::filesystem::path> get_input_img_paths(const std::filesystem::path& input_dir);

std::pair<uint8_t*, size_t> read_raw_jpeg_from_file(const std::filesystem::path& file_path);

uint max_abs_error(const uint8_t* array, const Dimensions& dims, uint8_t value);

bool write_as_pgm(const std::filesystem::path& file_path, const uint8_t* pixel_data, uint16_t width_px, uint16_t height_px);

void print_as_pgm(const uint8_t* pixel_data, uint16_t width_px, uint16_t height_px);

}  // namespace mdjpeg_test_utils
