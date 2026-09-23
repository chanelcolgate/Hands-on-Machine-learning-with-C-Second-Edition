#pragma once

#include <chrono>
#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cmath>

namespace rtsp_ai {

/**
 * Detection result from AI inference
 */
struct DetectionResult {
    int class_id{-1};
    float confidence{0.0f};
    cv::Rect box;
    std::string class_name;

    // Additional properties for advanced tracking
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

/**
 * Main frame data structure passed through the pipeline
 * Analogous to uvm_sequence_item in SystemVerilog
 */
struct FrameData {

    // Frame identification
    uint64_t frame_id{0};
    uint32_t source_id{0}; // Camera/source identifier

    // Timing information
    std::chrono::steady_clock::time_point capture_time;
    std::chrono::steady_clock::time_point process_time;

    // Image data
    cv::Mat raw_frame;                       // Du lieu anh tho tu RTSP Stream
    cv::Mat processed_blob;                  // Du lieu sau Preprocess

    // Detection results
    std::vector<DetectionResult> detections; // Ket qua suy luan AI

    // Metadata - flexible storage for additional information
    std::map<std::string, std::string> metadata_strings;
    std::map<std::string, float> metadata_floats;
    std::map<std::string, int32_t> metadata_ints;

    // Frame status
    enum class FrameStatus {
        ACQUIRED = 0,   // Just captured from RTSP
        PREPROCESSING,  // Being preprocessed
        INFERENCE,      // Being processed by AI
        POSTPROCESSING, // Post-processing detections
        READY,          // Ready for display/output
        FAILED          // Processing failed
    };
    FrameStatus status{FrameStatus::ACQUIRED};

    // Performance metrics
    struct ProcessingMetrics {
        std::chrono::milliseconds capture_to_preprocess_ms{0};
        std::chrono::milliseconds preprocess_to_inference_ms{0};
        std::chrono::milliseconds inference_to_postprocess_ms{0};
        std::chrono::milliseconds total_processing_ms{0};
    } metrics;

    /**
     * Check if frame data is valid
     */
    bool is_valid() const {
        return frame_id > 0 &&
               !raw_frame.empty() &&
               raw_frame.cols > 0 &&
               raw_frame.rows > 0;
    }

    /**
     * Get total frame size in bytes
     */
    size_t get_size_bytes() const {
        size_t total = 0;
        if (!raw_frame.empty()) {
            total += raw_frame.total() * raw_frame.elemSize();
        }
        if (!processed_blob.empty()) {
            total += processed_blob.total() * processed_blob.elemSize();
        }
        total += detections.size() * sizeof(DetectionResult);
        return total;
    }

    /**
     * Get latency from capture to now
     */
    std::chrono::milliseconds get_latency() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now - capture_time
        );
    }

    /**
     * Set metadata string value
     */
    void set_metadata(const std::string& key, const std::string& value) {
        metadata_strings[key] = value;
    }

    /**
     * Get metadata string value
     */
    std::string get_metadata(const std::string& key,
            const std::string& default_val = "") const {
        auto it = metadata_strings.find(key);
        return it != metadata_strings.end() ? it->second : default_val;
    }

    /**
     * Set metadata float value
     */
    void set_metadata(const std::string& key, float value) {
        metadata_floats[key] = value;
    }

    /**
     * Get metadata float value
     */
    float get_metadata_float(const std::string& key, float default_val = 0.0f) const {
        auto it = metadata_floats.find(key);
        return it != metadata_floats.end() ? it->second : default_val;
    }

    /**
     * Set metadata int value
     */
    void set_metadata(const std::string& key, int32_t value) {
        metadata_ints[key] = value;
    }

    /**
     * Get metadata int value
     */
    int32_t get_metadata_int(const std::string& key, int32_t default_val = 0) const {
        auto it = metadata_ints.find(key);
        return it != metadata_ints.end() ? it->second : default_val;
    }

    /**
     * Record processing timing
     */
    void record_preprocess_start() {
        process_time = std::chrono::steady_clock::now();
    }

    void record_preprocess_end() {
        metrics.capture_to_preprocess_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - process_time
            );
    }

    void record_inference_start() {
        process_time = std::chrono::steady_clock::now();
    }

    void record_inference_end() {
        metrics.preprocess_to_inference_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - process_time
            );
    }

    void record_postprocess_start() {
        process_time = std::chrono::steady_clock::now();
    }

    void record_postprocess_end() {
        metrics.inference_to_postprocess_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - process_time
            );

        // Calculate total
        metrics.total_processing_ms =
            metrics.capture_to_preprocess_ms +
            metrics.preprocess_to_inference_ms +
            metrics.inference_to_postprocess_ms;
    }

    /**
     * Log frame info
     */
    void print_info() const {
        std::cout << "\n=== Frame Data ===" << std::endl;
        std::cout << "Frame ID: " << frame_id << std::endl;
        std::cout << "Status: " << static_cast<int>(status) << std::endl;
        std::cout << "Raw frame size: " << raw_frame.cols << "x" << raw_frame.rows << std::endl;
        std::cout << "Detections: " << detections.size() << std::endl;
        std::cout << "Processing time:" << std::endl;
        std::cout << "  Preprocess: " << metrics.capture_to_preprocess_ms.count() << " ms" << std::endl;
        std::cout << "  Inference: " << metrics.preprocess_to_inference_ms.count() << " ms" << std::endl;
        std::cout << "  Postprocess: " << metrics.inference_to_postprocess_ms.count() << " ms" << std::endl;
        std::cout << "  Total: " << metrics.total_processing_ms.count() << " ms" << std::endl;
        std::cout << "==================\n" << std::endl;
    }
};
} // namespace rtsp_ai
