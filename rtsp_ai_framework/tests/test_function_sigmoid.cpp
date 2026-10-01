#include <gtest/gtest.h>
#include "processing/image_processor.hpp"

using namespace rtsp_ai;

TEST(SigmoidTest, BaseCases) {
    ImageProcessor::Config config;

    ImageProcessor image_processor(config);

    float actual = image_processor.using_sigmoid(-100.);
    float expected = 0;

    EXPECT_NEAR(actual, expected, 1e-7f) << "Sigmoid lower bound test failed";
    EXPECT_NEAR(image_processor.using_sigmoid(100.), 1, 1e-7f) << "Sigmoid upper bound test failed";
    EXPECT_NEAR(image_processor.using_sigmoid(.0), .5, 1e-7f) << "Sigmoid center position test failed";
}
