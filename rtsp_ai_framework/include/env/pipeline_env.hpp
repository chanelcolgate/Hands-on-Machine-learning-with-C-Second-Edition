#pragma once

#include "drivers/rtsp_driver.hpp"
#include "processing/preprocessor.hpp"
#include "core/safe_queue.hpp"
#include "publishers/display_monitor.hpp"

#include <memory>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <stdexcept>

namespace rtsp_ai {

class PipelineEnv {
public:
    struct Config {
        RtspDriver::Config rtsp_config;
        ImagePreprocessor::Config preprocessor_config;
        size_t queue_max_size{30};
        bool monitor_enabled{false};
        DisplayMonitor::Config monitor_config;
    };

    explicit PipelineEnv(const Config& config) : config_(config), started_(false) {
        std::cout << "[PipelineEnv] Initializing RTSP AI Framework Pipeline" << std::endl;
        raw_frame_queue_ = std::make_shared<SafeQueue<FrameData>>(config_.queue_max_size);
        processed_frame_queue_ = std::make_shared<SafeQueue<FrameData>>(config_.queue_max_size);
        driver_ = std::make_unique<RtspDriver>(raw_frame_queue_, config_.rtsp_config);
        preprocessor_ = std::make_unique<ImagePreprocessor>(
            raw_frame_queue_, processed_frame_queue_, config_.preprocessor_config);
        if (config_.monitor_enabled) {
            monitor_ = std::make_unique<DisplayMonitor>(
                processed_frame_queue_, config_.monitor_config);
        }
    }

    ~PipelineEnv() { stop_pipeline(); }
    PipelineEnv(const PipelineEnv&) = delete;
    PipelineEnv& operator=(const PipelineEnv&) = delete;

    void start_pipeline() {
        if (started_) return;

        try {
            start_time_ = std::chrono::steady_clock::now();
            driver_->start();
            if (!driver_->is_running()) {
                throw std::runtime_error("RTSP driver failed to start");
            }

            preprocessor_->start();
            if (!preprocessor_->is_running()) {
                throw std::runtime_error("Image preprocessor failed to start");
            }

            if (monitor_) {
                monitor_->start();
                if (!monitor_->is_running()) {
                    throw std::runtime_error("Display monitor failed to start");
                }
            }

            started_ = true;
            std::cout << "[PipelineEnv] Pipeline started successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[PipelineEnv] Failed to start pipeline: " << e.what() << std::endl;
            stop_components();
            throw;
        }
    }

    void stop_pipeline() {
        if (!started_) return;
        stop_components();
        started_ = false;
        std::cout << "[PipelineEnv] Pipeline stopped successfully" << std::endl;
        print_statistics();
    }

    bool is_running() const {
        return started_ && driver_->is_running() && preprocessor_->is_running() &&
               (!monitor_ || monitor_->is_running());
    }

    std::optional<FrameData> get_processed_frame(uint32_t timeout_ms = 100) {
        FrameData frame;
        if (processed_frame_queue_->pop(frame, timeout_ms)) return frame;
        return std::nullopt;
    }

    std::optional<FrameData> try_get_processed_frame() { return processed_frame_queue_->try_pop(); }
    size_t raw_queue_size() const { return raw_frame_queue_->size(); }
    size_t processed_queue_size() const { return processed_frame_queue_->size(); }

    void print_statistics() const {
        auto uptime = std::chrono::steady_clock::now() - start_time_;
        std::cout << "\n============================================================\n"
                  << "PIPELINE STATISTICS\n============================================================\n"
                  << "Uptime: "
                  << std::chrono::duration_cast<std::chrono::seconds>(uptime).count()
                  << " seconds\n"
                  << "Frames captured: " << driver_->get_frame_count() << "\n"
                  << "Current FPS: " << driver_->get_current_fps() << "\n"
                  << "Raw queue size: " << raw_frame_queue_->size() << "\n"
                  << "Processed queue size: " << processed_frame_queue_->size() << "\n";
    }

    std::shared_ptr<SafeQueue<FrameData>> get_raw_queue() { return raw_frame_queue_; }
    std::shared_ptr<SafeQueue<FrameData>> get_processed_queue() { return processed_frame_queue_; }
    RtspDriver* get_driver() { return driver_.get(); }
    ImagePreprocessor* get_preprocessor() { return preprocessor_.get(); }

private:
    void stop_components() {
        if (monitor_) monitor_->stop();
        if (preprocessor_) preprocessor_->stop();
        if (driver_) driver_->stop();
    }

    Config config_;
    bool started_;
    std::chrono::steady_clock::time_point start_time_;
    std::shared_ptr<SafeQueue<FrameData>> raw_frame_queue_;
    std::shared_ptr<SafeQueue<FrameData>> processed_frame_queue_;
    std::unique_ptr<RtspDriver> driver_;
    std::unique_ptr<ImagePreprocessor> preprocessor_;
    std::unique_ptr<DisplayMonitor> monitor_;
};
} // namespace rtsp_ai
