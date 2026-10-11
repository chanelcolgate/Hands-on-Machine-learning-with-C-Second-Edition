#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "core/safe_queue.hpp"
#include "core/frame_data.hpp"
#include "processing/preprocessor.hpp"

using namespace rtsp_ai;

/**
 * Test letterbox resizing maintains aspect ratio
 */
TEST(PreprocessorTest, LetterboxResizing) {
    auto input_queue = std::make_shared<SafeQueue<FrameData>>(10);
    auto output_queue = std::make_shared<SafeQueue<FrameData>>(10);

    ImagePreprocessor::Config config;
    config.target_width = 640;
    config.target_height = 640;
    config.resize_method = ImagePreprocessor::Config::ResizeMethod::LETTERBOX;

    ImagePreprocessor preprocessor(input_queue, output_queue, config);

    // Create test frame with different aspect ratio
    FrameData frame;
    frame.frame_id = 1;
    frame.raw_frame = cv::Mat(480, 1280, CV_8UC3, cv::Scalar(100, 150, 200));

    // Process frame
    EXPECT_TRUE(preprocessor.process_frame(frame));
    EXPECT_EQ(frame.processed_blob.cols, 640);
    EXPECT_EQ(frame.processed_blob.rows, 640);
}
