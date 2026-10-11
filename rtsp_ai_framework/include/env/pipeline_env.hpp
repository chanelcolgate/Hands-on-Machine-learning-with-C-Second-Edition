#pragma once

#include "core/safe_queue.hpp"
#include "drivers/rtsp_driver.hpp"
#include "publishers/display_monitor.hpp"
#include "processing/preprocessor.hpp"
#include "processing/image_processor.hpp"
#include "processing/postprocessor.hpp"
#include "inference/yolo_detector.hpp"

#include <memory>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <stdexcept>

namespace rtsp_ai {

/**
 * Complete RTSP AI Pipeline Environment
 * 
 * Orchestrates all components in a multi-threaded pipeline:
 * 
 * RTSP Stream → RtspDriver → [raw frames]
 *                    ↓
 *            ImagePreprocessor → [preprocessed blobs]
 *                    ↓
 *            ImageProcessor → [processed frames]
 *                    ↓
 *            YoloDetector → [frames with detections]
 *                    ↓
 *            DisplayMonitor → [visualization]
 */
class PipelineEnv {
public:
    struct Config {
        RtspDriver::Config rtsp_config;
        ImagePreprocessor::Config preprocessor_config;
        DisplayMonitor::Config monitor_config;
        ImageProcessor::Config image_processor_config;
        YoloDetector::Config yolo_config;

        size_t queue_max_size{30};
        bool monitor_enabled{false};
        bool yolo_enabled{false};
    };

    explicit PipelineEnv(const Config& config)
        : config_(config), started_(false) {
        std::cout << "[PipelineEnv] Initializing RTSP AI Framework Pipeline" << std::endl;

        // Create queues for inter-component communication
        raw_frame_queue_ = std::make_shared<SafeQueue<FrameData>>(config_.queue_max_size);
        processed_frame_queue_ = std::make_shared<SafeQueue<FrameData>>(config_.queue_max_size);
        processed_image_queue_ = std::make_shared<SafeQueue<FrameData>>(config_.queue_max_size);
        inference_queue_ = std::make_shared<SafeQueue<FrameData>>(config_.queue_max_size);

        // Create components
        driver_ = std::make_unique<RtspDriver>(raw_frame_queue_, config_.rtsp_config);
        preprocessor_ = std::make_unique<ImagePreprocessor>(
            raw_frame_queue_, processed_frame_queue_, config_.preprocessor_config);

        image_processor_ = std::make_unique<ImageProcessor>(
            processed_frame_queue_, processed_image_queue_, config_.image_processor_config);

        // YOLO detector (optional)
        if (config_.yolo_enabled) {
            detector_ = std::make_unique<YoloDetector>(
                processed_image_queue_, inference_queue_, config_.yolo_config);
        }

        // Display monitor (optional)
        if (config_.monitor_enabled) {
            // Use inference queue if YOLO is enabled, otherwise use processed image queue
            auto display_queue = config_.yolo_enabled ? inference_queue_ : processed_image_queue_;
            monitor_ = std::make_unique<DisplayMonitor>(display_queue, config_.monitor_config);
        }
    }

    ~PipelineEnv() { stop_pipeline(); }

    PipelineEnv(const PipelineEnv&) = delete;
    PipelineEnv& operator=(const PipelineEnv&) = delete;

    /**
     * Start all pipeline components
     * Components are started in order: driver → preprocessor → image_processor → detector → monitor
     */
    void start_pipeline() {
        if (started_) return;

        try {
            start_time_ = std::chrono::steady_clock::now();

            std::cout << "[PipelineEnv] Starting components..." << std::endl;

            // Start RTSP driver
            driver_->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!driver_->is_running()) {
                throw std::runtime_error("RTSP driver failed to start");
            }
            std::cout << "[PipelineEnv] ✓ RTSP driver started" << std::endl;

            // Start image preprocessor
            preprocessor_->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!preprocessor_->is_running()) {
                throw std::runtime_error("Image preprocessor failed to start");
            }
            std::cout << "[PipelineEnv] ✓ Image preprocessor started" << std::endl;

            // Start image processor
            image_processor_->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!image_processor_->is_running()) {
                throw std::runtime_error("Image processor failed to start");
            }
            std::cout << "[PipelineEnv] ✓ Image processor started" << std::endl;

