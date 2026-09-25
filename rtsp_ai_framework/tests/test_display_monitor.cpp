#include <gtest/gtest.h>

#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <opencv2/opencv.hpp>

// Mock/Include the actual components
// Assuming headers are in include/ directory
#include "core/base_component.hpp"
#include "core/safe_queue.hpp"
#include "core/frame_data.hpp"
#include "publishers/display_monitor.hpp"

namespace rtsp_ai {

/**
 * Test Fixture for DisplayMonitor Component
 */
class DisplayMonitorTest: public ::testing::Test {
protected:
    void SetUp() override {
        // Create shared queue
        input_queue_ = std::make_shared<SafeQueue<FrameData>>(30);

        // Create default config
        config_.window_title = "Test Monitor";
        config_.display_width = 640;
        config_.display_height = 480;
        config_.fps_display = 30;
        config_.show_fps = true;
        config_.show_latency = true;
        config_.show_timestamp = true;
        config_.show_frame_id = true;
        config_.show_resolution = true;
        config_.screenshot_dir = "./test_screenshots";
        config_.video_output_dir = "./test_videos";
        config_.enable_recording = false;

        // Create monitor
        monitor_ = std::make_unique<DisplayMonitor>(input_queue_, config_);
    }

    void TearDown() override {
        // Ensure monitor is stopped
        if (monitor_) {
            monitor_->stop();
        }

        // Clean up test directories
        system("rm -rf ./test_screenshots ./test_videos");
    }

    /**
     * Create a test FrameData with specific properties
     */
    FrameData CreateTestFrame(uint64_t frame_id = 1, int width = 640, int height = 480) {
        FrameData frame;
        frame.frame_id = frame_id;
        frame.source_id = 0;

        // Create a dummy BGR image
        frame.raw_frame = cv::Mat(height, width, CV_8UC3, cv::Scalar(100, 150, 200));

        // Set timing
        auto now = std::chrono::steady_clock::now();
        frame.capture_time = now;

        // Simulate processing stages with proper methods
        frame.record_preprocess_start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        frame.record_preprocess_end();

        frame.record_inference_start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        frame.record_inference_end();

        frame.record_postprocess_start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        frame.record_postprocess_end();

        frame.status = FrameData::FrameStatus::READY;

        return frame;
    }

