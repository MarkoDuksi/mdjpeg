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

std::vector<std::filesystem::path> get_input_img_paths(const std::filesystem::path& input_base_dir, const Dimensions& dim);

std::tuple<uint8_t*, size_t> read_raw_jpeg_from_file(const std::filesystem::path& file_path);

bool write_pgm(const std::filesystem::path& file_path, uint8_t* raw_image_data, const Dimensions& dim);

// run tests that decode full image frames w/o downscaling
bool run_integral_decoding_tests(std::filesystem::path input_base_dir, const Dimensions& dim);

// run tests that decode cropped 200x200px blocks from 800x800 px image frames
bool run_partial_decoding_tests(std::filesystem::path input_base_dir);

// run tests that decode full 800x800 px image frames w/ arbitrary downscaling
template <uint SRC_WIDTH_PX, uint SRC_HEIGHT_PX, uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
bool run_downscaling_decoding_tests(std::filesystem::path input_base_dir) {

    bool test_passed{true};

    Dimensions src_dims {SRC_WIDTH_PX, SRC_HEIGHT_PX};
    Dimensions dst_dims {DST_WIDTH_PX, DST_HEIGHT_PX};

    auto input_files_paths = get_input_img_paths(input_base_dir, src_dims);

    for (const auto& file_path : input_files_paths) {

        std::cout << "Running donwscaling decoding test on \"" << file_path.c_str() << "\""
                  << " (" << SRC_WIDTH_PX << "x" << SRC_HEIGHT_PX << " -> " << DST_WIDTH_PX << "x" << DST_HEIGHT_PX << ")\n";

        auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder(buff, size);
        DownscalingBlockWriter<DST_WIDTH_PX, DST_HEIGHT_PX> writer;

        uint8_t decoded_img[DST_WIDTH_PX * DST_HEIGHT_PX] {};
        if (decoder.decode(decoded_img, 0, 0, SRC_WIDTH_PX / 8 , SRC_HEIGHT_PX / 8, writer)) {

            std::filesystem::path output_dir = file_path.parent_path() / "downscaled";
            std::filesystem::create_directory(output_dir);
            std::filesystem::path output_file_path = output_dir / (std::string(file_path.stem()) + "_" + std::to_string(DST_WIDTH_PX) + "x" + std::to_string(DST_HEIGHT_PX) + ".pgm");

            if (!write_pgm(output_file_path, decoded_img, dst_dims)) {

                test_passed = false;
                std::cout << "\n  Writing output file FAILED.\n";
            }
        }
        else {

            test_passed = false;
            std::cout << "\n  Decoding+downscaling input file FAILED.\n";
        }

        delete[] buff;

    }

    std::cout << "\n";

    return test_passed;
}

void run_targeted_test1(const std::filesystem::path& input_base_dir);

void run_targeted_test2();

void run_targeted_test3();