            // Start YOLO detector (optional)
            if (detector_) {
                detector_->start();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!detector_->is_running()) {
                    throw std::runtime_error("YOLO detector failed to start");
                }
                std::cout << "[PipelineEnv] ✓ YOLO detector started" << std::endl;
            }

            // Start display monitor (optional)
            if (monitor_) {
                monitor_->start();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!monitor_->is_running()) {
                    throw std::runtime_error("Display monitor failed to start");
                }
                std::cout << "[PipelineEnv] ✓ Display monitor started" << std::endl;
            }

            started_ = true;
            std::cout << "\n[PipelineEnv] ========================================" << std::endl;
            std::cout << "[PipelineEnv] Pipeline started successfully" << std::endl;
            std::cout << "[PipelineEnv] ========================================" << std::endl << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[PipelineEnv] Failed to start pipeline: " << e.what() << std::endl;
            stop_components();
            throw;
        }
    }

    /**
     * Stop all pipeline components gracefully
     * Components are stopped in reverse order: monitor → detector → image_processor → preprocessor → driver
     */
    void stop_pipeline() {
        if (!started_) return;

        std::cout << "\n[PipelineEnv] Stopping pipeline..." << std::endl;
        stop_components();
        started_ = false;
        std::cout << "[PipelineEnv] Pipeline stopped" << std::endl;
        print_statistics();
    }

    /**
     * Check if pipeline is running
     */
    bool is_running() const {
        return started_ &&
               driver_->is_running() &&
               preprocessor_->is_running() &&
               image_processor_->is_running() &&
               (!detector_ || detector_->is_running()) &&
               (!monitor_ || monitor_->is_running());
    }

    /**
     * Get processed frame from inference queue (when not using display monitor)
     */
    std::optional<FrameData> get_processed_frame(uint32_t timeout_ms = 100) {
        FrameData frame;
        auto queue = config_.yolo_enabled ? inference_queue_ : processed_frame_queue_;
        if (queue->pop(frame, timeout_ms)) {
            return frame;
        }
        return std::nullopt;
    }

    // Accessor methods
    std::shared_ptr<SafeQueue<FrameData>> get_raw_queue() { return raw_frame_queue_; }
    std::shared_ptr<SafeQueue<FrameData>> get_processed_queue() { return processed_frame_queue_; }
    std::shared_ptr<SafeQueue<FrameData>> get_inference_queue() { return inference_queue_; }

    RtspDriver* get_driver() { return driver_.get(); }
    ImagePreprocessor* get_preprocessor() { return preprocessor_.get(); }
    ImageProcessor* get_image_processor() { return image_processor_.get(); }
    YoloDetector* get_detector() { return detector_.get(); }
    DisplayMonitor* get_monitor() { return monitor_.get(); }

private:
    void stop_components() {
        if (monitor_) {
            monitor_->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (detector_) {
            detector_->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (image_processor_) {
            image_processor_->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (preprocessor_) {
            preprocessor_->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (driver_) {
            driver_->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void print_statistics() const {
        auto uptime = std::chrono::steady_clock::now() - start_time_;
        std::cout << "\n============================================================" << std::endl
                  << "PIPELINE STATISTICS" << std::endl
                  << "============================================================" << std::endl
                  << "Uptime: "
                  << std::chrono::duration_cast<std::chrono::seconds>(uptime).count()
                  << " seconds" << std::endl
                  << "Frames captured: " << driver_->get_frame_count() << std::endl
                  << "Raw queue peak: " << raw_frame_queue_->get_metrics().peak_queue_size << std::endl
                  << "Processed queue peak: " << processed_frame_queue_->get_metrics().peak_queue_size << std::endl;

        if (detector_) {
            std::cout << "Inference queue peak: " << inference_queue_->get_metrics().peak_queue_size << std::endl;
        }

        if (monitor_) {
            std::cout << "Display FPS: " << std::fixed << std::setprecision(1)
                      << monitor_->get_display_fps() << std::endl;
        }

        std::cout << "============================================================" << std::endl << std::endl;
    }

    Config config_;
    bool started_{false};
    std::chrono::steady_clock::time_point start_time_;

    // Queues
    std::shared_ptr<SafeQueue<FrameData>> raw_frame_queue_;
    std::shared_ptr<SafeQueue<FrameData>> processed_frame_queue_;
    std::shared_ptr<SafeQueue<FrameData>> processed_image_queue_;
    std::shared_ptr<SafeQueue<FrameData>> inference_queue_;

    // Components
    std::unique_ptr<RtspDriver> driver_;
    std::unique_ptr<ImagePreprocessor> preprocessor_;
    std::unique_ptr<ImageProcessor> image_processor_;
    std::unique_ptr<YoloDetector> detector_;
    std::unique_ptr<DisplayMonitor> monitor_;
};

} // namespace rtsp_ai
