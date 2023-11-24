#include <iostream>

#include "tests/tests.h"


int main(int argc, char** argv) {

    if (argc != 2) {

        std::cout << "usage: " << argv[0] << " test_images_directory\n";

        return 1;
    }

    std::filesystem::path test_imgs_dir = argv[1];
    // - non-visual tests write output of failed tests to `test_imgs_dir`
    // - visual tests read input from `test_imgs_dir/MxN` (`M` and `N` being
    //   width and height) and write output in subdirectories thereof

    // track counts of failed tests
    uint failed_batched_tests_count = 0;
    uint total_failed_tests_count = 0;

    /////////////////////////
    // start non-visual tests
    // - non-visual tests cover the DownscalingBlockWriter implementation
    // - variable parameters include original and downscaled image resolution
    //   as well as a single fill value for all pixels in mocked original image
    // - the "failed" or "passed" status is definite
    // - in case of failure, downscaled output is written as PGM image for manual inspection
    
    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(0, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(1, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(127, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(254, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<120, 120, 120, 120>(255, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<800, 800, 800, 800>(0, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<800, 800, 800, 800>(1, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<800, 800, 800, 800>(127, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    failed_batched_tests_count = recursive_downscaling_test<800, 800, 800, 800>(255, test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // end non-visual tests
    ///////////////////////

    /////////////////////
    // start visual tests
    // - visual tests cover various modes of decompressing images
    // - the "failed" status is definite but "passed?" is not
    // - for definite confirmation of passing a test visual inspection
    //   is needed
    // - alternatively, output of initial rounds can be saved as a
    //   reference for diffing with output of future iterations

    // synthetic test images (small size, tracked by git)
    failed_batched_tests_count = full_frame_decoding_tests(test_imgs_dir, {160, 120});
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // synthetic test images (small size, tracked by git)
    failed_batched_tests_count = full_frame_dc_decoding_tests(test_imgs_dir, {160, 120});
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // synthetic test image (medium size, tracked by git)
    failed_batched_tests_count = full_frame_dc_decoding_tests(test_imgs_dir, {800, 800});
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // synthetic test image (medium size, tracked by git)
    failed_batched_tests_count = cropped_decoding_tests(test_imgs_dir);  // 800x800 implicit
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // synthetic test image (medium size, tracked by git)
    failed_batched_tests_count = recursive_downscaling_decoding_test<800, 800, 800, 800>(test_imgs_dir);
    std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";
    total_failed_tests_count += failed_batched_tests_count;

    // // real ESP32-CAM images (large size, not tracked by git)
    // failed_batched_tests_count = full_frame_decoding_tests(test_imgs_dir, {1280, 1024});
    // std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";

    // // real ESP32-CAM images (large size, not tracked by git)
    // failed_batched_tests_count = full_frame_decoding_tests(test_imgs_dir, {1600, 1200});
    // std::cout << "failed tests count for this batch: " << failed_batched_tests_count << "\n\n";

    // end visual tests
    ///////////////////

    // add your own test images and/or run other tests...

    std::cout << "total failed tests count for all batches: " << total_failed_tests_count << "\n\n";
    std::cout << "Note: Currently, some downscaling tests fail with sparse +/- 1 rounding errors "
              << "due to finite precission floating point accumulating buffers. However, the "
              << "incidence and magnitude of those errors are insignificant compared to errors "
              << "introduced by the lossy nature of JPEG compression.\n\n";

    return total_failed_tests_count != 0;
}
