// #define PRINT_STATES_FLOW
// #define PRINT_HUFFMAN_TABLES

#include <iostream>

#include "tests.h"


int main(int argc, char** argv) {

    if (argc != 2) {

        std::cout << "usage: " << argv[0] << " tests_base_directory" << "\n";
    }

    const char* input_base_dir = argv[1];
    // Dimensions dim {160, 120};
    // Dimensions dim {1280, 1024};
    Dimensions dim {1600, 1200};
    run_tests(input_base_dir, dim);

    return 0;
}
