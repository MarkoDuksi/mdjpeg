// #define PRINT_STATES_FLOW
// #define PRINT_HUFFMAN_TABLES

#include <iostream>

#include "tests.h"


int main(int argc, char** argv) {

    if (argc != 2) {

        std::cout << "usage: " << argv[0] << " tests_base_directory" << "\n";
    }

    std::filesystem::path input_base_dir = argv[1];

    // run_targeted_test1(input_base_dir);

    // run_targeted_test2();

    // run_integral_decoding_tests(input_base_dir, {160, 120});
    // run_integral_decoding_tests(input_base_dir, {1280, 1024});
    // run_integral_decoding_tests(input_base_dir, {1600, 1200});

    // run_partial_decoding_tests(input_base_dir);

    // run_downscaling_decoding_tests<800, 800, 800, 800>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 799, 799>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 798, 798>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 797, 797>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 796, 796>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 795, 795>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 794, 794>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 793, 793>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 792, 792>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 791, 791>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 790, 790>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 788, 788>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 785, 785>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 782, 782>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 779, 779>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 776, 776>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 773, 773>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 770, 770>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800,7 760, 760>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 750, 750>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 740, 740>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 730, 730>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 720, 720>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 710, 710>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 700, 700>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 690, 690>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 680, 680>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 670, 670>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 660, 660>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 650, 650>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 640, 640>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 630, 630>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 620, 620>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 610, 610>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 600, 600>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 600, 300>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 300, 600>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 128, 128>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 96, 96>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 74, 74>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 60, 60>(input_base_dir);
    // run_downscaling_decoding_tests<800, 800, 28, 28>(input_base_dir);

    // run_downscaling_decoding_tests<200, 200, 200, 200>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 199, 199>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 198, 198>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 197, 197>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 196, 196>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 195, 195>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 194, 194>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 193, 193>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 192, 192>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 191, 191>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 190, 190>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 188, 188>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 185, 185>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 182, 182>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 179, 179>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 176, 176>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 173, 173>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 170, 170>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 160, 160>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 150, 150>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 140, 140>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 130, 130>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 128, 128>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 120, 120>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 110, 110>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 100, 100>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 96, 96>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 90, 90>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 80, 80>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 74, 74>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 70, 70>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 60, 60>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 50, 50>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 40, 40>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 30, 30>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 28, 28>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 20, 20>(input_base_dir);
    // run_downscaling_decoding_tests<200, 200, 10, 10>(input_base_dir);

    run_downscaling_decoding_tests<40, 40, 40, 40>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 39, 39>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 38, 38>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 37, 37>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 36, 36>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 35, 35>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 34, 34>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 33, 33>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 32, 32>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 31, 31>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 30, 30>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 28, 28>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 25, 25>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 22, 22>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 19, 19>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 16, 16>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 13, 13>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 10, 10>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 9, 9>(input_base_dir);
    run_downscaling_decoding_tests<40, 40, 8, 8>(input_base_dir);

    return 0;
}
