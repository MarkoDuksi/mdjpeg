#include <iostream>

#include "tests/tests.h"


int main(int argc, char** argv) {

    // start automated tests
    uint failed_tests_count {};
    
    // failed_tests_count = recursive_downscaling_test<120, 120, 120, 120>(0);
    // std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    // failed_tests_count = recursive_downscaling_test<120, 120, 120, 120>(1);
    // std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    // failed_tests_count = recursive_downscaling_test<120, 120, 120, 120>(127);
    // std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    // failed_tests_count = recursive_downscaling_test<120, 120, 120, 120>(254);
    // std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    // failed_tests_count = recursive_downscaling_test<120, 120, 120, 120>(255);
    // std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    failed_tests_count = recursive_downscaling_test<800, 800, 800, 800>(0);
    std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    failed_tests_count = recursive_downscaling_test<800, 800, 800, 800>(1);
    std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    failed_tests_count = recursive_downscaling_test<800, 800, 800, 800>(127);
    std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    failed_tests_count = recursive_downscaling_test<800, 800, 800, 800>(255);
    std::cout << "total # of failed tests in this batch: " << failed_tests_count << "\n\n";

    std::cout << "Note: It is normal for some downscaling tests to fail with sparse +/- 1 rounding errors "
              << "due to finite precission floating point accumulating buffers.\n\n";

    // end automated tests

    // start non-automated tests
    if (argc != 2) {

        std::cout << "For other, non-automated tests:\n"
                  << "  1) run `" << argv[0] << " tests" << "`\n"
                  << "  2) visually inspect decompressed output images in the `tests` tree.\n";

        return failed_tests_count;
    }

    // non-automated tests use test images saved in  `argv[1]` tree, grouped
    // in subdirectories named `NxM`, where `N` is width, `M` is height
    std::filesystem::path input_base_dir = argv[1];

    // synthetic, small test images (tracked by git)
    full_frame_decoding_tests(input_base_dir, {160, 120});
    cropped_decoding_tests(input_base_dir);  // 800x800

    // real ESP32-CAM images (not tracked by git)
    // full_frame_decoding_tests(input_base_dir, {1280, 1024});
    // full_frame_decoding_tests(input_base_dir, {1600, 1200});

    // add your own test images and/or run other tests...

    return failed_tests_count;
}
