#include "tests.h"

#include <fstream>
#include <iostream>

#include <cassert>
#include <stdint.h>
#include <sys/types.h>


uint full_frame_dc_decoding_tests(const std::filesystem::path input_base_dir, const Dimensions& dims) {

    assert(dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    const auto input_files_paths = get_input_img_paths(input_base_dir / dims.to_str());

    uint tests_failed = 0;

    for (const auto& file_path : input_files_paths) {

        bool subtest_passed{true};

        std::cout << "Running full-frame DC-only decoding test on \"" << file_path.c_str() << "\"";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder(buff, size);
        uint8_t* decoded_img = new uint8_t[dims.width_blk * dims.height_blk];

        if (decoder.dc_luma_decode(decoded_img, 0, 0, dims.width_blk, dims.height_blk)) {

            std::filesystem::path output_dir = file_path.parent_path() / "full-frame_DC-only";
            std::filesystem::create_directory(output_dir);
            std::filesystem::path output_file_path = output_dir / file_path.filename().replace_extension("pgm");

            if (!write_as_pgm(output_file_path, decoded_img, dims.width_blk, dims.height_blk)) {

                subtest_passed = false;
                ++tests_failed;
                std::cout << "  => FAILED writing output.\n";
            }
        }

        else {

            subtest_passed = false;
            ++tests_failed;
            std::cout << "  => FAILED decoding JPEG.\n";
        }

        delete[] buff;
        delete[] decoded_img;

        if (subtest_passed) {

            std::cout << "  => passed?\n";
        }
    }

    return tests_failed;
}

uint full_frame_decoding_tests(const std::filesystem::path input_base_dir, const Dimensions& dims) {

    assert(dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    const auto input_files_paths = get_input_img_paths(input_base_dir / dims.to_str());

    uint tests_failed = 0;

    for (const auto& file_path : input_files_paths) {

        bool subtest_passed{true};

        std::cout << "Running full-frame decoding test on \"" << file_path.c_str() << "\"";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder(buff, size);
        uint8_t* decoded_img = new uint8_t[dims.width_px * dims.height_px];

        if (decoder.luma_decode(decoded_img, 0, 0, dims.width_blk, dims.height_blk)) {

            std::filesystem::path output_dir = file_path.parent_path() / "full-frame";
            std::filesystem::create_directory(output_dir);
            std::filesystem::path output_file_path = output_dir / file_path.filename().replace_extension("pgm");

            if (!write_as_pgm(output_file_path, decoded_img, dims.width_px, dims.height_px)) {

                subtest_passed = false;
                ++tests_failed;
                std::cout << "  => FAILED writing output.\n";
            }
        }

        else {

            subtest_passed = false;
            ++tests_failed;
            std::cout << "  => FAILED decoding JPEG.\n";
        }

        delete[] buff;
        delete[] decoded_img;

        if (subtest_passed) {

            std::cout << "  => passed?\n";
        }
    }

    return tests_failed;
}

uint cropped_decoding_tests(const std::filesystem::path input_base_dir) {

    const uint src_width_px = 800;
    const uint src_height_px = 800;
    const Dimensions src_dims {src_width_px, src_height_px};

    constexpr uint dst_width_px = 200;
    constexpr uint dst_height_px = 200;
    const Dimensions dst_dims {dst_width_px, dst_height_px};

    const auto input_files_paths = get_input_img_paths(input_base_dir / src_dims.to_str());

    uint tests_failed = 0;

    for (const auto& file_path : input_files_paths) {

        bool subtest_passed{true};

        std::cout << "Running cropped decoding test on \"" << file_path.c_str() << "\"";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder(buff, size);
        uint8_t decoded_img[dst_width_px * dst_height_px] {};

        uint quadrant_idx = 0;
        std::string quadrants[16] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};

        for (uint row_blk = 0; row_blk < src_dims.height_blk; row_blk += dst_dims.height_blk) {

            for (uint col_blk = 0; col_blk < src_dims.width_blk; col_blk += dst_dims.width_blk) {

                if (decoder.luma_decode(decoded_img, col_blk, row_blk, col_blk + dst_dims.width_blk, row_blk + dst_dims.height_blk)) {

                    std::filesystem::path output_dir = file_path.parent_path() / "cropped";
                    std::filesystem::create_directory(output_dir);
                    std::filesystem::path output_file_path = output_dir / (std::string(file_path.stem()) + "_" + quadrants[quadrant_idx] + ".pgm");

                    if (!write_as_pgm(output_file_path, decoded_img, dst_dims.width_px, dst_dims.height_px)) {

                        subtest_passed = false;
                        ++tests_failed;
                        std::cout << "\n  => FAILED writing output for quadrant \"" << quadrants[quadrant_idx] << "\".";
                    }
                }

                else {

                    subtest_passed = false;
                    ++tests_failed;
                    std::cout << "\n  => FAILED decoding JPEG for quadrant \"" << quadrants[quadrant_idx] << "\".";
                }

                ++quadrant_idx;
            }
        }

        delete[] buff;

        if (subtest_passed) {

            std::cout << "  => passed?\n";
        }

        else {

            std::cout << "\n";
        }
    }

    return tests_failed;
}
