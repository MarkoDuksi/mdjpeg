#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <filesystem>

#include <stdint.h>
#include <sys/types.h>

#include "JpegDecoder.h"


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

    bool is_8x8_multiple() {
        return (width_blk * 8 == width_px && height_blk * 8 == height_px);
    }

    std::string to_str() const noexcept {
        return std::to_string(width_px) + "x" + std::to_string(height_px);
    }
};

// run tests that decode complete image frames with no downscaling, using BasicBlockWriter
bool run_integral_decoding_tests(std::filesystem::path input_base_dir, const Dimensions& dim);

// run tests that decode cropped 200x200px blocks from 800x800 px images, using BasicBlockWriter
bool run_partial_decoding_tests(std::filesystem::path file_path);

std::vector<std::filesystem::path> get_input_img_paths(const std::filesystem::path& input_base_dir, const Dimensions& dim);

std::tuple<uint8_t*, size_t> read_raw_jpeg_from_file(const std::filesystem::path& file_path);

bool write_pgm(const std::filesystem::path& file_path, uint8_t* raw_image_data, const Dimensions& dim);

