#include "tests.h"

#include <fstream>


uint full_frame_dc_decoding_tests(const mdjpeg::test_utils::Dimensions& src_dims,
                                  const std::filesystem::path& test_imgs_dir,
                                  const std::filesystem::path& output_subdir) {

    assert(src_dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    using namespace mdjpeg::test_utils;

    const auto input_files_dir = test_imgs_dir / src_dims.to_str();
    const auto input_files_paths = get_input_img_paths(input_files_dir);
    const auto output_dir = input_files_dir / output_subdir;

    uint tests_failed = 0;

    for (const auto& file_path : input_files_paths) {

        std::cout << "Full-frame DC-only decoding test on \"" << file_path.filename().c_str() << "\"";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        mdjpeg::JpegDecoder decoder;
        decoder.assign(buff, size);
        std::unique_ptr<uint8_t[]> decoded_img = std::make_unique<uint8_t[]>(src_dims.width_blk * src_dims.height_blk);

        if (decoder.dc_luma_decode(decoded_img.get(), {0, 0, src_dims.width_blk, src_dims.height_blk})) {

            std::filesystem::create_directory(output_dir);
            const std::filesystem::path filename = file_path.filename().replace_extension("pgm");

            if (!write_as_pgm(output_dir / filename, decoded_img.get(), src_dims.width_blk, src_dims.height_blk)) {

                ++tests_failed;
                std::cout << ": FAILED writing output\n";
            }

            else {

                std::cout << ": PASSED (tentative)\n";
            }
        }

        else {

            ++tests_failed;
            std::cout << ": FAILED decoding JPEG\n";
        }

        delete[] buff;
    }

    return tests_failed;
}

uint full_frame_decoding_tests(const mdjpeg::test_utils::Dimensions& src_dims,
                               const std::filesystem::path& test_imgs_dir,
                               const std::filesystem::path& output_subdir) {

    assert(src_dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    using namespace mdjpeg::test_utils;

    const auto input_files_dir = test_imgs_dir / src_dims.to_str();
    const auto input_files_paths = get_input_img_paths(input_files_dir);
    const auto output_dir = input_files_dir / output_subdir;

    uint tests_failed = 0;

    for (const auto& file_path : input_files_paths) {

        std::cout << "Full-frame decoding test on \"" << file_path.filename().c_str() << "\"";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        mdjpeg::JpegDecoder decoder;
        decoder.assign(buff, size);
        std::unique_ptr<uint8_t[]> decoded_img = std::make_unique<uint8_t[]>(src_dims.width_px * src_dims.height_px);

        if (decoder.luma_decode(decoded_img.get(), {0, 0, src_dims.width_blk, src_dims.height_blk})) {

            std::filesystem::create_directory(output_dir);
            const std::filesystem::path filename = file_path.filename().replace_extension("pgm");

            if (!write_as_pgm(output_dir / filename, decoded_img.get(), src_dims.width_px, src_dims.height_px)) {

                ++tests_failed;
                std::cout << ": FAILED writing output\n";
            }

            else {

                std::cout << ": PASSED (tentative)\n";
            }
        }

        else {

            ++tests_failed;
            std::cout << ": FAILED decoding JPEG\n";
        }

        delete[] buff;
    }

    return tests_failed;
}

uint cropped_decoding_tests(const mdjpeg::test_utils::Dimensions& src_dims,
                            const std::filesystem::path& test_imgs_dir,
                            const std::filesystem::path& output_subdir) {

    using namespace mdjpeg::test_utils;

    const Dimensions dst_dims {static_cast<uint16_t>(src_dims.width_px / 4),
                               static_cast<uint16_t>(src_dims.height_px / 4)};

    // since the region of interest is 1/4 the width and height of the full
    // frame image, in order for its dimensions to still be multiples of 8, the
    // full frame width and height must be multiples of 4 * 8 = 32
    assert(dst_dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 32)");

    const auto input_files_dir = test_imgs_dir / src_dims.to_str();
    const auto input_files_paths = get_input_img_paths(input_files_dir);
    const auto output_dir = input_files_dir / output_subdir;

    uint tests_failed = 0;

    for (const auto& file_path : input_files_paths) {

        bool subtest_passed {true};

        std::cout << "Cropped decoding test on \"" << file_path.filename().c_str() << "\"";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        mdjpeg::JpegDecoder decoder;
        decoder.assign(buff, size);
        std::unique_ptr<uint8_t[]> decoded_img = std::make_unique<uint8_t[]>(dst_dims.width_px * dst_dims.height_px);

        uint quadrant_idx = 0;
        const std::string quadrants[4 * 4] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};

        for (uint16_t row_blk = 0; row_blk < src_dims.height_blk; row_blk += dst_dims.height_blk) {

            for (uint16_t col_blk = 0; col_blk < src_dims.width_blk; col_blk += dst_dims.width_blk) {

                if (decoder.luma_decode(decoded_img.get(), {col_blk,
                                                            row_blk,
                                                            static_cast<uint16_t>(col_blk + dst_dims.width_blk),
                                                            static_cast<uint16_t>(row_blk + dst_dims.height_blk)})) {

                    std::filesystem::create_directory(output_dir);
                    const std::filesystem::path filename = std::string(file_path.stem()) + "_" + quadrants[quadrant_idx] + ".pgm";

                    if (!write_as_pgm(output_dir / filename, decoded_img.get(), dst_dims.width_px, dst_dims.height_px)) {

                        subtest_passed = false;
                        ++tests_failed;
                        std::cout << "\n: FAILED writing output for quadrant \"" << quadrants[quadrant_idx] << "\"";
                    }
                }

                else {

                    subtest_passed = false;
                    ++tests_failed;
                    std::cout << "\n: FAILED decoding JPEG for quadrant \"" << quadrants[quadrant_idx] << "\"";
                }

                ++quadrant_idx;
            }
        }

        delete[] buff;

        if (subtest_passed) {

            std::cout << ": PASSED (tentative)\n";
        }

        else {

            std::cout << "\n";
        }
    }

    return tests_failed;
}
