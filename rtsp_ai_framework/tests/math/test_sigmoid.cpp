#include <iostream>

#include <gtest/gtest.h>
#include <unsupported/Eigen/CXX11/Tensor>

#include "core/math_utils.hpp"
#include "core/definitions.hpp"

using namespace rtsp_ai;

TEST(SigmoidTest, BasicSigmoid) {
    Tensor_2D A(2, 3);
    A.setRandom();

    std::cout << "A:\n\n" << A << std::endl;

    Tensor_2D B = sigmoid_activation(A);

    std::cout << "B:\n\n" << B << std::endl;
}
