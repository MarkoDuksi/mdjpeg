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
    std::ifstream file (file_path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    
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

        uint8_t decoded_img[160*120] {0};

        decoder.decode(decoded_img);

        std::cout << "processed image: " << file_path << "\n\n";

        for (uint i = 0; i < 120; ++i) {
            for (uint j = 0; j < 160; ++j) {
                std::cout << std::dec << (int)decoded_img[160 * i + j] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";

        delete[] buff;
    }

    return 0;
}
