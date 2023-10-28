#pragma once

#include <vector>
#include <utility>
#include <filesystem>

#include <stdint.h>
#include <sys/types.h>


struct Dimensions {
    Dimensions(uint width_px, uint height_px) :
        width_px(width_px),
        height_px(height_px),
        width_blk(width_px / 8),
        height_blk(height_px / 8)
        {}

    uint width_px {};
    uint height_px {};
    uint width_blk {};
    uint height_blk {};

    bool is_8x8_multiple() const noexcept {
        return (width_blk * 8 == width_px && height_blk * 8 == height_px);
    }

    std::string to_str() const noexcept {
        return std::to_string(width_px) + "x" + std::to_string(height_px);
    }
};

std::vector<std::filesystem::path> get_input_img_paths(const std::filesystem::path& input_base_dir, const Dimensions& dims);

std::pair<uint8_t*, size_t> read_raw_jpeg_from_file(const std::filesystem::path& file_path);

uint max_abs_error(const uint8_t* const array, const Dimensions& dims, const uint8_t value);

bool write_as_pgm(const std::filesystem::path& file_path, const uint8_t* raw_image_data, const uint width_px, const uint height_px);

void print_as_pgm(const uint8_t* const array, const uint width_px, const uint height_px);
