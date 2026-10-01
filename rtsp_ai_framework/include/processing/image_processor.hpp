#pragma once

#include <algorithm>    // std::for_each
#include <functional>   // std::less, std::less_equal
#include <iostream>     // std::cout
#include <numeric>
#include <vector>
#include <chrono>
#include <cmath>
#include <random>
#include <stdexcept>
#include <cstddef>

#include <Eigen/Dense>
#include <opencv2/opencv.hpp>

#include "core/base_component.hpp"

namespace rtsp_ai {

using VectorType = std::vector<double>;

/**
 * Edge detection method enumeration (moved outside for C++17 compatibility)
 */
enum class EdgeDetectionMethod {
    FUNCTIONAL, // Using functional programming
    LAMBDA,
    MATRIX,
};

/**
 * Image Processing Component using Eigen
 */
class ImageProcessor: public BaseComponent {
public:
    struct Config {
        // Processing flags
        bool enable_functional{false};
        bool enable_lambda{false};
        bool enable_matrix{false};

        // Edge detection settings
        EdgeDetectionMethod functional_method{EdgeDetectionMethod::FUNCTIONAL};
        EdgeDetectionMethod lambda_method{EdgeDetectionMethod::LAMBDA};
        EdgeDetectionMethod matrix_method{EdgeDetectionMethod::MATRIX};
    };

    /**
     * Constructor
     */
    ImageProcessor(
        const Config& config
    ) : config_(config) {}

    ~ImageProcessor() override = default;

    std::string get_name() const override {
        return "ImageProcessor";
    }

    /**
     * Process a single frame
     */
    bool process_frame() {
        try {
            if (config_.enable_functional) {
                apply_edge_detection(config_.functional_method);
            }

            if (config_.enable_lambda) {
                apply_edge_detection(config_.lambda_method);
            }

            if (config_.enable_matrix) {
                apply_edge_detection(config_.matrix_method);
            }
        return true;
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name() << "] Error processing frame " 
                      << std::endl;
            return false;
        }    
    }

    /**
     * Using Functional
     */
    static void using_functional() {
        std::vector<std::function<bool(double, double)>> comparators {
            std::less<double>(),
            std::less_equal<double>(),
            std::greater<double>(),
            std::greater_equal<double>()
        };

        double x = 10.;
        double y = 10.;
        auto compare = [&x, &y]
            (const std::function<bool(double, double)> &comparator) {
                bool b = comparator(x, y);
                std::cout << (b?"TRUE": "FALSE") << std::endl;
        };

        std::for_each(comparators.begin(), comparators.end(), compare);
    }

    /**
     * Using Lambda
     */
    static void using_lambda() {
        auto L2 = [](const VectorType &vec) {
            return std::inner_product(vec.begin(), vec.end(), vec.begin(), 0.0);
        };

        VectorType weights{1., 2., 3., 4., 5., 6.};

        std::cout << "L2(weight) = " << L2(weights) << std::endl;

        // Data members of classes (but not structs) additionally have trailing underscores.
        auto momentum_optimizer = [vec_ = VectorType{}]
            (const VectorType &gradient) mutable {
            if (vec_.empty()) {
                vec_.resize(gradient.size());
            } 
            std::transform(vec_.begin(), vec_.end(), gradient.begin(), vec_.begin(), [](double v, double dx) {
                double beta = 0.3;
                return beta * v + dx;
            });
            return vec_;
        };

        auto print = [](double d) { std::cout << d << " "; };

        const VectorType current_grads{1., 0., 1., 1., 0., 1.};
        for (int i = 0; i < 3; ++i) {
            VectorType weight_udpate = momentum_optimizer(current_grads);
            std::for_each(weight_udpate.begin(), weight_udpate.end(), print);
            std::cout << std::endl;
        }
    }

    /**
     * Common matrix operations
     */
    static void using_matrix() {
        Eigen::MatrixXd A(2, 2);
        A(0, 0) = 2.;
        A(1, 0) = -2.;
        A(0, 1) = 3.;
        A(1, 1) = 1.;

        Eigen::MatrixXd B(2, 3);
        B(0, 0) = 1.;
        B(1, 0) = 1.;
        B(0, 1) = 2.;
        B(1, 1) = 2.;
        B(0, 2) = -1;
        B(1, 2) = 1.;

        Eigen::MatrixXd C = A * B;

        std::cout << "A:\n" << A << std::endl;
        std::cout << "B:\n" << B << std::endl;
        std::cout << "C:\n" << C << std::endl;

        Eigen::MatrixXd D = B.cwiseProduct(C);
        std::cout << "coeficient-wise multiplication of B & C is:\n" << D << std::endl;

        Eigen::MatrixXd E = B + C;
        std::cout << "The sum of B & C is:\n" << E << std::endl;
        std::cout << "The transpose of B is:\n" << B.transpose() << std::endl;
        std::cout << "The A inverse is:\n" << A.inverse() << std::endl;
        std::cout << "The determinant of A is:\n" << A.determinant() << std::endl;
        std::cout << "Example of unary operation:\n";
        auto func_X_X = [](double x) { return x * x; };
        std::cout << A.unaryExpr(func_X_X) << std::endl;

        std::cout << "Example of binary operation:\n";
        auto func_X_Y = [](double x, double y) { return x * y; };
        std::cout << B.binaryExpr(C, func_X_Y) << std::endl;
    }

    /**
     * Coding for first unit test
     */
    static float using_sigmoid(float z) {
        float result;
        if (z >= 45.f) {
            result = 1.f;
        } else if (z <= -45.f) {
            result = 0.f;
        } else {
            result = 1.f / (1.f + std::exp(-z));
        }
        return result;
    }

    static int find_middle(std::vector<int> arr) {
        const std::size_t N = arr.size();
        if (N == 0) {
            throw std::invalid_argument("cannot find the middle of an empty vector");
        }

        std::sort(arr.begin(), arr.end());
        int result = arr[N / 2];
        return result;
    } 

    static std::vector<float> glorot_initializer(int fan_in, int fan_out) {
        const std::size_t size = fan_in * fan_out;
        const auto stddev = static_cast<float>(std::sqrt(2. / (fan_in + fan_out)));
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine generator(seed);
        std::normal_distribution<float> distribution(.0, stddev);
        std::vector<float> result(size);
        std::generate(result.begin(), result.end(), [&generator, &distribution]() {
            return distribution(generator);
        });
        return result;
    }


protected:
    void initialize() override {
        std::cout << "[" << get_name() << "] Initialized\n"
                  << std::endl;
    }

    void run() override {
        set_running(true);

        std::cout << "[" << get_name() << "] Image Processing started" << std::endl;

        process_frame();
    }

private:
    /**
     * Apply edge detection based on configured method
     */
    void apply_edge_detection(
        EdgeDetectionMethod method
    ) {
        switch(method) {
            case EdgeDetectionMethod::FUNCTIONAL:
                using_functional();
                break;
            case EdgeDetectionMethod::LAMBDA:
                using_lambda();
                break;
            case EdgeDetectionMethod::MATRIX:
                using_matrix();
                break;
            default:
                std::cerr << "["
                          << get_name()
                          << "] Unknown edge detection method"
                          << std::endl;
        }
    }

    // Members
    Config config_;
};
}
