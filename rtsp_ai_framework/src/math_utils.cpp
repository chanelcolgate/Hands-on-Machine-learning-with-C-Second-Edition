// rtsp_ai_framework/src/math_utils.cpp

#include <unsupported/Eigen/CXX11/Tensor>

#include "core/math_utils.hpp"
#include "core/definitions.hpp"

namespace rtsp_ai {

Matrix Conv_2D (const Matrix &input, const Matrix &kernel) {
    const int kernel_rows = kernel.rows();
    const int kernel_cols = kernel.cols();

    const int rows = (input.rows() - kernel_rows) + 1;
    const int cols = (input.cols() - kernel_cols) + 1;

    Matrix result = Matrix::Zero(rows, cols);

    for (int i = 0; i <rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            double sum = input.block(i, j, kernel_rows, kernel_cols).cwiseProduct(kernel).sum();
            result(i, j) = sum;
        }
    }

    return result;
}

Matrix Conv_2D_v2 (const Matrix &input, const Matrix &kernel, int padding) {
    const int input_rows = input.rows();
    const int input_cols = input.cols();

    const int kernel_rows = kernel.rows();
    const int kernel_cols = kernel.cols();

    if (input_rows < kernel_rows) throw std::invalid_argument("The input has less rows than the kernel");
    if (input_cols < kernel_cols) throw std::invalid_argument("The input has less columnes than the kernel");

    const int rows = input_rows - kernel_rows + 2*padding + 1;
    const int cols = input_cols - kernel_cols + 2*padding + 1;

    Matrix result = Matrix::Zero(rows, cols);

    auto fit_dims = [&padding](int pos, int k, int length) {
        int input = pos - padding;
        int kernel = 0;
        int size = k;

        if (input < 0) {
            kernel = -input;
            size += input;
            input = 0;
        }

        if (input + size > length) {
            size = length - input;
        }

        return std::make_tuple(input, kernel, size);
    };

    for (int i = 0; i < rows; ++i) {
        const auto [input_i, kernel_i, size_i] = fit_dims(i, kernel_rows, input_rows);

        for (int j = 0; size_i > 0 && j < cols; ++j) {
            const auto [input_j, kernel_j, size_j] = fit_dims(j, kernel_cols, input_cols);

            if (size_j > 0) {
                auto input_tile = input.block(input_i, input_j, size_i, size_j);
                auto input_kernel = kernel.block(kernel_i, kernel_j, size_i, size_j);
                result(i, j) = input_tile.cwiseProduct(input_kernel).sum();
            }
        }
    }

    return result;
}

float sigmoid (float z) {
    float result;
    if (z >= 45.f)
        result = 1.f;
    else if (z <= -45.f)
        result = 0.f; 
    else
        result = 1.f / (1.f + exp(-z));
    return result;
}
} // namespace rtsp_ai
