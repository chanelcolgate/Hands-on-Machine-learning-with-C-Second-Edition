#include "env/pipeline_env.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

std::atomic<bool> keep_running{true};

void signal_handler(int) {
    keep_running = false;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);

    std::string rtsp_url = "rtsp://admin:123456@192.168.1.60:554/0";

    std::cout << "[INFO] Starting RTSP AI Pipeline Framework..." << std::endl;
    // PipelineEnv env(rtsp_url);

    // env.start_pipeline();
    // std::cout << "[INFO] Pipeline running. Press Ctrl+C to terminate." << std::endl;

    // while (keep_running) {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // }

    // std::cout << "[INFO] Shutting down Pipeline..." << std::endl;
    // env.stop_pipeline();

    return 0;
}
