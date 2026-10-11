// rtsp_ai_framework/include/core/math_utils.hpp
#pragma once

#include <iostream>

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include "core/definitions.hpp"

using Matrix = Eigen::MatrixXd;

namespace rtsp_ai {

Matrix Conv_2D (const Matrix &input, const Matrix &kernel);
Matrix Conv_2D_v2 (const Matrix &input, const Matrix &kernel, int padding);
float sigmoid (float z);

template <int _RANK>
Tensor<_RANK> sigmoid_activation(const Tensor<_RANK>& Z) {
    auto result = Z.unaryExpr(std::ref(sigmoid));
};
} // namespace rtsp_ai
