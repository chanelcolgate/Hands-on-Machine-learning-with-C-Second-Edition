#pragma once

#include <string>
#include <exception>
#include <iomanip>

#include <unsupported/Eigen/CXX11/Tensor>

using TYPE = float;

using Tensor_0D = Eigen::Tensor<TYPE, 0>;
using Tensor_1D = Eigen::Tensor<TYPE, 1>;
using Tensor_2D = Eigen::Tensor<TYPE, 2>;
using Tensor_3D = Eigen::Tensor<TYPE, 3>;
using Tensor_4D = Eigen::Tensor<TYPE, 4>;

template<int _RANK>
using Tensor = Eigen::Tensor<TYPE, _RANK>;

template<int _RANK>
using DimArray = Eigen::array<Eigen::DenseIndex, _RANK>;
