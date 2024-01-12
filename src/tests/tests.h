#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <cassert>
#include <string>
#include <filesystem>
#include <iostream>

#include "../JpegDecoder.h"
#include "../DownscalingBlockWriter.h"
#include "test-utils.h"


/// \brief Tests full frame, 1/8-scale (DC only) decompression on a batch of JPEG images.
///
/// \param src_dims       Input images width and height, both must be multiples of 8.
/// \param test_imgs_dir  Base directory for test images.
/// \param output_subdir  Subdirectory for diagnostic output.
/// \return               Total count of failed tests in this batch.
///
/// Specifically tests JpegDecoder::dc_luma_decode in full frame mode. Images
/// matching "`test_imgs_dir`/`src_dims.width_px`x`src_dims.height_px`/*.jpg"
/// are processed individually by decompressing them directly to 1/8 of their
/// original width and height. The resulting scaled-down luma-only images are
/// written to "`test_imgs_dir`/`output_subdir`" in 8-bit ASCII PGM format. The
/// output directory is created if it does not exist.
///
/// \par PASSED/FAILED criteria, reporting
/// A test can fail on a particular image either because decompression fails or
/// because writing the output image to the filesystem fails. The cause of
/// failure is reported to stdout per image basis and the failed tests counter
/// is incremented by one. Otherwise, the test passes tentatively and is
/// reported to stdout as such.
///
/// \par Example input images
/// Example input images are provided along with checksums of expected output.
/// Output of tests that passed tentatively should be validated against the
/// checksums by running `make tests-validate` to obtain the final passed/failed
/// verdict.
///
/// \par Custom input images
/// The quality of downscaling depends on the nature of motifs captured in the
/// images. Users can populate the test image directories with JPEG images of
/// typical motifs in their use case. After running the tests, output images
/// written to the filesystem can be visually inspected and the quality judged
/// as either applicable to intended purpose or not. If checksums of the output
/// images are desired as references for future validations, the checksums list
/// can be updated by running `make tests-update`.
uint full_frame_dc_decoding_tests(
    const mdjpeg_test_utils::Dimensions& src_dims,
    const std::filesystem::path& test_imgs_dir,
    const std::filesystem::path& output_subdir = "decoded_DC-only"
);

/// \brief Tests full frame, 1:1 scale decompression on a batch of JPEG images.
///
/// \param src_dims       Input images width and height, both must be multiples of 8.
/// \param test_imgs_dir  Base directory for test images.
/// \param output_subdir  Subdirectory for diagnostic output.
/// \return               Total count of failed tests in this batch.
///
/// Specifically tests JpegDecoder::luma_decode in full frame mode. Images
/// matching "`test_imgs_dir`/`src_dims.width_px`x`src_dims.height_px`/*.jpg"
/// are processed individually by decompressing them in their full width and
/// height. The resulting luma-only images are written to
/// "`test_imgs_dir`/`output_subdir`" in 8-bit ASCII PGM format. The output
/// directory is created if it does not exist.
///
/// \par PASSED/FAILED criteria, reporting
/// A test can fail on a particular image either because decompression fails or
/// because writing the output image to the filesystem fails. The cause of
/// failure is reported to stdout per image basis and the failed tests counter
/// is incremented by one. Otherwise, the test passes tentatively and is
/// reported to stdout as such.
///
/// \par Example input images
/// Example input images are provided along with checksums of expected output.
/// Output of tests that passed tentatively should be validated against the
/// checksums by running `make tests-validate` to obtain the final passed/failed
/// verdict.
uint full_frame_decoding_tests(
    const mdjpeg_test_utils::Dimensions& src_dims,
    const std::filesystem::path& test_imgs_dir,
    const std::filesystem::path& output_subdir = "decoded_full_scale"
);

/// \brief Tests cropped frame, 1:1 scale decompression on a batch of JPEG images.
///
/// \param src_dims       Input images width and height, both must be multiples of 8.
/// \param test_imgs_dir  Base directory for test images.
/// \param output_subdir  Subdirectory for diagnostic output.
/// \return               Total count of failed tests in this batch.
///
/// Specifically tests JpegDecoder::luma_decode in crop mode. Images matching
/// "`test_imgs_dir`/`src_dims.width_px`x`src_dims.height_px`/*.jpg" are
/// processed individually by decompressing each one into 16 subimages tiled on
/// a 4 x 4 grid. The resulting luma-only subimages are written to
/// "`test_imgs_dir`/`output_subdir`" in 8-bit ASCII PGM format. The output
/// directory is created if it does not exist.
///
/// \par PASSED/FAILED criteria, reporting
/// A test can fail on a particular subimage either because decompression fails
/// or because writing the output subimage to the filesystem fails. The cause of
/// failure is reported to stdout per subimage basis and the failed tests
/// counter is incremented by one. For every image all 16 subimage tests are
/// performed even in some fail early. If none of them fail the test for a
/// particular image passes tentatively and is reported to stdout as such.
///
/// \par Example input image
/// Example input image is provided along with checksums of expected output
/// subimages. Output of tests that passed tentatively should be validated
/// against the checksums by running `make tests-validate` to obtain the final
/// passed/failed verdict.
uint cropped_decoding_tests(
    const mdjpeg_test_utils::Dimensions& src_dims,
    const std::filesystem::path& test_imgs_dir,
    const std::filesystem::path& output_subdir = "decoded_cropped"
);

