#pragma once

#include <chrono>
#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cmath>
#include <iostream>

namespace rtsp_ai {

struct DetectionResult {
    int class_id{-1};
    float confidence{0.0f};
    cv::Rect box;
    std::string class_name;
    std::map<std::string, float> properties;

    bool is_valid() const {
        return class_id >= 0 && confidence > 0.0f &&
               box.width > 0 && box.height > 0;
    }

    float get_property(const std::string& key, float default_val = 0.0f) const {
        auto it = properties.find(key);
        return it != properties.end() ? it->second : default_val;
    }

    void set_property(const std::string& key, float value) {
        properties[key] = value;
    }
};

struct FrameData {
    uint64_t frame_id{0};
    uint32_t source_id{0};
    std::chrono::steady_clock::time_point capture_time;
    std::chrono::steady_clock::time_point process_time;
    cv::Mat raw_frame;
    cv::Mat processed_blob;
    std::vector<DetectionResult> detections;
    std::map<std::string, std::string> metadata_strings;
    std::map<std::string, float> metadata_floats;
    std::map<std::string, int32_t> metadata_ints;

    enum class FrameStatus {
        ACQUIRED = 0, PREPROCESSING, INFERENCE, POSTPROCESSING,
        READY, FAILED
    };
    FrameStatus status{FrameStatus::ACQUIRED};

    struct ProcessingMetrics {
        std::chrono::milliseconds capture_to_preprocess_ms{0};
        std::chrono::milliseconds preprocess_to_inference_ms{0};
        std::chrono::milliseconds inference_to_postprocess_ms{0};
        std::chrono::milliseconds total_processing_ms{0};
    } metrics;

    // Frame ID 0 is valid: the capture driver starts counting at zero.
    bool is_valid() const {
        return !raw_frame.empty() && raw_frame.cols > 0 && raw_frame.rows > 0;
    }

    size_t get_size_bytes() const {
        size_t total = 0;
        if (!raw_frame.empty()) total += raw_frame.total() * raw_frame.elemSize();
        if (!processed_blob.empty()) total += processed_blob.total() * processed_blob.elemSize();
        total += detections.size() * sizeof(DetectionResult);
        return total;
    }

    std::chrono::milliseconds get_latency() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - capture_time);
    }

    void set_metadata(const std::string& key, const std::string& value) {
        metadata_strings[key] = value;
    }
    std::string get_metadata(const std::string& key,
                             const std::string& default_val = "") const {
        auto it = metadata_strings.find(key);
        return it != metadata_strings.end() ? it->second : default_val;
    }
    void set_metadata(const std::string& key, float value) { metadata_floats[key] = value; }
    float get_metadata_float(const std::string& key, float default_val = 0.0f) const {
        auto it = metadata_floats.find(key);
        return it != metadata_floats.end() ? it->second : default_val;
    }
    void set_metadata(const std::string& key, int32_t value) { metadata_ints[key] = value; }
    int32_t get_metadata_int(const std::string& key, int32_t default_val = 0) const {
        auto it = metadata_ints.find(key);
        return it != metadata_ints.end() ? it->second : default_val;
    }

    void record_preprocess_start() { process_time = std::chrono::steady_clock::now(); }
    void record_preprocess_end() {
        metrics.capture_to_preprocess_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - process_time);
    }
    void record_inference_start() { process_time = std::chrono::steady_clock::now(); }
    void record_inference_end() {
        metrics.preprocess_to_inference_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - process_time);
    }
    void record_postprocess_start() { process_time = std::chrono::steady_clock::now(); }
    void record_postprocess_end() {
        metrics.inference_to_postprocess_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - process_time);
        metrics.total_processing_ms = metrics.capture_to_preprocess_ms +
            metrics.preprocess_to_inference_ms + metrics.inference_to_postprocess_ms;
    }

    void print_info() const {
        std::cout << "\n=== Frame Data ===\n"
                  << "Frame ID: " << frame_id << '\n'
                  << "Status: " << static_cast<int>(status) << '\n'
                  << "Raw frame size: " << raw_frame.cols << "x" << raw_frame.rows << '\n'
                  << "Detections: " << detections.size() << '\n';
    }
};
} // namespace rtsp_ai
