#pragma once

#include "drivers/rtsp_driver.hpp"
#include "processing/preprocessor.hpp"
#include "core/safe_queue.hpp"
#include <memory>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>

namespace rtsp_ai {


/**
 * Pipeline Environment
 * Orchestrates all components: Driver -> Preprocessor -> Output
 * Analogous to uvm_env in SystemVerilog
 */
class PipelineEnv {
public:
    struct Config {
        // RTSP Configuration
        RtspDriver::Config rtsp_config;

        // Preprocessor Configuration
        ImagePreprocessor::Config preprocessor_config;

        // Queue Configuration
        size_t queue_max_size{30};
    };

    /**
     * Constructor
     * @param config Pipeline configuration
     */
    explicit PipelineEnv(const Config& config)
        : config_(config),
          started_(false) {

        std::cout << "[PipelineEnv] Initializing RTSP AI Framework Pipeline" << std::endl;

        // Create queues (analogous to uvm_tlm_fifo)
        raw_frame_queue_ = std::make_shared<SafeQueue<FrameData>>(config_.queue_max_size);
        processed_frame_queue_ = std::make_shared<SafeQueue<FrameData>>(config_.queue_max_size);

        // Create components (analogous to uvm_driver, uvm_monitor, etc.)
        driver_ = std::make_unique<RtspDriver>(raw_frame_queue_, config_.rtsp_config);
        preprocessor_ = std::make_unique<ImagePreprocessor>(
            raw_frame_queue_,
            processed_frame_queue_,
            config_.preprocessor_config
        );

        std::cout << "[PipelineEnv] Pipeline components created successfully" << std::endl;
    }

    ~PipelineEnv() {
        stop_pipeline();
    }

    // Non-copyable
    PipelineEnv(const PipelineEnv&) = delete;
    PipelineEnv& operator=(const PipelineEnv&) = delete;

    /**
     * Start the entire pipeline
     */
    void start_pipeline() {
        if (started_) {
            std::cout << "[PipelineEnv] Pipeline already running" << std::endl;
            return;
        }

        try {
            std::cout << "[PipelineEnv] Starting pipeline components..." << std::endl;

            // Start in order: Driver -> Preprocessor
            // This ensures output queues are ready before upstream data arrives
            start_time_ = std::chrono::steady_clock::now();

            // Start driver first (data source)
            driver_->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Start preprocessor
            preprocessor_->start();

            started_ = true;
            std::cout << "[PipelineEnv] Pipeline started successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[PipelineEnv] Failed to start pipeline: " << e.what() << std::endl;
            stop_pipeline();
            throw;
        }
    }

    /**
     * Stop the entire pipeline
     */
    void stop_pipeline() {
        if (!started_) {
            return;
        }

        std::cout << "[PipelineEnv] Stopping pipeline components..." << std::endl;

        // Stop in reverse order: Preprocessor -> Driver
        try {
            preprocessor_->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            driver_->stop();

            started_ = false;
            std::cout << "[PipelineEnv] Pipeline stopped successfully" << std::endl;

            print_statistics();
        } catch (const std::exception& e) {
            std::cerr << "[PipelineEnv] Error during shutdown: " << e.what() << std::endl;
        }
    }

    /**
     * Check if pipeline is running
     */
    bool is_running() const {
        return started_ && driver_->is_running() && preprocessor_->is_running();
    }

    /**
     * Get processed frame (blocking with timeout)
     */
    std::optional<FrameData> get_processed_frame(uint32_t timeout_ms = 100) {
        FrameData frame;
        if (processed_frame_queue_->pop(frame, timeout_ms)) {
            return frame;
        }
        return std::nullopt;
    }

    /**
     * Get processed frame (non-blocking)
     */
    std::optional<FrameData> try_get_processed_frame() {
        return processed_frame_queue_->try_pop();
    }

    /**
     * Get number of frames in raw queue
     */
    size_t raw_queue_size() const {
        return raw_frame_queue_->size();
    }

    /**
     * Get number of frames in processed queue
     */
    size_t processed_queue_size() const {
        return processed_frame_queue_->size();
    }

    /**
     * Print pipeline statistics
     */
    void print_statistics() const {
        auto uptime = std::chrono::steady_clock::now() - start_time_;
        auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();

        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "PIPELINE STATISTICS" << std::endl;
        std::cout << std::string(60, '=') << std::endl;

        std::cout << "\nUptime: " << uptime_seconds << " seconds" << std::endl;

        // Driver stats
        std::cout << "\n--- RTSP Driver ---" << std::endl;
        std::cout << "Frames captured: " << driver_->get_frame_count() << std::endl;
        std::cout << "Current FPS: " << driver_->get_current_fps() << std::endl;

        auto driver_stats = driver_->get_stats();
        std::cout << "Reconnection attempts: " << driver_stats.total_reconnect_attempts << std::endl;
        std::cout << "Successful reconnects: " << driver_stats.successful_reconnects << std::endl;
        std::cout << "Failed reconnects: " << driver_stats.failed_reconnects << std::endl;

        // Queue stats
        std::cout << "\n--- Queue Statistics ---" << std::endl;
        std::cout << "Raw frame queue size: " << raw_frame_queue_->size() << std::endl;
        std::cout << "Processed frame queue size: " << processed_frame_queue_->size() << std::endl;

        // Queue metrics
        std::cout << "\n--- Raw Frame Queue Metrics ---" << std::endl;
        auto raw_metrics = raw_frame_queue_->get_metrics();
        std::cout << "Total frames: " << raw_metrics.total_frames_pushed << std::endl;
        std::cout << "Dropped: " << raw_metrics.total_frames_dropped << std::endl;
        std::cout << "Peak size: " << raw_metrics.peak_queue_size << std::endl;

        std::cout << "\n--- Processed Frame Queue Metrics ---" << std::endl;
        auto proc_metrics = processed_frame_queue_->get_metrics();
        std::cout << "Total frames: " << proc_metrics.total_frames_pushed << std::endl;
        std::cout << "Dropped: " << proc_metrics.total_frames_dropped << std::endl;
        std::cout << "Peak size: " << proc_metrics.peak_queue_size << std::endl;

        std::cout << std::string(60, '=') << "\n" << std::endl;
    }

    /**
     * Get raw frame queue (for advanced usage)
     */
    std::shared_ptr<SafeQueue<FrameData>> get_raw_queue() {
        return raw_frame_queue_;
    }

    /**
     * Get processed frame queue (for advanced usage)
     */
    std::shared_ptr<SafeQueue<FrameData>> get_processed_queue() {
        return processed_frame_queue_;
    }

    /**
     * Get driver reference
     */
    RtspDriver* get_driver() {
        return driver_.get();
    }

    /**
     * Get preprocessor reference
     */
    ImagePreprocessor* get_preprocessor() {
        return preprocessor_.get();
    }

private:
    // Configuration
    Config config_;

    // State
    bool started_;
    std::chrono::steady_clock::time_point start_time_;

    // Queues (TLM FIFOs)
    std::shared_ptr<SafeQueue<FrameData>> raw_frame_queue_;
    std::shared_ptr<SafeQueue<FrameData>> processed_frame_queue_;

    // Components
    std::unique_ptr<RtspDriver> driver_;
    std::unique_ptr<ImagePreprocessor> preprocessor_;
};
} // namespace rtsp_ai