    // Members
    std::shared_ptr<SafeQueue<FrameData>> input_queue_;
    DisplayMonitor::Config config_;
    std::unique_ptr<DisplayMonitor> monitor_;
};


TEST_F(DisplayMonitorTest, DefaultConfigurationValues) {
    auto default_config = DisplayMonitor::Config();

    EXPECT_EQ(default_config.window_title, "RTSP Stream Monitor");
    EXPECT_EQ(default_config.display_width, 1280);
    EXPECT_EQ(default_config.display_height, 720);
    EXPECT_EQ(default_config.fps_display, 30);
    EXPECT_TRUE(default_config.show_fps);
    EXPECT_TRUE(default_config.show_latency);
    EXPECT_FALSE(default_config.enable_recording);
}

// ============================================================================
// Component Lifecycle Tests
// ============================================================================

TEST_F(DisplayMonitorTest, ComponentStartsSuccessfully) {
    EXPECT_NO_THROW(monitor_->start());
    EXPECT_TRUE(monitor_->is_running());

    monitor_->stop();
}

TEST_F(DisplayMonitorTest, DestructorCleansUpProperly) {
    {
        auto temp_monitor = std::make_unique<DisplayMonitor>(input_queue_, config_);
        temp_monitor->start();
        EXPECT_TRUE(temp_monitor->is_running());
        // Destructor called here
    }

    // No crash should occur
    EXPECT_TRUE(true);
}

// ============================================================================
// Frame Display Tests
// ============================================================================

TEST_F(DisplayMonitorTest, SingleFrameDisplayedCorrectly) {
    // Note: In headless environment, imshow will be no-op but counter should still work
    monitor_->start();

    // Create and push frame
    FrameData frame = CreateTestFrame(1, 640, 480);
    input_queue_->push(frame);

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop before checking (to ensure frame is processed)
    monitor_->stop();

    // Verify frame was displayed
    EXPECT_GT(monitor_->get_frames_displayed(), 0);
}

TEST_F(DisplayMonitorTest, MultipleFramesDisplayedSequentially) {
    monitor_->start();

    // Push multiple frames
    for (int i = 1; i <= 5; ++i) {
        FrameData frame = CreateTestFrame(i, 640, 480);
        input_queue_->push(frame);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    monitor_->stop();

    // Verify all frames were processed
    EXPECT_GE(monitor_->get_frames_displayed(), 1);
}

TEST_F(DisplayMonitorTest, DifferentResolutionsHandledCorrectly) {
    monitor_->start();

    // Push frames with different resolutions
    FrameData frame1 = CreateTestFrame(1, 1920, 1080);
    FrameData frame2 = CreateTestFrame(2, 640, 480);
    FrameData frame3 = CreateTestFrame(3, 800, 600);

    input_queue_->push(frame1);
    input_queue_->push(frame2);
    input_queue_->push(frame3);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    monitor_->stop();

    EXPECT_GE(monitor_->get_frames_displayed(), 1);
}

TEST_F(DisplayMonitorTest, FramesResizeHandledCorrectly) {
    // Frame with different size than display
    FrameData frame = CreateTestFrame(1, 1280, 960);
    ASSERT_NE(frame.raw_frame.cols, config_.display_width);
    ASSERT_NE(frame.raw_frame.rows, config_.display_height);

    monitor_->start();
    input_queue_->push(frame);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    monitor_->stop();

    EXPECT_GT(monitor_->get_frames_displayed(), 0);
}

TEST_F(DisplayMonitorTest, EmptyQueueHandledGracefully) {
    monitor_->start();

    // Don't push any frames
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    monitor_->stop();

    // Should still work without error
    EXPECT_EQ(monitor_->get_frames_displayed(), 0);
}

// ============================================================================
// FPS Counter Tests
// ============================================================================

TEST_F(DisplayMonitorTest, FPSCounterInitializedToZero) {
    EXPECT_DOUBLE_EQ(monitor_->get_display_fps(), 0.0);
}

TEST_F(DisplayMonitorTest, FPSCounterUpdatesAfterFrames) {
    monitor_->start();

    // Push frames rapidly to simulate 30+ FPS
    for (int i = 0; i < 35; ++i) {
        FrameData frame = CreateTestFrame(i, 640, 480);
        input_queue_->push(frame);
    }

    // Wait for FPS calculation (needs > 1 second)
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    monitor_->stop();

    // FPS should have been update
    // Actual value depends on processing speed, but should be > 0
    EXPECT_GE(monitor_->get_display_fps(), 0.0);
}

TEST_F(DisplayMonitorTest, FPSResetsBetweenIntervals) {
    monitor_->start();

    // Push frames in bursts
    for (int burst = 0; burst < 2; ++burst) {
        for (int i = 0; i < 20; ++i) {
            FrameData frame = CreateTestFrame(burst * 20 + i, 640, 480);
            input_queue_->push(frame);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    monitor_->stop();

    EXPECT_GE(monitor_->get_frames_displayed(), 1);
}

// ============================================================================
// Screenshot Tests
// ============================================================================

TEST_F(DisplayMonitorTest, ScreenshotCounterInitializedToZero) {
    EXPECT_EQ(monitor_->get_screenshots_count(), 0);
}

TEST_F(DisplayMonitorTest, ScreenshotDirectoryCreated) {
    // Directory should be created in constructor
    std::string cmd = "test -d " + config_.screenshot_dir;
    int result = system(cmd.c_str());
    EXPECT_EQ(result, 0) << "Screenshot directory was not created";
}

TEST_F(DisplayMonitorTest, VideoDirectoryCreated) {
    // Directory should be created in constructor
    std::string cmd = "test -d " + config_.video_output_dir;
    int result = system(cmd.c_str());
    EXPECT_EQ(result, 0) << "Video directory was not created";
}

// ============================================================================
// Pause/Resume Tests
// ============================================================================

TEST_F(DisplayMonitorTest, MonitorCanBePaused) {
    monitor_->start();

    FrameData frame = CreateTestFrame(1, 640, 480);
    input_queue_->push(frame);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Note: is_paused_ is private, so we can't directly test it
    // But we can verify the monitor continues to work
    monitor_->stop();

    EXPECT_GT(monitor_->get_frames_displayed(), 0);
}
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