/// \brief Tests downscaling on a homogeneous frame buffer.
///
/// \tparam SRC_WIDTH_PX   Input frame buffer width in pixels, must be a multiple of 8.
/// \tparam SRC_HEIGHT_PX  Input frame buffer height in pixels, must be a multiple of 8.
/// \tparam DST_WIDTH_PX   Output frame buffer width in pixels, must be no greater than \c SRC_WIDTH_PX.
/// \tparam DST_HEIGHT_PX  Output frame buffer height in pixels, must be no greater than \c SRC_HEIGHT_PX.
/// \param  fill_value     Unique value for every pixel in the frame buffer.
/// \param  test_imgs_dir  Base directory for test images.
/// \param  output_subdir  Subdirectory for diagnostic output.
/// \retval                true if passed.
/// \retval                false if failed.
///
/// Specifically tests DownscalingBlockWriter<DST_WIDTH_PX, DST_HEIGHT_PX>. A
/// mocked frame buffer filled entirely with \c fill_value is used instead of
/// decompressing a real image. If any of the output image elements differ from
/// \c fill_value, the element-wise maximum absolute error (difference) is
/// reported to stdout and the resulting (imperfectly) downscaled image written
/// to "`test_imgs_dir`/`output_subdir`" in 8-bit ASCII PGM format. The output
/// directory is created if it does not exist.
///
/// \par PASSED/FAILED criteria, reporting
/// The test can fail if writing the output image to the filesystem fails which
/// is reported to stdout. Otherwise, the test passes tentatively and is
/// reported to stdout as such.
///
/// \note Single precision floating point math used by DownscalingBlockWriter
/// introduces sparse +/- 1 errors in values of some downscaled frame buffers.
/// Extent of these errors depends on frame buffer dimensions and the magnitude
/// of \c fill_value. Effects of these errors on real images are insignificant
/// compared to errors introduced beforehand by the lossiness of JPEG
/// compression.
template <uint SRC_WIDTH_PX, uint SRC_HEIGHT_PX, uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
bool downscaling_test(const uint8_t fill_value,
                      const std::filesystem::path& test_imgs_dir,
                      const std::filesystem::path& output_subdir = "downscaling_diag") {

    using namespace mdjpeg_test_utils;

    const Dimensions src_dims {SRC_WIDTH_PX, SRC_HEIGHT_PX};
    const Dimensions dst_dims {DST_WIDTH_PX, DST_HEIGHT_PX};

    assert(src_dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    const auto output_dir = test_imgs_dir / output_subdir;

    int src_array[64];
    for (uint i = 0; i < 64; ++i) {

        src_array[i] = fill_value;
    }

    uint8_t dst_array[DST_WIDTH_PX * DST_HEIGHT_PX] {};

    DownscalingBlockWriter<DST_WIDTH_PX, DST_HEIGHT_PX> writer;
    writer.init(dst_array, SRC_WIDTH_PX, SRC_HEIGHT_PX);

    for (uint i = 0; i < src_dims.width_blk * src_dims.height_blk; ++i) {

        writer.write(src_array);
    }

    std::cout << "Downscaling test ("
              << src_dims.to_str() << " -> " << dst_dims.to_str()
              << " / fill value = " << static_cast<uint>(fill_value) << ")";

    const int error = max_abs_error(dst_array, dst_dims, fill_value);

    if (error == 0) {

        std::cout << ": PASSED\n";
    }

    else {

        std::cout << ": PASSED (tentative, max abs err = " << error << ")\n";

        std::filesystem::create_directory(output_dir);
        const std::filesystem::path filename = "failed_downscaling_from_" +
                                               src_dims.to_str() +
                                               "_to_" +
                                               dst_dims.to_str() +
                                               "_with_fill_value_" +
                                               std::to_string(static_cast<uint>(fill_value)) +
                                               ".pgm";
        
        if (!write_as_pgm(output_dir / filename, dst_array, dst_dims.width_px, dst_dims.height_px)) {

            std::cout << ": FAILED writing output\n";

            return false;
        }
    }

    return true;
}

/// \brief Runs a batched downscaling_test on fixed input dimensions over a range of output dimensions.
///
/// \tparam SRC_WIDTH_PX   Input frame buffer width in pixels, must be a multiple of 8.
/// \tparam SRC_HEIGHT_PX  Input frame buffer height in pixels, must be a multiple of 8.
/// \tparam DST_WIDTH_PX   Output frame buffer width in pixels, must be no greater than \c SRC_WIDTH_PX.
/// \tparam DST_HEIGHT_PX  Output frame buffer height in pixels, must be no greater than \c SRC_HEIGHT_PX.
/// \param  fill_value     Unique value for every pixel in the frame buffer.
/// \param  test_imgs_dir  Base directory for test images.
/// \param  output_subdir  Subdirectory for diagnostic output.
/// \param  tests_failed   Running counter of failed tests, do not set this param.
/// \return                Total count of failed tests in this batch.
///
/// The range of output dimensions is a sequence starting with a pair of
/// \c DST_WIDTH_PX and \c DST_HEIGHT_PX, decrementing each member by \c 1 as
/// long as both are still greater than zero. The test for a single pair of
/// output dimensions is described in downscaling_test.
///
/// \par Examples
/// Examples are provided for some combinations of input dimensions and fill
/// values across the entire applicable range of output dimensions along with
/// checksums of expected output (see the Note for downscaling_test). Output of
/// tests that passed tentatively should be validated against the checksums by
/// running `make tests-validate` to obtain the final passed/failed verdict.
///
/// \attention Recursive tests are both compile time and runtime resource
/// intensive.
template <uint SRC_WIDTH_PX, uint SRC_HEIGHT_PX, uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
uint recursive_downscaling_test(const uint8_t fill_value,
                                const std::filesystem::path& test_imgs_dir,
                                const std::filesystem::path& output_subdir = "downscaling_diag",
                                uint tests_failed = 0) {

    if constexpr (DST_WIDTH_PX && DST_HEIGHT_PX) {

        tests_failed += recursive_downscaling_test<SRC_WIDTH_PX, SRC_HEIGHT_PX, DST_WIDTH_PX - 1, DST_HEIGHT_PX - 1>(
            fill_value,
            test_imgs_dir,
            output_subdir,
            !downscaling_test<SRC_WIDTH_PX, SRC_HEIGHT_PX, DST_WIDTH_PX, DST_HEIGHT_PX>(
                fill_value,
                test_imgs_dir,
                output_subdir
            )
        );
    }

    return tests_failed;
}

/// \brief Tests full frame, downscaled decompression on a batch of JPEG images.
///
/// \tparam SRC_WIDTH_PX   Input image width in pixels, must be a multiple of 8.
/// \tparam SRC_HEIGHT_PX  Input image height in pixels, must be a multiple of 8.
/// \tparam DST_WIDTH_PX   Output image width in pixels, must be no greater than \c SRC_WIDTH_PX.
/// \tparam DST_HEIGHT_PX  Output image height in pixels, must be no greater than \c SRC_HEIGHT_PX.
/// \param  test_imgs_dir  Base directory for test images.
/// \param  output_subdir  Subdirectory for diagnostic output.
/// \return                Total count of failed tests in this batch.
///
/// Specifically tests JpegDecoder::luma_decode injected with
/// DownscalingBlockWriter<DST_WIDTH_PX, DST_HEIGHT_PX> in full frame mode.
/// Images matching "`test_imgs_dir`/`SRC_WIDTH_PX`x`SRC_HEIGHT_PX`/*.jpg" are
/// processed individually by decompressing them and downscaling accordingly.
/// The resulting scaled-down luma-only images are written to
/// "`test_imgs_dir`/`output_subdir`" in 8-bit ASCII PGM format. The output
/// directory is created if it does not exist.
///
/// \par PASSED/FAILED criteria, reporting
/// A test can fail on a particular image either because decompression fails or
/// because writing the output image to the filesystem fails. The cause of
/// failure is reported to stdout per image basis and the failed tests counter
/// is incremented by one. Otherwise, the test passes tentatively and is
/// reported to stdout as such.
///
/// \par Example input images
/// Example input images are provided along with checksums of expected output.
/// Output of tests that passed tentatively should be validated against the
/// checksums by running `make tests-validate` to obtain the final passed/failed
/// verdict.
template <uint SRC_WIDTH_PX, uint SRC_HEIGHT_PX, uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
uint downscaling_decoding_test(const std::filesystem::path& test_imgs_dir,
                               const std::filesystem::path& output_subdir = "decoded_downscaled") {

    using namespace mdjpeg_test_utils;

    const Dimensions src_dims {SRC_WIDTH_PX, SRC_HEIGHT_PX};
    assert(src_dims.is_8x8_multiple() && "invalid input dimensions (not multiples of 8)");

    const Dimensions dst_dims {DST_WIDTH_PX, DST_HEIGHT_PX};

    const auto input_files_dir = test_imgs_dir / src_dims.to_str();
    const auto input_files_paths = get_input_img_paths(input_files_dir);
    const auto output_dir = input_files_dir / output_subdir;

    uint tests_failed = 0;

    for (const auto& file_path : input_files_paths) {

        bool subtest_passed{true};

        std::cout << "Running downscaling decoding test on \"" << file_path.filename().c_str() << "\""
                  << " (" << src_dims.to_str() << " -> " << dst_dims.to_str() << ")";

        const auto [buff, size] = read_raw_jpeg_from_file(file_path);
        JpegDecoder decoder;
        decoder.assign(buff, size);
        DownscalingBlockWriter<DST_WIDTH_PX, DST_HEIGHT_PX> writer;

        uint8_t decoded_img[DST_WIDTH_PX * DST_HEIGHT_PX] {};
        if (decoder.luma_decode(decoded_img, {0, 0, SRC_WIDTH_PX / 8 , SRC_HEIGHT_PX / 8}, writer)) {

            std::filesystem::create_directory(output_dir);
            const std::filesystem::path filename = std::string(file_path.stem()) + "_" + dst_dims.to_str() + ".pgm";

            if (!write_as_pgm(output_dir / filename, decoded_img, dst_dims.width_px, dst_dims.height_px)) {

                subtest_passed = false;
                ++tests_failed;
                std::cout << ": FAILED writing output\n";
            }
        }

        else {

            subtest_passed = false;
            ++tests_failed;
            std::cout << ": FAILED decoding+downscaling JPEG\n";
        }

        delete[] buff;

        if (subtest_passed) {

            std::cout << ": PASSED (tentative)\n";
        }
    }

    return tests_failed;
}

/// \brief Runs a batched downscaling_decoding_test on fixed input dimensions over a range of output dimensions.
///
/// \tparam SRC_WIDTH_PX   Input image width in pixels, must be a multiple of 8.
/// \tparam SRC_HEIGHT_PX  Input image height in pixels, must be a multiple of 8.
/// \tparam DST_WIDTH_PX   Output image width in pixels, must be no greater than \c SRC_WIDTH_PX.
/// \tparam DST_HEIGHT_PX  Output image height in pixels, must be no greater than \c SRC_HEIGHT_PX.
/// \param  test_imgs_dir  Base directory for test images.
/// \param  tests_failed   Running counter of failed tests, do not set this param.
/// \return                Total count of failed tests in this batch.
///
/// The range of output dimensions is a sequence starting with a pair of
/// \c DST_WIDTH_PX and \c DST_HEIGHT_PX, decrementing each member by \c 1 as
/// long as both are still greater than zero. The test for a single pair of
/// output dimensions is described in downscaling_decoding_test.
///
/// \par Example
/// An example is provided for decompressing an 800x800 input image with
/// downscaling across the entire applicable range of output dimensions along
/// with checksums of expected output. Output of tests that passed tentatively
/// should be validated against the checksums by running `make tests-validate`
/// to obtain the final passed/failed verdict.
///
/// \attention Recursive tests are both compile time and runtime resource
/// intensive.
template <uint SRC_WIDTH_PX, uint SRC_HEIGHT_PX, uint DST_WIDTH_PX, uint DST_HEIGHT_PX>
uint recursive_downscaling_decoding_test(const std::filesystem::path& test_imgs_dir,
                                         const std::filesystem::path& output_subdir = "decoded_downscaled",
                                         uint tests_failed = 0) {

    if constexpr (DST_WIDTH_PX && DST_HEIGHT_PX) {

        tests_failed += recursive_downscaling_decoding_test<SRC_WIDTH_PX, SRC_HEIGHT_PX, DST_WIDTH_PX - 1, DST_HEIGHT_PX - 1>(
            test_imgs_dir,
            output_subdir,
            downscaling_decoding_test<SRC_WIDTH_PX, SRC_HEIGHT_PX, DST_WIDTH_PX, DST_HEIGHT_PX>(
                test_imgs_dir,
                output_subdir)
        );
    }

    return tests_failed;
}
