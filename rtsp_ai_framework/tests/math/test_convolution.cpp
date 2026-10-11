// rtsp_ai_framework/tests/test_convolution.cpp
#include <gtest/gtest.h>
#include "core/math_utils.hpp"
#include <iostream>

using namespace rtsp_ai;

TEST(ConvolutionTest, BasicConvolution) {
    Matrix kernel(3, 3);
    kernel <<
        -1, 0, 1,
        -1, 0, 1,
        -1, 0, 1;

    Matrix input(6, 6);
    input << 3, 1, 0, 2, 5, 6,
          4, 2, 1, 1, 4, 7,
          5, 4, 0, 0, 1, 2,
          1, 2, 2, 1, 3, 4,
          6, 3, 1, 0, 5, 2,
          3, 1, 0, 1, 3, 3;

    auto output = Conv_2D(input, kernel);

    std::cout << "Kernel:\n" << kernel << std::endl;
    std::cout << "Input:\n" << input << std::endl;
    std::cout << "Convolution:\n" << output << std::endl;

    ASSERT_EQ(output.rows(), 4);
    ASSERT_EQ(output.cols(), 4);
}

TEST(ConvolutionTest, ConvolutionV2) {
    Matrix input(6, 6);
    input <<
        3, 1, 0, 2, 5, 6,
        4, 2, 1, 1, 4, 7,
        5, 4, 0, 0, 1, 2,
        1, 2, 2, 1, 3, 4,
        6, 3, 1, 0, 5, 2,
        3, 1, 0, 1, 3, 3;

    Matrix kernel3(3, 3);

    kernel3 <<
        -1, 0, 1,
        -1, 0, 1,
        -1, 0, 1;

    auto out = Conv_2D_v2(input, kernel3, 1);

    std::cout << "Kernel:\n" << kernel3 << std::endl;
    std::cout << "Input:\n" << input << std::endl;
    std::cout << "Convolution:\n" << out << std::endl;

    ASSERT_EQ(out.rows(), 6);
    ASSERT_EQ(out.cols(), 6);
}
