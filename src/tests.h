#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <filesystem>

#include <cassert>
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

    bool is_8x8_multiple() const noexcept {
        return (width_blk * 8 == width_px && height_blk * 8 == height_px);
    }

    std::string to_str() const noexcept {
        return std::to_string(width_px) + "x" + std::to_string(height_px);
    }
};

std::vector<std::filesystem::path> get_input_img_paths(const std::filesystem::path& input_base_dir, const Dimensions& dims);

std::tuple<uint8_t*, size_t> read_raw_jpeg_from_file(const std::filesystem::path& file_path);

bool elements_eq_to(const uint8_t* const array, const Dimensions& dims, const uint8_t value);

bool write_as_pgm(const std::filesystem::path& file_path, const uint8_t* raw_image_data, const Dimensions& dims);

void print_as_pgm(const uint8_t* const array, const Dimensions& dims);

bool full_frame_decoding_tests(const std::filesystem::path input_base_dir, const Dimensions& dims);

bool cropped_decoding_tests(const std::filesystem::path input_base_dir);

template <uint SRC_WIDTH_PX, uint SRC_HEIGHT_PX, uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
bool downscaling_decoding_tests(std::filesystem::path input_base_dir) {

    Dimensions src_dims {SRC_WIDTH_PX, SRC_HEIGHT_PX};
    assert(src_dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    Dimensions dst_dims {DST_WIDTH_PX, DST_HEIGHT_PX};

    auto input_files_paths = get_input_img_paths(input_base_dir, src_dims);

    bool test_passed{true};

    for (const auto& file_path : input_files_paths) {

        bool subtest_passed{true};

        std::cout << "Running donwscaling decoding subtest on \"" << file_path.c_str() << "\""
                  << " (" << src_dims.to_str() << " -> " << dst_dims.to_str() << ")... ";

        auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder(buff, size);
        DownscalingBlockWriter<DST_WIDTH_PX, DST_HEIGHT_PX> writer;

        uint8_t decoded_img[DST_WIDTH_PX * DST_HEIGHT_PX] {};
        if (decoder.decode(decoded_img, 0, 0, SRC_WIDTH_PX / 8 , SRC_HEIGHT_PX / 8, writer)) {

            std::filesystem::path output_dir = file_path.parent_path() / "downscaled";
            std::filesystem::create_directory(output_dir);
            std::filesystem::path output_file_path = output_dir / (std::string(file_path.stem()) + "_" + std::to_string(DST_WIDTH_PX) + "x" + std::to_string(DST_HEIGHT_PX) + ".pgm");

            if (!write_as_pgm(output_file_path, decoded_img, dst_dims)) {

                subtest_passed = false;
                test_passed = false;
                std::cout << "\n  Writing output file FAILED.\n";
            }
        }
        else {

            subtest_passed = false;
            test_passed = false;
            std::cout << "\n  Decoding+downscaling input file FAILED.\n";
        }

        delete[] buff;

        if (subtest_passed) {

            std::cout << "passed?\n";
        }
    }

    std::cout << "\n";

    return test_passed;
}

template <uint SRC_WIDTH_PX, uint SRC_HEIGHT_PX, uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
bool targeted_downscaling_decoding_test(const uint8_t fill_value) {

    const Dimensions src_dims {SRC_WIDTH_PX, SRC_HEIGHT_PX};
    assert(src_dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    const Dimensions dst_dims {DST_WIDTH_PX, DST_HEIGHT_PX};

    int src_array[64];
    for (uint row = 0; row < 8; ++row) {
        for (uint col = 0; col < 8; ++col) {
            src_array[8 * row + col] = fill_value;
        }
    }

    uint8_t dst_array[DST_WIDTH_PX * DST_HEIGHT_PX] {};

    DownscalingBlockWriter<DST_WIDTH_PX, DST_HEIGHT_PX> writer;
    writer.init_frame(dst_array, SRC_WIDTH_PX, SRC_HEIGHT_PX);

    for (uint i = 0; i < src_dims.width_blk * src_dims.height_blk; ++i) {

        writer.write(src_array);
    }

    std::cout << "Running targeted downscaling decoding test "
              << " (" << src_dims.to_str() << " -> " << dst_dims.to_str()
              << ") with fill value = " << static_cast<uint>(fill_value) << "... ";

    bool test_passed;

    if ((test_passed = elements_eq_to(dst_array, dst_dims, fill_value))) {

        std::cout << "passed.\n";
    }

    else {

        std::cout << "FAILED.\n";

        if (dst_dims.width_blk <= 5 && dst_dims.height_blk <= 5) {

            std::cout << "\n";
            print_as_pgm(dst_array, dst_dims);
        }
    }

    std::cout << "\n";

    return test_passed;
}
