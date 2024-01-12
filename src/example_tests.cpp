#include <filesystem>
#include <iostream>

#include "tests/tests.h"


int main(int argc, char** argv) {

    // providing the path to existing test images directory is a must
    if (argc != 2 || !std::filesystem::is_directory(argv[1])) {

        std::cout << "usage: " << argv[0] << " existing_test_images_directory\n";

        return 1;
    }

    const std::filesystem::path test_imgs_dir = argv[1];

    // track counts of failed tests
    uint failed_batched_tests_count = 0;
    uint total_failed_tests_count = 0;

    /////////////////////////////
    // start downscaling tests //
    
    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(0, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(1, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(127, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(254, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(255, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<800, 800, 800, 800>(0, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<800, 800, 800, 800>(1, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<800, 800, 800, 800>(127, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<800, 800, 800, 800>(255, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // end downscaling tests //
    ///////////////////////////

    ///////////////////////////////
    // start decompression tests //

    // full frame, 1:1 scale //
    
    // synthetic test images (small size, tracked by git)
    failed_batched_tests_count = full_frame_decoding_tests({160, 120}, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // synthetic test image (medium size, tracked by git)
    failed_batched_tests_count = full_frame_decoding_tests({800, 800}, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // // actual ESP32-CAM images (large size, NOT TRACKED by git)
    // failed_batched_tests_count = full_frame_decoding_tests({1280, 1024}, test_imgs_dir);
    // std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";

    // // actual ESP32-CAM images (large size, NOT TRACKED by git)
    // failed_batched_tests_count = full_frame_decoding_tests({1600, 1200}, test_imgs_dir);
    // std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";

    // full frame, 1:8 scale (DC-only) //
    
    // synthetic test images (small size, tracked by git)
    failed_batched_tests_count = full_frame_dc_decoding_tests({160, 120}, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // synthetic test image (medium size, tracked by git)
    failed_batched_tests_count = full_frame_dc_decoding_tests({800, 800}, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // // actual ESP32-CAM images (large size, NOT TRACKED by git)
    // failed_batched_tests_count = full_frame_dc_decoding_tests({1280, 1024}, test_imgs_dir);
    // std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";

    // // actual ESP32-CAM images (large size, NOT TRACKED by git)
    // failed_batched_tests_count = full_frame_dc_decoding_tests({1600, 1200}, test_imgs_dir);
    // std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";

    // cropped frame, 1:1 scale //

    // synthetic test image (medium size, tracked by git)
    failed_batched_tests_count = cropped_decoding_tests({800, 800}, test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // full frame, adaptive scale //

    // synthetic test image (medium size, tracked by git)
    failed_batched_tests_count = recursive_downscaling_decoding_test<800, 800, 799, 799>(test_imgs_dir);
    std::cout << "Failed tests count in this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // end decompression tests //
    /////////////////////////////

    // add your own test images and/or run other tests...

    std::cout << "Total failed tests count across all batches: " << total_failed_tests_count << "\n\n";
    std::cout << "Note 1: Single precision floating point math used by DownscalingBlockWriter\n"
              << "introduces sparse +/- 1 errors in values of some downscaled frame buffers.\n"
              << "Extent of these errors depends on frame buffer dimensions and fill value.\n"
              << "Effects of these errors on real images are insignificant compared to errors\n"
              << "introduced beforehand by the lossiness of JPEG compression.\n\n";
    std::cout << "Note 2: To validate the output of tests that have passed tentatively against\n"
              << "known checksums: `make tests-validate`.\n\n";
    std::cout << "Note 3: To update the checksums list: `make tests-update`.\n\n";

    return total_failed_tests_count != 0;
}
