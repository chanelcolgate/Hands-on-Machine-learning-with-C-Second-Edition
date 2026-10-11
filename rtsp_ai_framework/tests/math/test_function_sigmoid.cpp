#include <gtest/gtest.h>
#include "processing/image_processor.hpp"
#include "core/safe_queue.hpp"
#include "core/frame_data.hpp"

using namespace rtsp_ai;

TEST(SigmoidTest, BaseCases) {
    ImageProcessor::Config config;
    auto input_queue = std::make_shared<SafeQueue<FrameData>>(10);
    auto output_queue = std::make_shared<SafeQueue<FrameData>>(10);

    ImageProcessor image_processor(input_queue, output_queue, config);

    float actual = image_processor.using_sigmoid(-100.);
    float expected = 0;

    EXPECT_NEAR(actual, expected, 1e-7f) << "Sigmoid lower bound test failed";
    EXPECT_NEAR(image_processor.using_sigmoid(100.), 1, 1e-7f) << "Sigmoid upper bound test failed";
    EXPECT_NEAR(image_processor.using_sigmoid(.0), .5, 1e-7f) << "Sigmoid center position test failed";
}
