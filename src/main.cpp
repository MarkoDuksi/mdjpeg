// #define PRINT_STATES_FLOW
// #define PRINT_HUFFMAN_TABLES

#include <iostream>

#include "tests.h"


int main(int argc, char** argv) {

    if (argc != 2) {

        std::cout << "usage: " << argv[0] << " tests_base_directory" << "\n";
    }

    std::filesystem::path input_base_dir = argv[1];

    run_targeted_test1(input_base_dir);
    // run_integral_decoding_tests(input_base_dir, {160, 120});
    // run_integral_decoding_tests(input_base_dir, {1280, 1024});
    // run_integral_decoding_tests(input_base_dir, {1600, 1200});

    // run_partial_decoding_tests(input_base_dir);

    // run_downscaling_decoding_tests<800, 800>(input_base_dir);
    // run_downscaling_decoding_tests<799, 799>(input_base_dir);
    // run_downscaling_decoding_tests<798, 798>(input_base_dir);
    // run_downscaling_decoding_tests<797, 797>(input_base_dir);
    // run_downscaling_decoding_tests<796, 796>(input_base_dir);
    // run_downscaling_decoding_tests<795, 795>(input_base_dir);
    // run_downscaling_decoding_tests<794, 794>(input_base_dir);
    // run_downscaling_decoding_tests<793, 793>(input_base_dir);
    // run_downscaling_decoding_tests<792, 792>(input_base_dir);
    // run_downscaling_decoding_tests<791, 791>(input_base_dir);
    // run_downscaling_decoding_tests<790, 790>(input_base_dir);
    // run_downscaling_decoding_tests<788, 788>(input_base_dir);
    // run_downscaling_decoding_tests<785, 785>(input_base_dir);
    // run_downscaling_decoding_tests<782, 782>(input_base_dir);
    // run_downscaling_decoding_tests<779, 779>(input_base_dir);
    // run_downscaling_decoding_tests<776, 776>(input_base_dir);
    // run_downscaling_decoding_tests<773, 773>(input_base_dir);
    // run_downscaling_decoding_tests<770, 770>(input_base_dir);
    // run_downscaling_decoding_tests<760, 760>(input_base_dir);
    // run_downscaling_decoding_tests<750, 750>(input_base_dir);
    // run_downscaling_decoding_tests<740, 740>(input_base_dir);
    // run_downscaling_decoding_tests<730, 730>(input_base_dir);
    // run_downscaling_decoding_tests<720, 720>(input_base_dir);
    // run_downscaling_decoding_tests<710, 710>(input_base_dir);
    // run_downscaling_decoding_tests<700, 700>(input_base_dir);
    // run_downscaling_decoding_tests<690, 690>(input_base_dir);
    // run_downscaling_decoding_tests<680, 680>(input_base_dir);
    // run_downscaling_decoding_tests<670, 670>(input_base_dir);
    // run_downscaling_decoding_tests<660, 660>(input_base_dir);
    // run_downscaling_decoding_tests<650, 650>(input_base_dir);
    // run_downscaling_decoding_tests<640, 640>(input_base_dir);
    // run_downscaling_decoding_tests<630, 630>(input_base_dir);
    // run_downscaling_decoding_tests<620, 620>(input_base_dir);
    // run_downscaling_decoding_tests<610, 610>(input_base_dir);
    // run_downscaling_decoding_tests<600, 600>(input_base_dir);
    // run_downscaling_decoding_tests<600, 300>(input_base_dir);
    // run_downscaling_decoding_tests<300, 600>(input_base_dir);
    // run_downscaling_decoding_tests<128, 128>(input_base_dir);
    // run_downscaling_decoding_tests<96, 96>(input_base_dir);
    // run_downscaling_decoding_tests<74, 74>(input_base_dir);
    // run_downscaling_decoding_tests<60, 60>(input_base_dir);
    // run_downscaling_decoding_tests<28, 28>(input_base_dir);

    return 0;
}
