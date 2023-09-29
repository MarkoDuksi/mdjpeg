// #define PRINT_STATES_FLOW
// #define PRINT_HUFFMAN_TABLES

#include <iostream>

#include "tests.h"


int main(int argc, char** argv) {

    if (argc != 2) {

        std::cout << "usage: " << argv[0] << " tests_base_directory" << "\n";
    }

    std::filesystem::path input_base_dir = argv[1];

    // full_frame_decoding_tests(input_base_dir, {160, 120});
    // full_frame_decoding_tests(input_base_dir, {1280, 1024});
    // full_frame_decoding_tests(input_base_dir, {1600, 1200});

    // cropped_decoding_tests(input_base_dir);

    targeted_downscaling_decoding_test<32, 8, 31, 7>(10);
    targeted_downscaling_decoding_test<16, 16, 7, 7>(10);
    targeted_downscaling_decoding_test<16, 16, 9, 9>(10);
    targeted_downscaling_decoding_test<24, 24, 7, 7>(10);
    targeted_downscaling_decoding_test<24, 24, 10, 10>(10);
    targeted_downscaling_decoding_test<32, 32, 31, 31>(10);
    targeted_downscaling_decoding_test<800, 800, 788, 788>(111);

    downscaling_decoding_tests<2000, 2000,  2000, 2000>(input_base_dir);
    downscaling_decoding_tests<2000, 2000,  1999, 1999>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1998, 1998>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1997, 1997>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1996, 1996>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1995, 1995>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1994, 1994>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1993, 1993>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1992, 1992>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1991, 1991>(input_base_dir);
    downscaling_decoding_tests<2000, 2000, 1990, 1990>(input_base_dir);

    return 0;
}
