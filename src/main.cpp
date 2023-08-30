#include <iostream>
#include <ios>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <memory>

#include <cstdio>

#include "JpegDecoder.h"
#include "BlockWriter.h"


std::vector<std::filesystem::path> getInputImgPaths(const char* input_dir) {
    std::vector<std::filesystem::path> input_paths;

    for (const auto& dir_entry : std::filesystem::directory_iterator(input_dir)){
        if (dir_entry.path().extension() == ".jpg")
            input_paths.push_back(dir_entry.path());
    }
    std::sort(input_paths.begin(), input_paths.end());

    return input_paths;
}

std::tuple<uint8_t*, size_t> read_raw_jpeg_from_file(std::filesystem::path file_path) {
    std::ifstream file(file_path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cout << "Error1 reading file: " << file_path.c_str() << std::endl;
        return {nullptr, 0};
    }

    auto size = file.tellg();
    char* raw_jpeg = new char[size];

    file.seekg(0);
    file.read(raw_jpeg, size);

    if (file.gcount() != size) {
        std::cout << "Error2 reading file: " << file_path.c_str() << std::endl;
        return {nullptr, 0};
    }

    return {reinterpret_cast<uint8_t*>(raw_jpeg), size};
}

bool write_pgm(const char* file_name, uint8_t* raw_image_data, uint width_px, uint height_px) {
    std::ofstream file(file_name);
    if (!file.is_open()) {
        std::cout << "Error1 opening file: " << file_name << std::endl;
        return false;
    }

    file << "P2\n" << width_px << " " << height_px << " " << 255 << "\n";

    for (uint row = 0; row < height_px; ++row) {
        for (uint col = 0; col < width_px; ++col) {
            file << static_cast<uint>(*raw_image_data++) << " ";
        }
        file << "\n";
    }

    file << std::endl;
    file.close();

    return true;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " input_directory" << std::endl;
        exit(-1);
    }
    auto input_paths = getInputImgPaths(argv[1]);

    // for (const auto& file_path : input_paths) {
    for (size_t i = 0; i < 1; ++i) {
        auto file_path = input_paths[i];

        std::cout << "\nprocessing image: " << file_path << "\n";

        auto [buff, size] = read_raw_jpeg_from_file(file_path);

        JpegDecoder decoder(buff, size);
        DirectBlockWriter writer;

        constexpr uint     x1_blk = 0;
        constexpr uint     y1_blk = 0;
        constexpr uint  width_blk = 20;
        constexpr uint height_blk = 15;

        uint8_t decoded_img[8 * width_blk * 8 * height_blk] {0};
        if (decoder.decode(decoded_img, x1_blk, y1_blk, x1_blk + width_blk, y1_blk + height_blk, writer)) {
            for (uint i = 0; i < 8 * height_blk; ++i) {
                for (uint j = 0; j < 8 * width_blk; ++j) {
                    std::cout << std::dec << (int)decoded_img[8 * width_blk * i + j] << " ";
                }
                std::cout << "\n";
            }
        }

        write_pgm("../output/test1.pgm", decoded_img, 160, 120);

        uint8_t decoded_img2[width_blk * height_blk] {0};
        if (decoder.dc_decode(decoded_img2, x1_blk, y1_blk, x1_blk + width_blk, y1_blk + height_blk)) {
            for (uint i = 0; i < height_blk; ++i) {
                for (uint j = 0; j < width_blk; ++j) {
                    std::cout << std::dec << (int)decoded_img2[width_blk * i + j] << " ";
                }
                std::cout << "\n";
            }
        }

        write_pgm("../output/test2.pgm", decoded_img2, 20, 15);

        std::cout << "\nprocessed image: " << file_path << "\n\n";

        delete[] buff;
    }

    return 0;
}
