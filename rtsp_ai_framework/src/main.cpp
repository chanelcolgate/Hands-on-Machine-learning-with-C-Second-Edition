#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>
#include <stdexcept>

#include "env/pipeline_env.hpp"

std::atomic<bool> keep_running{true};

void signal_handler(int signal_num) {
    if (signal_num == SIGINT) {
        std::cout << "\n[Main] Received SIGINT, initiating graceful shutdown..." << std::endl;
        keep_running.store(false);
    }
}

bool parse_bool(const std::string& value) {
    if (value == "1" || value == "true" || value == "on" || value == "yes") return true;
    if (value == "0" || value == "false" || value == "off" || value == "no") return false;
    throw std::invalid_argument("monitor must be true or false");
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    std::cout << "================================\nRTSP AI Framework Application\n================================" << std::endl;

    std::string rtsp_url = "rtsp://admin:123456@192.168.1.60:554/0";
    bool monitor_enabled = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--monitor=", 0) == 0) monitor_enabled = parse_bool(arg.substr(10));
        else if (arg == "--monitor") monitor_enabled = true;
        else if (arg == "--no-monitor") monitor_enabled = false;
        else if (arg.rfind("--", 0) != 0) rtsp_url = arg;
    }

    std::cout << "[Main] RTSP URL: " << rtsp_url << std::endl;
    try {
        rtsp_ai::PipelineEnv::Config pipeline_config;
        pipeline_config.rtsp_config.rtsp_url = rtsp_url;
        pipeline_config.rtsp_config.connection_timeout_sec = 10;
        pipeline_config.rtsp_config.max_reconnect_attempts = 5;
        pipeline_config.rtsp_config.target_fps = 30;
        pipeline_config.preprocessor_config.target_width = 640;
        pipeline_config.preprocessor_config.target_height = 640;
        pipeline_config.preprocessor_config.normalize = false;
        pipeline_config.preprocessor_config.convert_bgr_to_rgb = false;
        pipeline_config.queue_max_size = 30;
        pipeline_config.monitor_enabled = monitor_enabled;

        rtsp_ai::PipelineEnv pipeline(pipeline_config);
        std::cout << "\n[Main] Starting pipeline..." << std::endl;
        pipeline.start_pipeline();
        std::cout << "[Main] Pipeline running. Press ESC in the video window or Ctrl+C to terminate." << std::endl;

        uint64_t frame_count = 0;
        while (keep_running.load()) {
            if (monitor_enabled) {
                // DisplayMonitor owns the processed queue in monitor mode.
                // Do not consume the same queue here or frames would randomly
                // go to the logger instead of the OpenCV window.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                frame_count = pipeline.get_driver()->get_frame_count();
            } else {
                auto frame_opt = pipeline.get_processed_frame(1000);
                if (frame_opt) {
                    ++frame_count;
                    if (frame_count % 30 == 0) {
                        const auto& frame = frame_opt.value();
                        std::cout << "[Main] Processed frame #" << frame.frame_id
                                  << " | Latency: " << frame.get_latency().count() << " ms" << std::endl;
                    }
                }
            }

            static auto last_health_check = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_health_check).count() >= 5) {
                if (!pipeline.is_running()) {
                    std::cerr << "[Main] Pipeline is not running!" << std::endl;
                    keep_running.store(false);
                }
                last_health_check = now;
            }
        }

        std::cout << "\n[Main] Shutting down pipeline..." << std::endl;
        pipeline.stop_pipeline();
        std::cout << "[Main] Total frames processed: " << frame_count << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[Main] FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}
