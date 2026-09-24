#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include "publishers/display_monitor.hpp"

namespace {

using rtsp_ai::DisplayMonitor;
using rtsp_ai::FrameData;
using rtsp_ai::SafeQueue;

class DisplayMonitorTest: public ::testing::Test {
protected:
    void SetUp() override {
        root_ = std::filesystem::temp_directory_path() /
                ("display_monitor_test_" +
                 std::to_string(std::chrono::steady_clock::now()
                                    .time_since_epoch().count()));
        screenshot_dir_ = root_ / "screenshots";
        video_dir_ = root_ / "videos";
    }

    void TearDown() override {
        std::filesystem::remove_all(root_);
    }

    DisplayMonitor::Config make_config() const {
        DisplayMonitor::Config config;
        config.window_title = "DisplayMonitorTest";
        config.screenshot_dir = screenshot_dir_.string();
        config.video_output_dir = video_dir_.string();
        return config;
    }

    std::filesystem::path root_;
    std::filesystem::path screenshot_dir_;
    std::filesystem::path video_dir_;
};

TEST_F(DisplayMonitorTest, HasExpectedNameAndInitialMetrics) {
    auto queue = std::make_shared<SafeQueue<FrameData>>();
    DisplayMonitor monitor(queue, make_config());

    EXPECT_EQ(monitor.get_name(), "DisplayMonitor");
    EXPECT_DOUBLE_EQ(monitor.get_display_fps(), 0.0);
    EXPECT_EQ(monitor.get_frames_displayed(), 0u);
    EXPECT_EQ(monitor.get_screenshots_count(), 0u);
}
}
