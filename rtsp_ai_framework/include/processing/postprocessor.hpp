#pragma once

#include <algorithm>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "core/frame_data.hpp"

namespace rtsp_ai {

/**
 * Post-Processor for YOLO detections
 * 
 * Responsibilities:
 * - Filter detections by confidence threshold
 * - Apply Non-Maximum Suppression (NMS) to remove overlapping boxes
 * - Scale detections from inference size (640x640) to original image size
 */
class PostProcessor {
public:
    struct Config {
        float confidence_threshold{0.5f};
        float nms_iou_threshold{0.45f};
        bool scale_to_original{true};
        int original_width{1920};
        int original_height{1080};
        int inference_width{640};
        int inference_height{640};
    };

    explicit PostProcessor(const Config& config = Config())
        : config_(config) {}

    /**
     * Process raw YOLO detections
     * 
     * Pipeline:
     * 1. Filter by confidence threshold
     * 2. Apply NMS to remove duplicates
     * 3. Scale to original image size
     */
    std::vector<DetectionResult> process(
        const std::vector<DetectionResult>& raw_detections,
        const cv::Size& original_size = cv::Size(1920, 1080),
        const cv::Size& inference_size = cv::Size(640, 640)
    ) {
        auto filtered = filter_by_confidence(raw_detections);
        auto deduped = apply_nms(filtered);

        if (config_.scale_to_original) {
            cv::Size src = inference_size.width > 0 && inference_size.height > 0
                ? inference_size
                : cv::Size(config_.inference_width, config_.inference_height);

            cv::Size dst = original_size.width > 0 && original_size.height > 0
                ? original_size
                : cv::Size(config_.original_width, config_.original_height);

            scale_to_original(deduped, src, dst);
        }

        return deduped;
    }

    /**
     * Filter detections by confidence threshold
     */
    std::vector<DetectionResult> filter_by_confidence(
        const std::vector<DetectionResult>& detections
    ) const {
        std::vector<DetectionResult> result;
        result.reserve(detections.size());

        for (const auto& det : detections) {
            if (!det.is_valid()) continue;
            if (det.confidence >= config_.confidence_threshold) {
                result.push_back(det);
            }
        }
        return result;
    }

    /**
     * Non-Maximum Suppression (NMS)
     * 
     * Removes overlapping bounding boxes, keeping only the one with highest confidence
     * 
     * Algorithm:
     * 1. Sort detections by confidence (descending)
     * 2. For each detection:
     *    - Keep it if IoU with any kept detection < threshold
     *    - Otherwise, mark as suppressed
     */
    std::vector<DetectionResult> apply_nms(
        const std::vector<DetectionResult>& detections
    ) const {
        if (detections.empty()) {
            return {};
        }

        std::vector<DetectionResult> sorted = detections;
        std::sort(sorted.begin(), sorted.end(),
                  [](const DetectionResult& a, const DetectionResult& b) {
                      return a.confidence > b.confidence;
                  });

        std::vector<bool> suppressed(sorted.size(), false);
        std::vector<DetectionResult> keep;
        keep.reserve(sorted.size());

        for (size_t i = 0; i < sorted.size(); ++i) {
            if (suppressed[i]) continue;

            keep.push_back(sorted[i]);

            for (size_t j = i + 1; j < sorted.size(); ++j) {
                if (suppressed[j]) continue;

                float iou = compute_iou(sorted[i].box, sorted[j].box);
                if (iou >= config_.nms_iou_threshold) {
                    suppressed[j] = true;
                }
            }
        }

        return keep;
    }

    /**
     * Compute Intersection over Union (IoU) between two bounding boxes
     */
    static float compute_iou(const cv::Rect& a, const cv::Rect& b) {
        int inter_x1 = std::max(a.x, b.x);
        int inter_y1 = std::max(a.y, b.y);
        int inter_x2 = std::min(a.x + a.width, b.x + b.width);
        int inter_y2 = std::min(a.y + a.height, b.y + b.height);

        if (inter_x2 <= inter_x1 || inter_y2 <= inter_y1) {
            return 0.0f;
        }

        float inter_w = static_cast<float>(inter_x2 - inter_x1);
        float inter_h = static_cast<float>(inter_y2 - inter_y1);
        float inter_area = inter_w * inter_h;

        float area_a = static_cast<float>(a.width * a.height);
        float area_b = static_cast<float>(b.width * b.height);
        float union_area = area_a + area_b - inter_area;

        if (union_area <= 0.0f) {
            return 0.0f;
        }

        return inter_area / union_area;
    }

    /**
     * Scale detections from inference size to original image size
     */
    static void scale_to_original(
        std::vector<DetectionResult>& detections,
        const cv::Size& inference_size,
        const cv::Size& original_size
    ) {
        if (inference_size.width <= 0 || inference_size.height <= 0 ||
            original_size.width <= 0 || original_size.height <= 0) {
            return;
        }

        const float scale_x = static_cast<float>(original_size.width) /
                              static_cast<float>(inference_size.width);

        const float scale_y = static_cast<float>(original_size.height) /
                              static_cast<float>(inference_size.height);

        for (auto& det : detections) {
            const float x_center = det.box.x + det.box.width * 0.5f;
            const float y_center = det.box.y + det.box.height * 0.5f;

            const float new_w = det.box.width * scale_x;
            const float new_h = det.box.height * scale_y;

            const float new_x = x_center * scale_x - new_w * 0.5f;
            const float new_y = y_center * scale_y - new_h * 0.5f;

            det.box.x = cvRound(new_x);
            det.box.y = cvRound(new_y);
            det.box.width = cvRound(new_w);
            det.box.height = cvRound(new_h);

            if (det.box.width <= 0) det.box.width = 1;
            if (det.box.height <= 0) det.box.height = 1;
        }
    }

    Config& get_config() { return config_; }
    const Config& get_config() const { return config_; }

private:
    Config config_;
};

} // namespace rtsp_ai
