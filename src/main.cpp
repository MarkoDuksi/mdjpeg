// #define PRINT_STATES_FLOW
// #define PRINT_HUFFMAN_TABLES

#include <iostream>

#include "tests.h"


int main(int argc, char** argv) {

    if (argc != 2) {

        std::cout << "usage: " << argv[0] << " tests_base_directory" << "\n";
    }

    std::filesystem::path input_base_dir = argv[1];

    run_integral_decoding_tests(input_base_dir, {160, 120});
    run_integral_decoding_tests(input_base_dir, {1280, 1024});
    run_integral_decoding_tests(input_base_dir, {1600, 1200});
    
    run_partial_decoding_tests(input_base_dir);

    return 0;
}
