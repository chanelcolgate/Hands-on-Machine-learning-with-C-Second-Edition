#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>

// Include pipeline components
#include "env/pipeline_env.hpp"

// Global flag for graceful shutdown
std::atomic<bool> keep_running{true};

/**
 * Signal handler for SIGINT (Ctrl+C)
 */
void signal_handler(int signal_num) {
    if (signal_num == SIGINT) {
        std::cout << "\n[Main] Received SIGINT, initiating graceful shutdown..." << std::endl;
        keep_running.store(false);
    }
}

/**
 * Main application entry point
 */
int main(int argc, char** argv) {
    // Register signal handler
    std::signal(SIGINT, signal_handler);

    std::cout << "================================" << std::endl;
    std::cout << "RTSP AI Framework Application" << std::endl;
    std::cout << "================================" << std::endl;

    // Parse command line arguments
    std::string rtsp_url = "rtsp://admin:123456@192.168.1.60:554/0";

    if (argc > 1) {
        rtsp_url = argv[1];
    }

    std::cout << "[Main] RTSP URL: " << rtsp_url << std::endl;

    try {
        // Create pipeline configuration
        rtsp_ai::PipelineEnv::Config pipeline_config;

        // Configure RTSP Driver
        pipeline_config.rtsp_config.rtsp_url = rtsp_url;
        pipeline_config.rtsp_config.connection_timeout_sec = 10;
        pipeline_config.rtsp_config.max_reconnect_attempts = 5;
        pipeline_config.rtsp_config.target_fps = 30;

        // Configure Preprocessor
        pipeline_config.preprocessor_config.target_width = 640;
        pipeline_config.preprocessor_config.target_height = 640;
        pipeline_config.preprocessor_config.normalize = true;
        pipeline_config.preprocessor_config.convert_bgr_to_rgb = true;

        // Configure Queues
        pipeline_config.queue_max_size = 30;

        // Create and start pipeline
        std::cout << "\n[Main] Initializing pipeline..." << std::endl;
        rtsp_ai::PipelineEnv pipeline(pipeline_config);

        std::cout << "[Main] Starting pipeline..." << std::endl;
        pipeline.start_pipeline();

        std::cout << "[Main] Pipeline running. Press Ctrl+C to terminate." << std::endl;

        // Main loop - pull and display frames
        uint64_t frame_count = 0;

        while (keep_running.load()) {
            // Try to get a processed frame with 1000ms timeout
            auto frame_opt = pipeline.get_processed_frame(1000);

            if (frame_opt) {
                auto& frame = frame_opt.value();
                frame_count++;

                // Log frame processing every 30 frames
                if (frame_count % 30 == 0) {
                    std::cout << "[Main] Processed frame #" << frame.frame_id
                              << " | Latency: " << frame.get_latency().count() << " ms"
                              << " | Processing: " << frame.metrics.total_processing_ms.count() << " ms"
                              << " | Detections: " << frame.detections.size() << std::endl;
                }

                // Here you would typicaly:
                // 1. Run inference on processed blob
                // 2. Perform post-processing (NMS, etc.)
                // 3. Display/output results
            } else {
                // No frame available within timeout - continue waiting
                if (frame_count % 10 == 0 && frame_count > 0) {
                    std::cout << "[Main] Waiting for frames...( received "
                              << frame_count << " so far)" << std::endl;
                }
            }

            // Check pipeline health every 5 seconds
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

        std::cout << "\n[Main] Total frames processed: " << frame_count << std::endl;
        std::cout << "[Main] Application terminated successfully" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[Main] FATAL ERROR: " << e.what() << std::endl;
        std::cerr << "[Main] Application terminating due to exception" << std::endl;
        return 1;
    }
}
