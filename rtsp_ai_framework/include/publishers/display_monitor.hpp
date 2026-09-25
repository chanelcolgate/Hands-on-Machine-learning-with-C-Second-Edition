#pragma once

#include "core/base_component.hpp"
#include "core/safe_queue.hpp"
#include "core/frame_data.hpp"
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <queue>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace rtsp_ai {

/**
 * Display Monitor Component
 * Visualizes RTSP stream with FPS and latency overlay
 * Handles keyboard events for screenshots and recording
 */
class DisplayMonitor : public BaseComponent {
public:
    struct Config {
        std::string window_title{"RTSP Stream Monitor"};
        int display_width{1280};
        int display_height{720};
        int fps_display{30};

        bool show_fps{true};
        bool show_latency{true};
        bool show_timestamp{true};
        bool show_frame_id{true};
        bool show_resolution{true};

        // Text properties
        int font_face{cv::FONT_HERSHEY_SIMPLEX};
        double font_scale{0.6};
        int font_thickness{1};
        cv::Scalar text_color{0, 255, 0}; // Green (BGR)

        std::string screenshot_dir{"./screenshots"};
        std::string video_output_dir{"./videos"};
        bool enable_recording{false};
    };

    /**
     * Constructor
     * @param input_queue Queue containing frames to display
     * @param config Display configuration
     */
    DisplayMonitor(
        std::shared_ptr<SafeQueue<FrameData>> input_queue,
        const Config& config
    ) : input_queue_(input_queue),
        config_(config),
        is_paused_(false),
        is_recording_(false),
        fps_counter_(0),
        display_fps_(0.0),
        last_fps_update_(std::chrono::steady_clock::now()) {
        create_output_directories();
    }

    ~DisplayMonitor() override {
        stop();
        // Bọc try-catch an toàn tránh lỗi assertion crash của OpenCV GTK backend trên Cygwin
        try {
            if (cv::getWindowProperty(config_.window_title, cv::WND_PROP_VISIBLE) >= 0) {
                cv::destroyWindow(config_.window_title);
            }
        } catch (const cv::Exception& e) {
            // Ignored on teardown
        }
    }

    std::string get_name() const override {
        return "DisplayMonitor";
    }

    /**
     * Override hàm stop() để đánh thức thread đang bị nghẽn trong Queue
     */
    void stop() override {
        if (!should_run()) return;

        // Bật cờ dừng trong BaseComponent
        set_running(false);

        // Đánh thức thread đang bị nghẽn (blocked) trong input_queue_->pop()
        if (input_queue_) {
            input_queue_->push(FrameData{});
        }

        // Gọi stop() của lớp cơ sở để join worker thread
        BaseComponent::stop();
    }

    /**
     * Get current display FPS
     */
    double get_display_fps() const {
        return display_fps_;
    }

    /**
     * Get total frames displayed
     */
    uint64_t get_frames_displayed() const {
        return frames_displayed_;
    }

    /**
     * Get total screenshots taken
     */
    uint32_t get_screenshots_count() const {
        return screenshots_taken_;
    }

protected:
    void initialize() override {
        std::cout << "[" << get_name() << "] Creating display window: "
                  << config_.window_title << std::endl;

        // Create window
        cv::namedWindow(config_.window_title, cv::WINDOW_NORMAL);
        cv::resizeWindow(config_.window_title, config_.display_width, config_.display_height);

        std::cout << "[" << get_name() << "] Display initialized at "
                  << config_.display_width << "x" << config_.display_height << std::endl;
    }

    void finalize() override {
        if (is_recording_) {
            stop_recording();
        }

        try {
            cv::destroyWindow(config_.window_title);
        } catch (const cv::Exception& e) {
            // Ignored
        }
        std::cout << "[" << get_name() << "] Display window closed" << std::endl;
    }

    void run() override {
        set_running(true);

        std::cout << "[" << get_name() << "] Display loop started" << std::endl;

        while (should_run()) {
            if (is_paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                handle_keyboard_events();
                continue;
            }

            // Try to get frame from queue
            FrameData frame;

            if (!input_queue_->pop(frame, 200)) {
                // No frame available, allow handling events and continue
                handle_keyboard_events();
                continue;
            }

            // Display the frame
            display_frame(frame);

            // Handle keyboard events
            handle_keyboard_events();

            // Update FPS counter
            update_fps_counter();
        }

        std::cout << "[" << get_name() << "] Total frames displayed: "
                  << frames_displayed_ << std::endl;
    }

private:
    /**
     * Display frame with overlay
     */
    void display_frame(const FrameData& frame_data) {
        if (frame_data.raw_frame.empty()) return;

        // Prepare frame for display
        cv::Mat display_frame = frame_data.raw_frame.clone();

        // Resize if necessary
        if (display_frame.cols != config_.display_width ||
            display_frame.rows != config_.display_height) {
            cv::resize(display_frame, display_frame,
                    cv::Size(config_.display_width, config_.display_height));
        }

        // Draw text overlays
        draw_text_overlay(display_frame, frame_data);

        // Display
        cv::imshow(config_.window_title, display_frame);

        // Save current rendered frame for screenshot
        last_frame_ = display_frame.clone();
        frames_displayed_++;
        last_displayed_frame_ = frame_data.frame_id;

        // Record if enabled
        if (is_recording_ && video_writer_.isOpened()) {
            video_writer_.write(display_frame);
        }
    }

    /**
     * Draw text overlay on frame
     */
    void draw_text_overlay(cv::Mat& frame, const FrameData& frame_data) {
        int y_offset = 25;
        const int line_spacing = 25;

        // Frame ID
        if (config_.show_frame_id) {
            std::stringstream ss;
            ss << "Frame ID: " << frame_data.frame_id;
            put_text_with_bg(frame, ss.str(), cv::Point(10, y_offset));
            y_offset += line_spacing;
        }

        // FPS
        if (config_.show_fps) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << "FPS: " << display_fps_;
            put_text_with_bg(frame, ss.str(), cv::Point(10, y_offset));
            y_offset += line_spacing;
        }

        // Latency
        if (config_.show_latency) {
            auto latency = frame_data.get_latency();
            std::stringstream ss;
            ss << "Latency: " << latency.count() << " ms";
            put_text_with_bg(frame, ss.str(), cv::Point(10, y_offset));
            y_offset += line_spacing;
        }

        // Timestamp
        if (config_.show_timestamp) {
            auto now = std::time(nullptr);
            auto tm = std::localtime(&now);
            std::stringstream ss;
            ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
            put_text_with_bg(frame, ss.str(), cv::Point(10, y_offset));
            y_offset += line_spacing;
        }

        // Resolution
        if (config_.show_resolution) {
            std::stringstream ss;
            ss << "Resolution: " << frame_data.raw_frame.cols << "x"
               << frame_data.raw_frame.rows;
            put_text_with_bg(frame, ss.str(), cv::Point(10, y_offset));
            y_offset += line_spacing;
        }

        // Recording indicator
        if (is_recording_) {
            put_text_with_bg(frame, "● REC", cv::Point(10, frame.rows - 30),
                             cv::Scalar(0, 0, 255)); // Red
        }

        // Pause indicator
        if (is_paused_) {
            put_text_with_bg(frame, "PAUSED", cv::Point(10, frame.rows - 30));
        }

        // Help text at bottom
        put_text_with_bg(frame,
                         "ESC: Quit | S: Screenshot | R: Record | SPACE: Pause",
                         cv::Point(10, frame.rows - 10),
                         cv::Scalar(200, 200, 200),
                         0.4);
    }

    /**
     * Put text with semi-transparent background
     */
    void put_text_with_bg(cv::Mat& frame,
                          const std::string& text,
                          cv::Point pos,
                          cv::Scalar color = cv::Scalar(0, 255, 0),
                          double scale = -1) {
        if (scale < 0) {
            scale = config_.font_scale;
        }

        // Get text size
        int baseline = 0;
        cv::Size text_size = cv::getTextSize(text, config_.font_face,
                                             scale, config_.font_thickness,
                                             &baseline);

        // Draw background rectangle
        cv::rectangle(frame,
                      cv::Rect(pos.x - 5, pos.y - text_size.height - 5,
                               text_size.width + 10, text_size.height + 10),
                      cv::Scalar(0, 0, 0), -1); // Black background

        // Draw text
        cv::putText(frame, text, cv::Point(pos.x, pos.y),
                    config_.font_face, scale, color,
                    config_.font_thickness, cv::LINE_AA);
    }

    /**
     * Handle keyboard events
     */
    void handle_keyboard_events() {
        int key = cv::waitKey(1) & 0xFF;

        if (key == 27) { // ESC
            std::cout << "[" << get_name() << "] ESC pressed - quitting" << std::endl;
            set_running(false);
        }
        else if (key == 's' || key == 'S') {
            save_screenshot();
        }
        else if (key == 'r' || key == 'R') {
            if (is_recording_) {
                stop_recording();
            } else {
                start_recording();
            }
        }
        else if (key == ' ') {
            is_paused_ = !is_paused_;
            std::string state = is_paused_ ? "PAUSED" : "RESUMED";
            std::cout << "[" << get_name() << "] Display " << state << std::endl;
        }
    }

    /**
     * Save screenshot to file
     */
    void save_screenshot() {
        if (last_frame_.empty()) {
            std::cerr << "[" << get_name() << "] No frame to screenshot" << std::endl;
            return;
        }

        try {
            // Get current timestamp
            auto now = std::time(nullptr);
            auto tm = std::localtime(&now);

            std::stringstream filename;
            filename << config_.screenshot_dir << "/screenshot_"
                     << std::put_time(tm, "%Y%m%d_%H%M%S")
                     << "_frame_" << last_displayed_frame_ << ".png";

            // Save the actual frame image
            if (cv::imwrite(filename.str(), last_frame_)) {
                std::cout << "[" << get_name() << "] Screenshot saved: "
                          << filename.str() << std::endl;
                screenshots_taken_++;
            } else {
                std::cerr << "[" << get_name() << "] Failed to write screenshot to: "
                          << filename.str() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name() << "] Error saving screenshot: "
                      << e.what() << std::endl;
        }
    }

    /**
     * Start video recording
     */
    void start_recording() {
        try {
            auto now = std::time(nullptr);
            auto tm = std::localtime(&now);

            std::stringstream filename;
            filename << config_.video_output_dir << "/video_"
                     << std::put_time(tm, "%Y%m%d_%H%M%S") << ".mp4";

            int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');

            video_writer_.open(filename.str(), fourcc, config_.fps_display,
                               cv::Size(config_.display_width, config_.display_height));

            if (!video_writer_.isOpened()) {
                std::cerr << "[" << get_name() << "] Failed to open video file" << std::endl;
                return;
            }

            is_recording_ = true;
            std::cout << "[" << get_name() << "] Recording started: "
                      << filename.str() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name() << "] Error starting recording: "
                      << e.what() << std::endl;
        }
    }

    /**
     * Stop video recording
     */
    void stop_recording() {
        if (video_writer_.isOpened()) {
            video_writer_.release();
            is_recording_ = false;
            std::cout << "[" << get_name() << "] Recording stopped" << std::endl;
        }
    }

    /**
     * Update FPS counter
     */
    void update_fps_counter() {
        fps_counter_++;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_fps_update_
        );

        if (elapsed.count() >= 1000) {
            display_fps_ = (fps_counter_ * 1000.0) / elapsed.count();
            fps_counter_ = 0;
            last_fps_update_ = now;
        }
    }

    /**
     * Create output directories if they don't exist
     */
    void create_output_directories() {
        try {
            std::string mkdir_cmd = "mkdir -p " + config_.screenshot_dir +
                                    " " + config_.video_output_dir;
            int ret = system(mkdir_cmd.c_str());

            if (ret == 0) {
                std::cout << "[" << get_name() << "] Output directories ready" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name() << "] Warning: Could not create output dirs: "
                      << e.what() << std::endl;
        }
    }

    // Members
    std::shared_ptr<SafeQueue<FrameData>> input_queue_;
    Config config_;

    // Display state
    bool is_paused_;
    bool is_recording_;
    cv::VideoWriter video_writer_;
    cv::Mat last_frame_;

    // Metrics
    uint64_t frames_displayed_{0};
    uint32_t screenshots_taken_{0};
    uint64_t last_displayed_frame_{0};

    // FPS calculation
    uint32_t fps_counter_;
    double display_fps_;
    std::chrono::steady_clock::time_point last_fps_update_;
};

} // namespace rtsp_ai
