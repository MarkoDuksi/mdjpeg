#include "tests.h"

#include <algorithm>
#include <fstream>
#include <ios>
#include <iostream>

#include <cassert>
#include <stdint.h>
#include <sys/types.h>

#include <fmt/core.h>


bool full_frame_decoding_tests(const std::filesystem::path input_base_dir, const Dimensions& dims) {

    assert(dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    const auto input_files_paths = get_input_img_paths(input_base_dir, dims);

    bool test_passed{true};

    for (const auto& file_path : input_files_paths) {

        bool subtest_passed{true};

        std::cout << "Running full-frame decoding subtest on \"" << file_path.c_str() << "\"  => ";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder(buff, size);
        uint8_t* decoded_img = new uint8_t[dims.width_px * dims.height_px];

        if (decoder.luma_decode(decoded_img, 0, 0, dims.width_blk, dims.height_blk)) {

            std::filesystem::path output_dir = file_path.parent_path() / "full-frame";
            std::filesystem::create_directory(output_dir);
            std::filesystem::path output_file_path = output_dir / file_path.filename().replace_extension("pgm");

            if (!write_as_pgm(output_file_path, decoded_img, dims)) {

                subtest_passed = false;
                test_passed = false;
                std::cout << "\n  Writing output FAILED.\n";
            }
        }

        else {

            subtest_passed = false;
            test_passed = false;
            std::cout << "\n  Decoding JPEG FAILED.\n";
        }

        delete[] buff;
        delete[] decoded_img;

        if (subtest_passed) {

            std::cout << "passed?\n";
        }
    }

    std::cout << "\n";

    return test_passed;
}

bool cropped_decoding_tests(const std::filesystem::path input_base_dir) {

    const uint src_width_px = 800;
    const uint src_height_px = 800;
    const Dimensions src_dims {src_width_px, src_height_px};

    constexpr uint dst_width_px = 200;
    constexpr uint dst_height_px = 200;
    const Dimensions dst_dims {dst_width_px, dst_height_px};

    const auto input_files_paths = get_input_img_paths(input_base_dir, src_dims);

    bool test_passed{true};

    for (const auto& file_path : input_files_paths) {

        bool subtest_passed{true};

        std::cout << "Running cropped decoding subtest on \"" << file_path.c_str() << "\"  => ";

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

                    if (!write_as_pgm(output_file_path, decoded_img, dst_dims)) {

                        subtest_passed = false;
                        test_passed = false;
                        std::cout << "\n  Writing output for quadrant \"" << quadrants[quadrant_idx] << "\" FAILED.\n";
                    }
                }

                else {

                    subtest_passed = false;
                    test_passed = false;
                    std::cout << "\n  Decoding JPEG for quadrant \"" << quadrants[quadrant_idx] << "\" FAILED.\n";
                }

                ++quadrant_idx;
            }
        }

        delete[] buff;

        if (subtest_passed) {

            std::cout << "passed?\n";
        }
    }

    std::cout << "\n";

    return test_passed;
}

std::vector<std::filesystem::path> get_input_img_paths(const std::filesystem::path& input_base_dir, const Dimensions& dims) {
    
    std::string input_dir = input_base_dir / dims.to_str();
    std::vector<std::filesystem::path> input_files_paths;

    for (const auto& dir_entry : std::filesystem::directory_iterator(input_dir)) {

        if (dir_entry.path().extension() == ".jpg")

            input_files_paths.push_back(dir_entry.path());
    }

    std::sort(input_files_paths.begin(), input_files_paths.end());

    return input_files_paths;
}

std::tuple<uint8_t*, size_t> read_raw_jpeg_from_file(const std::filesystem::path& file_path) {

    std::ifstream file(file_path, std::ios::in | std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {

        std::cout << "Error opening input file: " << file_path.c_str() << "\n";

        return {nullptr, 0};
    }

    const auto size = file.tellg();
    char* raw_jpeg = new char[size];

    file.seekg(0);
    file.read(raw_jpeg, size);

    if (file.gcount() != size) {

        std::cout << "Error reading input file: " << file_path.c_str() << "\n";
        file.close();

        return {nullptr, 0};
    }

    file.close();

    return {reinterpret_cast<uint8_t*>(raw_jpeg), size};
}

uint max_abs_error(const uint8_t* const array, const Dimensions& dims, const uint8_t value) {

    int max_absolute_error = 0;

    for (uint row = 0; row < dims.height_px; ++row) {

        for (uint col = 0; col < dims.width_px; ++col) {

            const int error = array[row * dims.width_px + col] - value;
            max_absolute_error = std::max(max_absolute_error, error >= 0 ? error : -error);
        }
    }

    return max_absolute_error;
}

bool write_as_pgm(const std::filesystem::path& file_path, const uint8_t* raw_image_data, const Dimensions& dims) {

    std::ofstream file(file_path);

    if (!file.is_open()) {

        std::cout << "Error opening output file: " << file_path.c_str() << "\n";

        return false;
    }

    file << "P2\n" << dims.width_px << " " << dims.height_px << " " << 255 << "\n";

    for (uint row = 0; row < dims.height_px; ++row) {

        for (uint col = 0; col < dims.width_px; ++col) {

            file << static_cast<uint>(*raw_image_data++) << " ";
        }

        file << "\n";
    }

    file << std::endl;
    file.close();

    return true;
}

void print_as_pgm(const uint8_t* const array, const Dimensions& dims) {

    std::cout << "P2\n" << dims.width_px << " " << dims.height_px << " " << 255 << "\n";

    for (uint row = 0; row < dims.height_px; ++row) {

            if (row % 8 == 0) {

                std::cout << "\n";
            }

        for (uint col = 0; col < dims.width_px; ++col) {

            if (col && col % 8 == 0) {

                std::cout << " ";
            }

            fmt::print("{:0>2} ", array[row * dims.width_px + col]);
        }

        std::cout << "\n";
    }

    std::cout << "\n";
}
