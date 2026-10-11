#pragma once

#include "core/base_component.hpp"
#include "core/frame_data.hpp"
#include "core/safe_queue.hpp"
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

namespace rtsp_ai {

/**
 * Image preprocessing component
 * Converts raw frames to normalized blobs suitable for AI inference
 */
class ImagePreprocessor : public BaseComponent {
public:
    struct Config {
        // Target size for inference model
        int target_width{640};
        int target_height{640};

        // Normalization settings
        bool normalize{true};
        std::array<float, 3> mean_values{{0.485f, 0.456f, 0.406f}};
        std::array<float, 3> std_values{{0.229f, 0.224f, 0.225f}};

        // Color space
        bool convert_bgr_to_rgb{true};

        // Preprocessing method
        enum class ResizeMethod {
            LETTERBOX,  // Keep aspect ratio with padding
            DIRECT      // Direct resize (may distort)
        };
        ResizeMethod resize_method{ResizeMethod::LETTERBOX};

        // Blob format
        bool use_float32{true};
    };

    /**
     * Constructor
     * @param input_queue Queue containing raw frames
     * @param output_queue Queue for preprocessed frames
     * @param config Configuration parameters
     */
    ImagePreprocessor(
        std::shared_ptr<SafeQueue<FrameData>> input_queue,
        std::shared_ptr<SafeQueue<FrameData>> output_queue,
        const Config& config
    ) : input_queue_(input_queue),
        output_queue_(output_queue),
        config_(config) {}

    ~ImagePreprocessor() override = default;

    std::string get_name() const override {
        return "ImagePreprocessor";
    }

    /**
     * Process a single frame
     */
    bool process_frame(FrameData& frame) {
        if (!frame.is_valid()) {
            std::cerr << "[Preprocessor] Invalid frame: " << frame.frame_id << std::endl;
            return false;
        }

        try {
            frame.record_preprocess_start();
            frame.status = FrameData::FrameStatus::PREPROCESSING;

            // Step 1: Resize with letterbox
            cv::Mat resized = resize_letterbox(frame.raw_frame);

            // Step 2: Convert color space if needed
            if (config_.convert_bgr_to_rgb) {
                cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
            }

            // Step 3: Normalize and convert to blob
            cv::Mat blob = normalize(resized);

            frame.processed_blob = blob;
            frame.record_preprocess_end();
            frame.status = FrameData::FrameStatus::PREPROCESSING;

            return true;
        } catch (const std::exception& e) {
            std::cerr << "[Preprocessor] Error processing frame " << frame.frame_id
                      << ": " << e.what() << std::endl;
            frame.status = FrameData::FrameStatus::FAILED;
            return false;
        }
    }

    std::string get_config_info() const {
        std::string info;
        info += "Target Size: " + std::to_string(config_.target_width) + "x" +
                std::to_string(config_.target_height) + "\n";
        info += "Normalize: " + std::string(config_.normalize ? "Yes" : "No") + "\n";
        info += "Convert BGR To RGB: " + std::string(config_.convert_bgr_to_rgb ? "Yes" : "No") + "\n";
        info += "Resize Method: " + std::string(
            config_.resize_method == Config::ResizeMethod::LETTERBOX ? "Letterbox" : "Direct"
        ) + "\n";
        return info;
    }

protected:
    void run() override {
        set_running(true);

        std::cout << "[" << get_name() << "] Processing started\n"
                  << get_config_info() << std::endl;

        uint64_t processed_count = 0;

        while (should_run()) {
            FrameData frame;

            // Wait for input with 1000ms timeout
            if (!input_queue_->pop(frame, 1000)) {
                // Timeout - just continue
                continue;
            }

            // Process the frame
            if (process_frame(frame)) {
                // Push to output queue
                output_queue_->push(frame);
                processed_count++;

                // Log progress every 30 frames
                if (processed_count % 30 == 0) {
                    std::cout << "[" << get_name() << "] Processed " << processed_count
                              << " frames" << std::endl;
                }
            }
        }

        std::cout << "[" << get_name() << "] Total processed: " << processed_count << std::endl;
    }

private:
    /**
     * Resize frame maintaining aspect ratio (letterbox method)
     * Adds black padding to preserve aspect ratio
     */
    cv::Mat resize_letterbox(const cv::Mat& src){
        cv::Mat dst(config_.target_height, config_.target_width, src.type());

        // Calculate scale to fit image in target size
        float scale = std::min(
            static_cast<float>(config_.target_width) / src.cols,
            static_cast<float>(config_.target_height) / src.rows
        );

        // Calculate new dimensions
        int new_width = static_cast<int>(src.cols * scale);
        int new_height = static_cast<int>(src.rows * scale);

        // Resize image
        cv::Mat resized;
        cv::resize(src, resized, cv::Size(new_width, new_height), 0, 0, cv::INTER_LINEAR);

        // Calculate padding
        int pad_left = (config_.target_width - new_width) / 2;
        int pad_top = (config_.target_height - new_height) / 2;

        // Place resized image on black background
        dst.setTo(cv::Scalar(114, 114, 114)); // Gray padding
        resized.copyTo(dst(cv::Rect(pad_left, pad_top, new_width, new_height)));

        return dst;
    }

    /**
     * Normalize image to blob format suitable for inference
     * Applies channel-wise normalization and converts to float if needed
     */
    cv::Mat normalize(const cv::Mat& src) {
        cv::Mat blob;

        // Convert to float32 if needed
        if (config_.use_float32) {
            src.convertTo(blob, CV_32F, 1.0 / 255.0); // Normalize to 0-1
        } else {
            blob = src.clone();
        }

        if (config_.normalize) {
            // Apply channel-wise normalization
            std::vector<cv::Mat> channels;
            cv::split(blob, channels);

            for (size_t i = 0; i < channels.size() && i < 3; ++i) {
                channels[i] -= config_.mean_values[i];
                channels[i] /= config_.std_values[i];
            }

            cv::merge(channels, blob);
        }

        return blob;
    }

    std::shared_ptr<SafeQueue<FrameData>> input_queue_;
    std::shared_ptr<SafeQueue<FrameData>> output_queue_;
    Config config_;
};
} // namespace rtsp_ai
