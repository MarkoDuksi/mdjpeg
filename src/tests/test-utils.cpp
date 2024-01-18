#include "test-utils.h"

#include <algorithm>
#include <fstream>
#include <ios>
#include <iostream>

#include <fmt/core.h>


std::vector<std::filesystem::path> mdjpeg_test_utils::get_input_img_paths(const std::filesystem::path& input_dir) {

    std::vector<std::filesystem::path> input_files_paths;

    if (!std::filesystem::is_directory(input_dir)) {

        return input_files_paths;
    }

    for (const auto& dir_entry : std::filesystem::directory_iterator(input_dir)){

        if (dir_entry.path().extension() == ".jpg") {

            input_files_paths.push_back(dir_entry.path());
        }
    }

    std::sort(input_files_paths.begin(), input_files_paths.end());

    return input_files_paths;
}

std::pair<uint8_t*, size_t> mdjpeg_test_utils::read_raw_jpeg_from_file(const std::filesystem::path& file_path) {

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
        delete[] raw_jpeg;

        return {nullptr, 0};
    }

    file.close();

    return {reinterpret_cast<uint8_t*>(raw_jpeg), size};
}

uint mdjpeg_test_utils::max_abs_error(const uint8_t* const array, const Dimensions& dims, const uint8_t value) {

    int max_absolute_error = 0;

    for (uint row = 0; row < dims.height_px; ++row) {

        for (uint col = 0; col < dims.width_px; ++col) {

            const int error = array[row * dims.width_px + col] - value;
            max_absolute_error = std::max(max_absolute_error, std::abs(error));
        }
    }

    return max_absolute_error;
}

bool mdjpeg_test_utils::write_as_pgm(const std::filesystem::path& file_path, const uint8_t* const pixel_data, const uint16_t width_px, const uint16_t height_px) {

    std::ofstream file(file_path);

    if (!file.is_open()) {

        std::cout << "Error opening output file: " << file_path.c_str() << "\n";

        return false;
    }

    file << "P2\n" << width_px << " " << height_px << " " << 255 << "\n";

    uint idx = 0;
    for (uint row = 0; row < height_px; ++row) {

        for (uint col = 0; col < width_px; ++col) {

            file << static_cast<uint>(pixel_data[idx++]) << " ";
        }

        file << "\n";
    }

    file << "\n";
    file.close();

    return true;
}

void mdjpeg_test_utils::print_as_pgm(const uint8_t* const pixel_data, const uint16_t width_px, const uint16_t height_px) {

    std::cout << "P2\n" << width_px << " " << height_px << " " << 255 << "\n";

    uint idx = 0;
    for (uint row = 0; row < height_px; ++row) {

            if (row % 8 == 0) {

                std::cout << "\n";
            }

        for (uint col = 0; col < width_px; ++col) {

            if (col && col % 8 == 0) {

                std::cout << " ";
            }

            fmt::print("{:0>2} ", pixel_data[idx++]);
        }

        std::cout << "\n";
    }

    std::cout << "\n";
}
