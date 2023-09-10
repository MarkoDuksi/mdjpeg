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

#include "JpegDecoder.h"
#include "BlockWriter.h"
#include "tests.h"


std::vector<std::filesystem::path> getInputImgPaths(const std::filesystem::path& input_base_dir, const Dimensions& dim) {
    
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

    auto size = file.tellg();
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

bool test1(JpegDecoder& decoder, std::filesystem::path file_path, const Dimensions& dim) {

    const uint     x1_blk = 0;
    const uint     y1_blk = 0;
    const uint  width_blk = dim.width_px / 8;
    const uint height_blk = dim.height_px / 8;

    uint8_t* decoded_img = new uint8_t[dim.width_px * dim.height_px];

    if (decoder.decode(decoded_img, x1_blk, y1_blk, x1_blk + width_blk, y1_blk + height_blk)) {

        std::filesystem::path output_file_path = file_path.replace_extension("pgm");

        if (write_pgm(output_file_path, decoded_img, dim)) {

            delete[] decoded_img;
            return true;
        }
        else {

            std::cout << "Writing output failed.\n";

            delete[] decoded_img;
            return false;
        }
    }
    else {

        std::cout << "Decoding JPEG failed.\n";

        delete[] decoded_img;
        return false;
    }
}

void run_tests(std::filesystem::path input_base_dir, const Dimensions& dim) {

    auto input_files_paths = getInputImgPaths(input_base_dir, dim);

    for (const auto& file_path : input_files_paths) {

        auto [buff, size] = read_raw_jpeg_from_file(file_path);

        JpegDecoder decoder(buff, size);

        // BasicBlockWriter writer1;

        if (test1(decoder, file_path, dim)) {

            std::cout << "test1 passed on \"" << file_path.c_str() << "\"\n";
        }
        else {

            std::cout << "test1 FAILED on \"" << file_path.c_str() << "\"\n";
        }

        delete[] buff;
    }
}
