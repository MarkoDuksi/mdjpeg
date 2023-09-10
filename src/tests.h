#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <filesystem>

#include <stdint.h>
#include <sys/types.h>

#include "JpegDecoder.h"


struct Dimensions {
    uint width_px {};
    uint height_px {};

    std::string to_str() const noexcept {
        return std::to_string(width_px) + "x" + std::to_string(height_px);
    }
};

std::vector<std::filesystem::path> getInputImgPaths(const std::filesystem::path& input_base_dir, const Dimensions& dim);

std::tuple<uint8_t*, size_t> read_raw_jpeg_from_file(const std::filesystem::path& file_path);

// decode whole image, implicitly using BasicWriter
bool test1(JpegDecoder& decoder, std::filesystem::path file_path, const Dimensions& dim);

void run_tests(std::filesystem::path input_base_dir, const Dimensions& dim);

bool write_pgm(const std::filesystem::path& file_path, uint8_t* raw_image_data, const Dimensions& dim);

