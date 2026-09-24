#pragma once

#include "core/base_component.hpp"
#include "core/frame_data.hpp"
#include "core/safe_queue.hpp"
#include "drivers/ffmpeg_rtsp_reader.hpp"

#include <opencv2/opencv.hpp>

#include <string>
#include <memory>
#include <atomic>
#include <chrono>

namespace rtsp_ai {

/**
 * RTSP Stream Acquisition Driver
 * Captures video from RTSP source with automatic reconnection
 */
class RtspDriver : public BaseComponent {
public:
    struct Config {
        std::string rtsp_url;

        // Connection settings
        int connection_timeout_sec{10};
        int max_reconnect_attempts{5};
        float reconnect_backoff_multiplier{2.0f}; // Exponential backoff

        // Frame settings
        int target_fps{30};
        bool skip_corrupted_frames{true};

        // OpenCV settings
        int buffer_size{30}; // Internal OpenCV buffer
    };

    /**
     * Constructor
     * @param output_queue Queue for captured frames
     * @param config Configuration parameters
     */
    RtspDriver(
        std::shared_ptr<SafeQueue<FrameData>> output_queue,
        const Config& config
    ) : output_queue_(output_queue),
        config_(config),
        connection_failed_(false),
        current_frame_id_(0) {}

    ~RtspDriver() override {
        stop();
    }

    std::string get_name() const override {
        return "RtspDriver";
    }

    /**
     * Get connection state
     */
    bool is_connected() const {
        return reader_.is_open() && !connection_failed_.load();
    }

    /**
     * Get current FPS
     */
    double get_current_fps() const {
        if (is_connected()) {
            return reader_.get_fps();
        }
        return 0.0;
    }

    /**
     * Get number of frames captured
     */
    uint64_t get_frame_count() const {
        return current_frame_id_;
    }

    /**
     * Get connection statistics
     */
    struct ConnectionStats {
        uint32_t total_reconnect_attempts{0};
        uint32_t successful_reconnects{0};
        uint32_t failed_reconnects{0};
        std::chrono::steady_clock::time_point last_connected_time;
        std::chrono::milliseconds total_downtime_ms{0};
    };

    ConnectionStats get_stats() const {
        return stats_;
    }

protected:
    void initialize() override {
        std::cout << "[" << get_name() << "] Initializing RTSP connection to: "
                  << config_.rtsp_url << std::endl;

        if (!open_connection()) {
            throw std::runtime_error("Failed to open RTSP connection");
        }

        stats_.last_connected_time = std::chrono::steady_clock::now();
    }

    void finalize() override {
        close_connection();
    }

    void run() override {
        set_running(true);

        uint64_t frames_captured = 0;
        uint64_t frames_dropped = 0;
        uint32_t reconnect_count = 0;

        std::chrono::steady_clock::time_point last_frame_time =
            std::chrono::steady_clock::now();

        std::cout << "[" << get_name() << "] Starting frame capture loop" << std::endl;

        while (should_run()) {
            // Check connection status
            if (!is_connected()) {
                std::cerr << "[" << get_name() << "] Connection lost, attempting reconnect..." << std::endl;

                if (reconnect_with_backoff()) {
                    reconnect_count++;
                    stats_.successful_reconnects++;
                    std::cout << "[" << get_name() << "] Reconnected successfully" << std::endl;
                } else {
                    stats_.failed_reconnects++;
                    std::cerr << "[" << get_name() << "] Reconnection failed" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }
            }

            // Capture frame
            cv::Mat frame;
            if (!reader_.read(frame)) {
                connection_failed_.store(true);
                ++frames_dropped;
                continue;
            }

            if (frame.empty()) {
                ++frames_dropped;
                continue;
            }

            // Create FrameData structure
            FrameData frame_data;
            frame_data.frame_id = current_frame_id_++;
            frame_data.source_id = 0; // Single source for now
            frame_data.capture_time = std::chrono::steady_clock::now();
            frame_data.raw_frame = frame.clone();
            frame_data.status = FrameData::FrameStatus::ACQUIRED;

            // Add RTSP metadata
            frame_data.set_metadata("source", config_.rtsp_url);
            frame_data.set_metadata("driver", "RtspDriver");

            // Push to output queue
            output_queue_->push(frame_data);
            frames_captured++;

            // Log progress
            if (frames_captured % 30 == 0) {
                std::cout << "[" << get_name() << "] Captured " << frames_captured
                          << " frames (dropped: " << frames_dropped << ")" << std::endl;
            }

            // FPS control - maintain target frame rate
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_frame_time
            );

            int target_frame_interval_ms = 1000 / config_.target_fps;
            if (elapsed.count() < target_frame_interval_ms) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(target_frame_interval_ms - elapsed.count())
                );
            }

            last_frame_time = std::chrono::steady_clock::now();
        }

        std::cout << "[" << get_name() << "] Capture loop ended" << std::endl;
        std::cout << "[" << get_name() << "] Total frames captured: " << frames_captured << std::endl;
        std::cout << "[" << get_name() << "] Total frames dropped: " << frames_dropped << std::endl;
        std::cout << "[" << get_name() << "] Total reconnect attempts: " << reconnect_count << std::endl;
    }

private:
    /**
     * Open RTSP connection
     */
    bool open_connection() {
        try {
            if (!reader_.open(config_.rtsp_url)) {
                std::cerr << "[" << get_name()
                          << "] Failed to open RTSP stream" << std::endl;
                return false;
            }

            connection_failed_.store(false);

            std::cout << "[" << get_name()
                      << "] RTSP stream opened: "
                      << reader_.width() << "x" 
                      << reader_.height() << " @ "
                      << reader_.get_fps() << " FPS" << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name()
                      << "] Exception opening RTSP: "
                      << e.what() << std::endl;
            return false;
        }
    }

    /**
     * Close RTSP connection
     */
    void close_connection() {
        reader_.close();
        std::cout << "[" << get_name()
                  << "] RTSP connection closed" << std::endl;
    }

    /**
     * Attempt to reconnect with exponential backoff
     */
    bool reconnect_with_backoff() {
        uint32_t attempt = 0;
        float backoff_time = 1.0f; // Start with 1 second

        while (attempt < config_.max_reconnect_attempts && should_run()) {
            attempt++;
            stats_.total_reconnect_attempts++;

            std::cout << "[" << get_name() << "] Reconnect attempt " << attempt
                      << "/" << config_.max_reconnect_attempts
                      << " (waiting " << backoff_time << " seconds)" << std::endl;

            // Wait with backoff
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(backoff_time * 1000))
            );

            close_connection();
            if (open_connection()) {
                return true;
            }

            // Exponential backoff
            backoff_time *= config_.reconnect_backoff_multiplier;
            backoff_time = std::min(backoff_time, 60.0f); // Cap at 60 seconds
        }

        return false;
    }

    FFmpegRtspReader reader_;
    std::shared_ptr<SafeQueue<FrameData>> output_queue_;
    Config config_;

    std::atomic<bool> connection_failed_{false};
    uint64_t current_frame_id_{0};
    ConnectionStats stats_;
};
} // namespace rtsp_ai
