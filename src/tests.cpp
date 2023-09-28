#include <vector>
#include <filesystem>
#include <algorithm>
#include <tuple>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

#include <stdint.h>
#include <sys/types.h>

#include <fmt/core.h>

#include "JpegDecoder.h"
#include "BlockWriter.h"
#include "tests.h"


// run tests that decode full image frames w/o downscaling
bool run_integral_decoding_tests(std::filesystem::path input_base_dir, const Dimensions& dim) {

    bool test_passed{true};

    const auto input_files_paths = get_input_img_paths(input_base_dir, dim);

    for (const auto& file_path : input_files_paths) {

        std::cout << "Running integral decoding test on \"" << file_path.c_str() << "\"... ";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder(buff, size);
        uint8_t* decoded_img = new uint8_t[dim.width_px * dim.height_px];

        if (decoder.decode(decoded_img, 0, 0, dim.width_blk, dim.height_blk)) {
            std::filesystem::path output_dir = file_path.parent_path() / "integral";
            std::filesystem::create_directory(output_dir);
            std::filesystem::path output_file_path = output_dir / file_path.filename().replace_extension("pgm");

            if (!write_pgm(output_file_path, decoded_img, dim)) {

                test_passed = false;
                std::cout << "\n  Writing output FAILED.\n";
            }
        }
        else {

            test_passed = false;
            std::cout << "\n  Decoding JPEG FAILED.\n";
        }

        delete[] buff;
        delete[] decoded_img;

        if (test_passed) {
            std::cout << "passed.\n";
        }
    }

    std::cout << "\n";

    return test_passed;
}

// run tests that decode cropped 200x200px blocks from 800x800 px image frames
bool run_partial_decoding_tests(std::filesystem::path input_base_dir) {

    bool test_passed{true};

    const Dimensions src_dims {800, 800};
    const Dimensions cropped_dims {200, 200};

    constexpr uint src_width_px = 800;
    constexpr uint src_height_px = 800;
    constexpr uint dst_width_px = 200;
    constexpr uint dst_height_px = 200;

    const auto input_files_paths = get_input_img_paths(input_base_dir, src_dims);

    for (const auto& file_path : input_files_paths) {

        std::cout << "Running partial decoding test on \"" << file_path.c_str() << "\"... ";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder(buff, size);
        uint8_t decoded_img[dst_width_px * dst_height_px] {};

        uint quadrant_idx = 0;
        std::string quadrants[16] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};

        for (uint row = 0; row < src_height_px; row += dst_height_px) {

            for (uint col = 0; col < src_width_px; col += dst_width_px) {

                const uint row_blk = row / 8;
                const uint col_blk = col / 8;

                if (decoder.decode(decoded_img, col_blk, row_blk, col_blk + cropped_dims.width_blk, row_blk + cropped_dims.height_blk)) {

                    std::filesystem::path output_dir = file_path.parent_path() / "partial";
                    std::filesystem::create_directory(output_dir);
                    std::filesystem::path output_file_path = output_dir / (std::string(file_path.stem()) + "_" + quadrants[quadrant_idx] + ".pgm");

                    if (!write_pgm(output_file_path, decoded_img, cropped_dims)) {

                        test_passed = false;
                        std::cout << "\n  Writing output for quadrant \"" << quadrants[quadrant_idx] << "\" FAILED.\n";
                    }
                }
                else {

                    test_passed = false;
                    std::cout << "\n  Decoding JPEG for quadrant \"" << quadrants[quadrant_idx] << "\" FAILED.\n";
                }
                ++quadrant_idx;
            }
        }

        delete[] buff;

        if (test_passed) {
            std::cout << "passed.\n";
        }
    }

    std::cout << "\n";

    return test_passed;
}

