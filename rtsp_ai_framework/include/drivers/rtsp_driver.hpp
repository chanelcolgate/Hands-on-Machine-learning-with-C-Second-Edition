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
#include <cstdlib>
#include <algorithm>

namespace rtsp_ai {

class RtspDriver : public BaseComponent {
public:
    struct Config {
        std::string rtsp_url;
        int connection_timeout_sec{10};
        int max_reconnect_attempts{5};
        float reconnect_backoff_multiplier{2.0f};
        int target_fps{30};
        bool skip_corrupted_frames{true};
        int buffer_size{30};
    };

    RtspDriver(std::shared_ptr<SafeQueue<FrameData>> output_queue,
               const Config& config)
        : output_queue_(output_queue), config_(config),
          connection_failed_(false), current_frame_id_(0) {}

    ~RtspDriver() override { stop(); }

    std::string get_name() const override { return "RtspDriver"; }
    bool is_connected() const { return reader_.is_open() && !connection_failed_.load(); }
    double get_current_fps() const { return is_connected() ? reader_.get_fps() : 0.0; }
    uint64_t get_frame_count() const { return current_frame_id_; }

    struct ConnectionStats {
        uint32_t total_reconnect_attempts{0};
        uint32_t successful_reconnects{0};
        uint32_t failed_reconnects{0};
        std::chrono::steady_clock::time_point last_connected_time;
        std::chrono::milliseconds total_downtime_ms{0};
    };

    ConnectionStats get_stats() const { return stats_; }

protected:
    void initialize() override {
        std::cout << "[" << get_name() << "] Initializing RTSP connection to: "
                  << config_.rtsp_url << std::endl;
#if defined(_WIN32)
        _putenv("OPENCV_FFMPEG_CAPTURE_OPTIONS=rtsp_transport;udp");
#else
        setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;udp", 1);
#endif
        if (!open_connection()) throw std::runtime_error("Failed to open RTSP connection");
        stats_.last_connected_time = std::chrono::steady_clock::now();
    }

    void finalize() override { close_connection(); }

    void run() override {
        set_running(true);
        uint64_t frames_captured = 0;
        uint64_t frames_dropped = 0;
        auto last_frame_time = std::chrono::steady_clock::now();

        std::cout << "[" << get_name() << "] Starting frame capture loop" << std::endl;
        while (should_run()) {
            if (!is_connected()) {
                std::cerr << "[" << get_name() << "] Connection lost, attempting reconnect..." << std::endl;
                if (!reconnect_with_backoff()) {
                    ++stats_.failed_reconnects;
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    continue;
                }
                ++stats_.successful_reconnects;
            }

            cv::Mat frame;
            if (!reader_.read(frame)) {
                // read() can skip a bad UDP packet internally. Reconnect only
                // after a real end-of-stream/read failure, not on one decode error.
                ++frames_dropped;
                if (frames_dropped <= 5 || frames_dropped % 30 == 0) {
                    std::cerr << "[" << get_name() << "] No decoded frame; dropped="
                              << frames_dropped << std::endl;
                }
                continue;
            }
            if (frame.empty()) {
                ++frames_dropped;
                continue;
            }

            FrameData frame_data;
            frame_data.frame_id = current_frame_id_++;
            frame_data.source_id = 0;
            frame_data.capture_time = std::chrono::steady_clock::now();
            frame_data.raw_frame = std::move(frame);
            frame_data.status = FrameData::FrameStatus::ACQUIRED;
            frame_data.set_metadata("source", config_.rtsp_url);
            frame_data.set_metadata("driver", "RtspDriver");
            output_queue_->push(std::move(frame_data));
            ++frames_captured;

            if (frames_captured % 30 == 0) {
                std::cout << "[" << get_name() << "] Captured " << frames_captured
                          << " frames (dropped: " << frames_dropped << ")" << std::endl;
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time);
            int interval = config_.target_fps > 0 ? 1000 / config_.target_fps : 0;
            if (elapsed.count() < interval) {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval - elapsed.count()));
            }
            last_frame_time = std::chrono::steady_clock::now();
        }
    }

private:
    bool open_connection() {
        try {
            if (!reader_.open(config_.rtsp_url)) {
                std::cerr << "[" << get_name() << "] Failed to open RTSP stream" << std::endl;
                return false;
            }
            connection_failed_.store(false);
            std::cout << "[" << get_name() << "] RTSP stream opened: "
                      << reader_.width() << "x" << reader_.height() << " @ "
                      << reader_.get_fps() << " FPS" << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name() << "] Exception opening RTSP: " << e.what() << std::endl;
            return false;
        }
    }

    void close_connection() {
        reader_.close();
        std::cout << "[" << get_name() << "] RTSP connection closed" << std::endl;
    }

    bool reconnect_with_backoff() {
        ++stats_.total_reconnect_attempts;
        close_connection();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return open_connection();
    }

    FFmpegRtspReader reader_;
    std::shared_ptr<SafeQueue<FrameData>> output_queue_;
    Config config_;
    std::atomic<bool> connection_failed_{false};
    uint64_t current_frame_id_{0};
    ConnectionStats stats_;
};
} // namespace rtsp_ai
