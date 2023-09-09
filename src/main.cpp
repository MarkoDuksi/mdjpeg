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
        DirectBlockWriter writer1;


        constexpr uint     x1_blk1 = 0;
        constexpr uint     y1_blk1 = 0;
        constexpr uint  width_blk1 = 20;
        constexpr uint height_blk1 = 15;

        uint8_t decoded_img1[8 * width_blk1 * 8 * height_blk1] {0};
        if (!decoder.decode(decoded_img1, x1_blk1, y1_blk1, x1_blk1 + width_blk1, y1_blk1 + height_blk1)) {
            std::cout << "Decoding failed.\n";
        }
        else {
            std::cout << "Decoding successful.\n";
            std::cout << "  Writing to ../output/test1.pgm... ";
            if (!write_pgm("../output/test1.pgm", decoded_img1, 8 * width_blk1, 8 * height_blk1)) {
                std::cout << "Failed.\n";
            }
            else {
                std::cout << "Successful.\n";
            }
        }


        constexpr uint     x1_blk2 = 0;
        constexpr uint     y1_blk2 = 0;
        constexpr uint  width_blk2 = 20;
        constexpr uint height_blk2 = 15;

        uint8_t decoded_img2[width_blk2 * height_blk2] {0};
        if (!decoder.dc_decode(decoded_img2, x1_blk2, y1_blk2, x1_blk2 + width_blk2, y1_blk2 + height_blk2)) {
            std::cout << "Decoding failed.\n";
        }
        else {
            std::cout << "Decoding successful.\n";
            std::cout << "  Writing to ../output/test2.pgm... ";
            if (!write_pgm("../output/test2.pgm", decoded_img2, width_blk2, height_blk2)) {
                std::cout << "Failed.\n";
            }
            else {
                std::cout << "Successful.\n";
            }
        }


        constexpr uint     x1_blk3 = 1;
        constexpr uint     y1_blk3 = 1;
        constexpr uint  width_blk3 = 7;
        constexpr uint height_blk3 = 5;

        uint8_t decoded_img3[8 * width_blk3 * 8 * height_blk3] {0};
        if (!decoder.decode(decoded_img3, x1_blk3, y1_blk3, x1_blk3 + width_blk3, y1_blk3 + height_blk3, writer1)) {
            std::cout << "Decoding failed.\n";
        }
        else {
            std::cout << "Decoding successful.\n";
            std::cout << "  Writing to ../output/test3.pgm... ";
            if (!write_pgm("../output/test3.pgm", decoded_img3, 8 * width_blk3, 8 * height_blk3)) {
                std::cout << "Failed.\n";
            }
            else {
                std::cout << "Successful.\n";
            }
        }


        constexpr uint     x1_blk4 = 6;
        constexpr uint     y1_blk4 = 4;
        constexpr uint  width_blk4 = 8;
        constexpr uint height_blk4 = 7;

        uint8_t decoded_img4[8 * width_blk4 * 8 * height_blk4] {0};
        if (!decoder.decode(decoded_img4, x1_blk4, y1_blk4, x1_blk4 + width_blk4, y1_blk4 + height_blk4, writer1)) {
            std::cout << "Decoding failed.\n";
        }
        else {
            std::cout << "Decoding successful.\n";
            std::cout << "  Writing to ../output/test4.pgm... ";
            if (!write_pgm("../output/test4.pgm", decoded_img4, 8 * width_blk4, 8 * height_blk4)) {
                std::cout << "Failed.\n";
            }
            else {
                std::cout << "Successful.\n";
            }
        }


        constexpr uint     x1_blk5 = 0;
        constexpr uint     y1_blk5 = 0;
        constexpr uint  width_blk5 = 20;
        constexpr uint height_blk5 = 15;
        constexpr uint downscaled_width_px5 = 2 * width_blk5;
        constexpr uint downscaled_height_px5 = 2 * height_blk5;
        DownscalingBlockWriter<downscaled_width_px5, downscaled_height_px5> writer2;

        uint8_t decoded_img5[downscaled_width_px5 * downscaled_height_px5] {0};
        if (!decoder.decode(decoded_img5, x1_blk5, y1_blk5, x1_blk5 + width_blk5, y1_blk5 + height_blk5, writer2)) {
            std::cout << "Decoding failed.\n";
        }
        else {
            std::cout << "Decoding successful.\n";
            std::cout << "  Writing to ../output/test5.pgm... ";
            if (!write_pgm("../output/test5.pgm", decoded_img5, downscaled_width_px5, downscaled_height_px5)) {
                std::cout << "Failed.\n";
            }
            else {
                std::cout << "Successful.\n";
            }
        }


        constexpr uint     x1_blk6 = 0;
        constexpr uint     y1_blk6 = 0;
        constexpr uint  width_blk6 = 20;
        constexpr uint height_blk6 = 15;
        constexpr uint downscaled_width_px6 = 8 * width_blk6;
        constexpr uint downscaled_height_px6 = 8 * height_blk6;
        DownscalingBlockWriter<downscaled_width_px6, downscaled_height_px6> writer3;

        uint8_t decoded_img6[downscaled_width_px6 * downscaled_height_px6] {0};
        if (!decoder.decode(decoded_img6, x1_blk6, y1_blk6, x1_blk6 + width_blk6, y1_blk6 + height_blk6, writer3)) {
            std::cout << "Decoding failed.\n";
        }
        else {
            std::cout << "Decoding successful.\n";
            std::cout << "  Writing to ../output/test6.pgm... ";
            if (!write_pgm("../output/test6.pgm", decoded_img6, downscaled_width_px6, downscaled_height_px6)) {
                std::cout << "Failed.\n";
            }
            else {
                std::cout << "Successful.\n";
            }
        }


        std::cout << "\nprocessed image: " << file_path << "\n\n";

        delete[] buff;
    }

    return 0;
}