std::vector<std::filesystem::path> get_input_img_paths(const std::filesystem::path& input_base_dir, const Dimensions& dim) {
    
    std::string input_dir = input_base_dir / dim.to_str();
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

bool write_pgm(const std::filesystem::path& file_path, uint8_t* raw_image_data, const Dimensions& dim) {

    std::ofstream file(file_path);

    if (!file.is_open()) {

        std::cout << "Error opening output file: " << file_path.c_str() << "\n";

        return false;
    }

    file << "P2\n" << dim.width_px << " " << dim.height_px << " " << 255 << "\n";

    for (uint row = 0; row < dim.height_px; ++row) {
        for (uint col = 0; col < dim.width_px; ++col) {
            file << static_cast<uint>(*raw_image_data++) << " ";
        }
        file << "\n";
    }

    file << std::endl;
    file.close();

    return true;
}

void run_targeted_test1(const std::filesystem::path& input_base_dir) {
    const Dimensions src_dims {1600, 1200};
    constexpr uint dst_width_px = 192;
    constexpr uint dst_height_px = 64;
    const Dimensions cropped_dims {dst_width_px, dst_height_px};

    const auto file_path = input_base_dir / "1600x1200" / "ESP32-CAM_res13_qual3.jpg";

    const auto [buff, size] = read_raw_jpeg_from_file(file_path);
    JpegDecoder decoder(buff, size);
    uint8_t decoded_img[dst_width_px * dst_height_px] {};

    if (decoder.decode(decoded_img, (1600 - dst_width_px) / 8, 2, src_dims.width_blk, cropped_dims.height_blk)) {

        std::filesystem::path output_dir = file_path.parent_path() / "targeted1";
        std::filesystem::create_directory(output_dir);
        std::filesystem::path output_file_path = output_dir / (std::string(file_path.stem()) + "_" + std::to_string(dst_width_px) + "x" + std::to_string(dst_height_px) + ".pgm");

        if (!write_pgm(output_file_path, decoded_img, cropped_dims)) {

            std::cout << "\n  Writing output FAILED.\n";
        }
    }
    else {

        std::cout << "\n  Decoding JPEG FAILED.\n";
    }

    delete[] buff;
}

void run_targeted_test2() {
    
    constexpr uint src_width_px = 32;
    constexpr uint src_height_px = 8;

    constexpr uint dst_width_px = 31;
    constexpr uint dst_height_px = 7;
    const Dimensions scaled_dims {dst_width_px, dst_height_px};


    int src_array1[64];
    for (uint row = 0; row < 8; ++row) {
        for (uint col = 0; col < 8; ++col) {
            src_array1[8 * row + col] = 100;
        }
    }

    int src_array2[64];
    for (uint row = 0; row < 8; ++row) {
        for (uint col = 0; col < 8; ++col) {
            src_array2[8 * row + col] = 200;
        }
    }

    uint8_t dst_array[dst_width_px * dst_height_px] {};

    DownscalingBlockWriter<dst_width_px, dst_height_px> writer;
    writer.init_frame(dst_array, src_width_px, src_height_px);

    writer.write(src_array1);
    writer.write(src_array2);
    writer.write(src_array1);
    writer.write(src_array2);

    std::cout << "P2\n" << scaled_dims.width_px << " " << scaled_dims.height_px << " " << 255 << "\n";
    for (uint row = 0; row < scaled_dims.height_px; ++row) {
            if (row % 8 == 0) {
                std::cout << "\n";
            }
        for (uint col = 0; col < scaled_dims.width_px; ++col) {
            if (col && col % 8 == 0) {
                std::cout << " ";
            }
            fmt::print("{:0>3} ", dst_array[scaled_dims.width_px * row + col]);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void run_targeted_test3() {
    
    constexpr uint src_width_px = 16;
    constexpr uint src_height_px = 16;

    constexpr uint dst_width_px = 7;
    constexpr uint dst_height_px = 7;
    const Dimensions scaled_dims {dst_width_px, dst_height_px};


    int src_array1[64];
    for (uint row = 0; row < 8; ++row) {
        for (uint col = 0; col < 8; ++col) {
            src_array1[8 * row + col] = 10;
        }
    }

    uint8_t dst_array[dst_width_px * dst_height_px] {};

    DownscalingBlockWriter<dst_width_px, dst_height_px> writer;
    writer.init_frame(dst_array, src_width_px, src_height_px);

    writer.write(src_array1);
    writer.write(src_array1);
    writer.write(src_array1);
    writer.write(src_array1);

    std::cout << "P2\n" << scaled_dims.width_px << " " << scaled_dims.height_px << " " << 255 << "\n";
    for (uint row = 0; row < scaled_dims.height_px; ++row) {
            if (row % 8 == 0) {
                std::cout << "\n";
            }
        for (uint col = 0; col < scaled_dims.width_px; ++col) {
            if (col && col % 8 == 0) {
                std::cout << " ";
            }
            fmt::print("{:0>2} ", dst_array[scaled_dims.width_px * row + col]);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void run_targeted_test4() {
    
    constexpr uint src_width_px = 16;
    constexpr uint src_height_px = 16;

    constexpr uint dst_width_px = 9;
    constexpr uint dst_height_px = 9;
    const Dimensions scaled_dims {dst_width_px, dst_height_px};


    int src_array1[64];
    for (uint row = 0; row < 8; ++row) {
        for (uint col = 0; col < 8; ++col) {
            src_array1[8 * row + col] = 10;
        }
    }

    uint8_t dst_array[dst_width_px * dst_height_px] {};

    DownscalingBlockWriter<dst_width_px, dst_height_px> writer;
    writer.init_frame(dst_array, src_width_px, src_height_px);

    writer.write(src_array1);
    writer.write(src_array1);
    writer.write(src_array1);
    writer.write(src_array1);

    std::cout << "P2\n" << scaled_dims.width_px << " " << scaled_dims.height_px << " " << 255 << "\n";
    for (uint row = 0; row < scaled_dims.height_px; ++row) {
            if (row % 8 == 0) {
                std::cout << "\n";
            }
        for (uint col = 0; col < scaled_dims.width_px; ++col) {
            if (col && col % 8 == 0) {
                std::cout << " ";
            }
            fmt::print("{:0>2} ", dst_array[scaled_dims.width_px * row + col]);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void run_targeted_test5() {
    
    constexpr uint src_width_px = 24;
    constexpr uint src_height_px = 24;

    constexpr uint dst_width_px = 7;
    constexpr uint dst_height_px = 7;
    const Dimensions scaled_dims {dst_width_px, dst_height_px};


    int src_array[64];
    for (uint row = 0; row < 8; ++row) {
        for (uint col = 0; col < 8; ++col) {
            src_array[8 * row + col] = 10;
        }
    }

    uint8_t dst_array[dst_width_px * dst_height_px] {};

    DownscalingBlockWriter<dst_width_px, dst_height_px> writer;
    writer.init_frame(dst_array, src_width_px, src_height_px);

    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);

    std::cout << "P2\n" << scaled_dims.width_px << " " << scaled_dims.height_px << " " << 255 << "\n";
    for (uint row = 0; row < scaled_dims.height_px; ++row) {
            if (row % 8 == 0) {
                std::cout << "\n";
            }
        for (uint col = 0; col < scaled_dims.width_px; ++col) {
            if (col && col % 8 == 0) {
                std::cout << " ";
            }
            fmt::print("{:0>2} ", dst_array[scaled_dims.width_px * row + col]);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void run_targeted_test6() {
    
    constexpr uint src_width_px = 24;
    constexpr uint src_height_px = 24;

    constexpr uint dst_width_px = 10;
    constexpr uint dst_height_px = 10;
    const Dimensions scaled_dims {dst_width_px, dst_height_px};


    int src_array[64];
    for (uint row = 0; row < 8; ++row) {
        for (uint col = 0; col < 8; ++col) {
            src_array[8 * row + col] = 10;
        }
    }

    uint8_t dst_array[dst_width_px * dst_height_px] {};

    DownscalingBlockWriter<dst_width_px, dst_height_px> writer;
    writer.init_frame(dst_array, src_width_px, src_height_px);

    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);

    std::cout << "P2\n" << scaled_dims.width_px << " " << scaled_dims.height_px << " " << 255 << "\n";
    for (uint row = 0; row < scaled_dims.height_px; ++row) {
            if (row % 8 == 0) {
                std::cout << "\n";
            }
        for (uint col = 0; col < scaled_dims.width_px; ++col) {
            if (col && col % 8 == 0) {
                std::cout << " ";
            }
            fmt::print("{:0>2} ", dst_array[scaled_dims.width_px * row + col]);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void run_targeted_test7() {
    
    constexpr uint src_width_px = 32;
    constexpr uint src_height_px = 32;

    constexpr uint dst_width_px = 31;
    constexpr uint dst_height_px = 31;
    const Dimensions scaled_dims {dst_width_px, dst_height_px};


    int src_array[64];
    for (uint row = 0; row < 8; ++row) {
        for (uint col = 0; col < 8; ++col) {
            src_array[8 * row + col] = 10;
        }
    }

    uint8_t dst_array[dst_width_px * dst_height_px] {};

    DownscalingBlockWriter<dst_width_px, dst_height_px> writer;
    writer.init_frame(dst_array, src_width_px, src_height_px);

    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);
    writer.write(src_array);

    std::cout << "P2\n" << scaled_dims.width_px << " " << scaled_dims.height_px << " " << 255 << "\n";
    for (uint row = 0; row < scaled_dims.height_px; ++row) {
            if (row % 8 == 0) {
                std::cout << "\n";
            }
        for (uint col = 0; col < scaled_dims.width_px; ++col) {
            if (col && col % 8 == 0) {
                std::cout << " ";
            }
            fmt::print("{:0>2} ", dst_array[scaled_dims.width_px * row + col]);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}
