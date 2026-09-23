#pragma once
// #include "drivers/rtsp_driver.hpp"
#include "core/safe_queue.hpp"
#include <memory>
#include <string>

// class PipelineEnv {
// public:
//     explicit PipelineEnv(const std::string& rtsp_url) {
//         // raw_frame_queue_ = std::make_shared<SafeQueue<FrameData>>();
//         // driver_ = std::make_unique<RtspDriver>(rtsp_url, raw_frame_queue_);
//         // Khởi tạo Preprocessor, AI Inferencer, Display Monitor tại đây...
//     }
// 
//     void start_pipeline() {
//         // driver_->start();
//         // Start các components khác...
//     }
// 
//     void stop_pipeline() {
//         // driver_->stop();
//         // Stop các components khác...
//     }
// 
// private:
//     // std::shared_ptr<SafeQueue<FrameData>> raw_frame_queue_;
//     // std::unique_ptr<RtspDriver> driver_;
// };
