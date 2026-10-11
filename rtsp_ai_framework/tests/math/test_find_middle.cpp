#include <gtest/gtest.h>
#include "processing/image_processor.hpp"
#include "core/safe_queue.hpp"
#include "core/frame_data.hpp"

using namespace rtsp_ai;

TEST(OddCase, BaseCases) {
    ImageProcessor::Config config;
    auto input_queue = std::make_shared<SafeQueue<FrameData>>(10);
    auto output_queue = std::make_shared<SafeQueue<FrameData>>(10);

    ImageProcessor image_processor(input_queue, output_queue, config);

    std::vector<int> vec{9, 4, 1, 11, 20, -1, 2};
    int actual = image_processor.find_middle(vec);

    EXPECT_EQ(4, actual);
}

TEST(EvenCase, BaseCases) {
    ImageProcessor::Config config;
    auto input_queue = std::make_shared<SafeQueue<FrameData>>(10);
    auto output_queue = std::make_shared<SafeQueue<FrameData>>(10);
    ImageProcessor image_processor(input_queue, output_queue, config);

    std::vector<int> vec{9, 4, 11, 20, -1, 2};
    int actual = image_processor.find_middle(vec);

    EXPECT_EQ(9, actual);
}

TEST(CheckEmptyVector, BaseCases) {
    ImageProcessor::Config config;
    auto input_queue = std::make_shared<SafeQueue<FrameData>>(10);
    auto output_queue = std::make_shared<SafeQueue<FrameData>>(10);
    ImageProcessor image_processor(input_queue, output_queue, config);
    EXPECT_THROW(
        {
            std::vector<int> vec;
            image_processor.find_middle(vec);
        },
        std::invalid_argument
    );
}
