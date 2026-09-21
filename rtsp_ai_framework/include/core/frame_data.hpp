#pragma once
#include <chrono>
#include <opencv2/opencv.hpp>
#include <vector>

struct DetectionResult {
  int class_id;
  float confidence;
  cv::Rect box;
};

struct FrameData {
  uint64_t frame_id{0};
  std::chrono::steady_clock::time_point timestamp;

  cv::Mat raw_frame;                       // Du lieu anh tho tu RTSP Stream
  cv::Mat processed_blob;                  // Du lieu sau Preprocess
  std::vector<DetectionResult> detections; // Ket qua suy luan AI
};
