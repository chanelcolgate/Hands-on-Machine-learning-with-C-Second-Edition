#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <iomanip>

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
    throw std::invalid_argument("Value must be true or false");
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  RTSP AI Framework - YOLO Object Detection Pipeline" << std::endl;
    std::cout << std::string(60, '=') << std::endl << std::endl;

    // Default configuration
    std::string rtsp_url = "rtsp://admin:123456@192.168.1.60:554/0";
    std::string model_path = "models/yolov8n.onnx";
    bool monitor_enabled = false;
    bool yolo_enabled = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: rtsp_ai_app [OPTIONS]\n" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --rtsp=<url>            RTSP stream URL (default: rtsp://admin:123456@192.168.1.60:554/0)" << std::endl;
            std::cout << "  --model=<path>          Path to YOLO ONNX model (default: models/yolov8n.onnx)" << std::endl;
            std::cout << "  --monitor               Enable display monitor (default: disabled)" << std::endl;
            std::cout << "  --yolo                  Enable YOLO detector (default: disabled)" << std::endl;
            std::cout << "  --help                  Show this help message" << std::endl;
            return 0;
        }
        else if (arg.rfind("--rtsp=", 0) == 0) {
            rtsp_url = arg.substr(7);
        }
        else if (arg.rfind("--model=", 0) == 0) {
            model_path = arg.substr(8);
        }
        else if (arg == "--monitor") {
            monitor_enabled = true;
        }
        else if (arg == "--yolo") {
            yolo_enabled = true;
        }
        else if (arg.rfind("--", 0) != 0) {
            // Assume it's RTSP URL if no dash prefix
            rtsp_url = arg;
        }
    }

    std::cout << "[Main] Configuration:" << std::endl;
    std::cout << "  RTSP URL: " << rtsp_url << std::endl;
    std::cout << "  YOLO Model: " << model_path << std::endl;
    std::cout << "  Monitor: " << (monitor_enabled ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  YOLO: " << (yolo_enabled ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << std::endl;

    try {
        // Build pipeline configuration
        rtsp_ai::PipelineEnv::Config pipeline_config;

        // RTSP driver configuration
        pipeline_config.rtsp_config.rtsp_url = rtsp_url;
        pipeline_config.rtsp_config.connection_timeout_sec = 10;
        pipeline_config.rtsp_config.max_reconnect_attempts = 5;
        pipeline_config.rtsp_config.target_fps = 30;
        pipeline_config.rtsp_config.buffer_size = 30;

        // Image preprocessor configuration
        pipeline_config.preprocessor_config.target_width = 640;
        pipeline_config.preprocessor_config.target_height = 640;
        pipeline_config.preprocessor_config.normalize = true;
        pipeline_config.preprocessor_config.convert_bgr_to_rgb = true;
        pipeline_config.preprocessor_config.resize_method =
            rtsp_ai::ImagePreprocessor::Config::ResizeMethod::LETTERBOX;

        // Image processor configuration
        pipeline_config.image_processor_config.enable_functional = false;
        pipeline_config.image_processor_config.enable_lambda = false;
        pipeline_config.image_processor_config.enable_matrix = false;
        pipeline_config.image_processor_config.enable_opencv = false;

        // YOLO detector configuration
        pipeline_config.yolo_config.model_path = model_path;
        pipeline_config.yolo_config.backend = "opencv_dnn";
        pipeline_config.yolo_config.input_width = 640;
        pipeline_config.yolo_config.input_height = 640;
        pipeline_config.yolo_config.confidence_threshold = 0.5f;
        pipeline_config.yolo_config.nms_iou_threshold = 0.45f;
        pipeline_config.yolo_config.use_gpu = false;
        pipeline_config.yolo_config.scale_to_original = true;

        // Display monitor configuration
        pipeline_config.monitor_config.window_title = "RTSP AI Stream - YOLO Object Detection";
        pipeline_config.monitor_config.display_width = 1280;
        pipeline_config.monitor_config.display_height = 720;
        pipeline_config.monitor_config.fps_display = 30;
        pipeline_config.monitor_config.show_fps = true;
        pipeline_config.monitor_config.show_latency = true;
        pipeline_config.monitor_config.show_timestamp = true;
        pipeline_config.monitor_config.show_frame_id = true;
        pipeline_config.monitor_config.show_resolution = true;

        // Enable components
        pipeline_config.queue_max_size = 30;
        pipeline_config.monitor_enabled = monitor_enabled;
        pipeline_config.yolo_enabled = yolo_enabled;

        // Create and start pipeline
        std::cout << "[Main] Creating pipeline..." << std::endl;
        rtsp_ai::PipelineEnv pipeline(pipeline_config);

        std::cout << "[Main] Starting pipeline..." << std::endl;
        pipeline.start_pipeline();

        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Verify pipeline is running
        if (!pipeline.is_running()) {
            throw std::runtime_error("Pipeline failed to start correctly");
        }

        std::cout << "[Main] Pipeline running. Press Ctrl+C to terminate." << std::endl;
        std::cout << std::string(60, '-') << std::endl << std::endl;

        uint64_t frame_count = 0;
        auto last_stats = std::chrono::steady_clock::now();

        // Main loop
        while (keep_running.load()) {
            if (monitor_enabled) {
                // When display monitor is enabled, it consumes the queue
                // Just monitor health and print periodic statistics
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                frame_count = pipeline.get_driver()->get_frame_count();
            } else {
                // Without monitor, we can consume frames here
                auto frame_opt = pipeline.get_processed_frame(1000);
                if (frame_opt) {
                    ++frame_count;
                    const auto& frame = frame_opt.value();

                    if (frame_count % 30 == 0) {
                        std::cout << "[Main] Frame #" << std::setfill(' ') << std::setw(6) << frame.frame_id
                                  << " | Detections: " << std::setw(3) << frame.detections.size()
                                  << " | Latency: " << std::setfill(' ') << std::setw(4) << frame.get_latency().count() << "ms";

                        if (yolo_enabled) {
                            std::cout << " | Inference: "
                                      << std::setfill(' ') << std::setw(3)
                                      << frame.metrics.preprocess_to_inference_ms.count() << "ms";
                        }
                        std::cout << std::endl;
                    }
                }
            }

            // Periodic health check
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_stats).count() >= 10) {
                if (!pipeline.is_running()) {
                    std::cerr << "[Main] WARNING: Pipeline is not running!" << std::endl;
                    keep_running.store(false);
                }
                last_stats = now;
            }
        }

        std::cout << std::string(60, '-') << std::endl << std::endl;
        std::cout << "[Main] Shutting down pipeline..." << std::endl;
        pipeline.stop_pipeline();

        std::cout << "[Main] Total frames processed: " << frame_count << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "[Main] Application terminated successfully" << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n" << std::string(60, '=') << std::endl;
        std::cerr << "[Main] FATAL ERROR" << std::endl;
        std::cerr << std::string(60, '=') << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << std::string(60, '=') << std::endl << std::endl;
        return 1;
    }
}
