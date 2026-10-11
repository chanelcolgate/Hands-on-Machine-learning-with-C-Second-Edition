#pragma once

#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "core/base_component.hpp"
#include "core/frame_data.hpp"
#include "core/safe_queue.hpp"
#include "processing/postprocessor.hpp"

namespace rtsp_ai {

/**
 * YOLO Object Detector Component
 * 
 * Responsibilities:
 * - Load YOLO model (ONNX format)
 * - Run inference on preprocessed frames
 * - Parse YOLO output format
 * - Apply post-processing (NMS, confidence filtering, scaling)
 * 
 * Input: Preprocessed image blob (float32, normalized, 640x640)
 * Output: Vector of DetectionResult with bounding boxes
 */
class YoloDetector : public BaseComponent {
public:
    struct Config {
        std::string model_path;              // Path to ONNX model file
        std::string backend{"opencv_dnn"};   // Inference backend
        int input_width{640};                // YOLO input width
        int input_height{640};               // YOLO input height
        float confidence_threshold{0.5f};    // Detection confidence threshold
        float nms_iou_threshold{0.45f};      // NMS IoU threshold
        bool use_gpu{false};                 // Use GPU if available
        bool scale_to_original{true};        // Scale detections to original size
    };

    /**
     * Constructor
     * @param input_queue Queue with preprocessed frames
     * @param output_queue Queue for frames with detections
     * @param config Configuration
     */
    YoloDetector(
        std::shared_ptr<SafeQueue<FrameData>> input_queue,
        std::shared_ptr<SafeQueue<FrameData>> output_queue,
        const Config& config
    ) : input_queue_(input_queue),
        output_queue_(output_queue),
        config_(config),
        postprocessor_(PostProcessor::Config{
            config.confidence_threshold,
            config.nms_iou_threshold,
            config.scale_to_original,
            1920, 1080,
            config.input_width, config.input_height
        }) {}

    ~YoloDetector() override {
        stop();
    }

    std::string get_name() const override {
        return "YoloDetector";
    }

    /**
     * Load YOLO model from file
     */
    bool load_model() {
        try {
            std::cout << "[" << get_name() << "] Loading YOLO model: " << config_.model_path << std::endl;

            if (config_.backend == "opencv_dnn") {
                if (config_.model_path.empty()) {
                    std::cerr << "[" << get_name() << "] Empty model path" << std::endl;
                    return false;
                }

                model_ = cv::dnn::readNetFromONNX(config_.model_path);
                if (model_.empty()) {
                    std::cerr << "[" << get_name() << "] Failed to load ONNX model" << std::endl;
                    return false;
                }

                if (config_.use_gpu) {
                    model_.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
                    model_.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
                    std::cout << "[" << get_name() << "] Using CUDA backend" << std::endl;
                } else {
                    model_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
                    model_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
                    std::cout << "[" << get_name() << "] Using CPU backend" << std::endl;
                }

                model_loaded_ = true;
                std::cout << "[" << get_name() << "] Model loaded successfully" << std::endl;
                return true;
            }

            std::cerr << "[" << get_name() << "] Unsupported backend: " << config_.backend << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name() << "] Error loading model: " << e.what() << std::endl;
            return false;
        }
    }

    bool is_model_loaded() const { return model_loaded_; }

    /**
     * Run inference on preprocessed frame
     * 
     * @param input_blob Preprocessed image (float32, normalized, 640x640)
     * @return Vector of DetectionResult with post-processing applied
     */
    std::vector<DetectionResult> infer(const cv::Mat& input_blob) {
        std::vector<DetectionResult> detections;

        if (!model_loaded_) {
            std::cerr << "[" << get_name() << "] Model not loaded" << std::endl;
            return detections;
        }

        try {
            // Create blob from preprocessed image
            cv::Mat blob = cv::dnn::blobFromImage(
                input_blob,
                1.0,
                cv::Size(config_.input_width, config_.input_height),
                cv::Scalar(0, 0, 0),
                true,
                false
            );

            // Set input and run inference
            model_.setInput(blob, "images");

            std::vector<cv::Mat> outputs;
            std::vector<std::string> out_names = model_.getUnconnectedOutLayersNames();
            model_.forward(outputs, out_names);

            // Parse raw YOLO outputs
            if (!outputs.empty()) {
                detections = parse_yolo_outputs(outputs[0]);
            }

            // Apply post-processing: confidence filtering, NMS, scaling
            detections = postprocessor_.process(
                detections,
                cv::Size(1920, 1080),
                cv::Size(config_.input_width, config_.input_height)
            );

            return detections;
        } catch (const std::exception& e) {
            std::cerr << "[" << get_name() << "] Inference error: " << e.what() << std::endl;
            return detections;
        }
    }

    /**
     * Set COCO class names for display
     */
    void set_class_names(const std::vector<std::string>& names) {
        class_names_ = names;
    }

protected:
    void initialize() override {
        std::cout << "[" << get_name() << "] Initializing..." << std::endl;
        if (!load_model()) {
            throw std::runtime_error("Failed to initialize YOLO detector");
        }
    }

    void finalize() override {
        model_ = cv::dnn::Net();
        model_loaded_ = false;
        std::cout << "[" << get_name() << "] Finalized" << std::endl;
    }

    void run() override {
        set_running(true);
        std::cout << "[" << get_name() << "] Running inference loop..." << std::endl;

        uint64_t processed = 0;

        while (should_run()) {
            FrameData frame;
            if (!input_queue_->pop(frame, 1000)) {
                continue;
            }

            if (!frame.is_valid()) {
                continue;
            }

            frame.record_inference_start();
            auto detections = infer(frame.processed_blob);
            frame.detections = detections;
            frame.record_inference_end();
            frame.status = FrameData::FrameStatus::READY;

            output_queue_->push(frame);
            ++processed;

            if (processed % 50 == 0) {
                std::cout << "[" << get_name() << "] Processed " << processed << " frames (" 
                          << frame.detections.size() << " detections in last frame)" << std::endl;
            }
        }

        std::cout << "[" << get_name() << "] Stopped after " << processed << " frames" << std::endl;
    }

private:
    /**
     * Parse YOLO v8 output format
     * 
     * YOLO output shape: [1, N, 84] or [1, 84, N]
     * Each detection: [cx, cy, w, h, obj_confidence, class_0_score, ..., class_79_score]
     * 
     * Returns raw detections (before NMS and filtering)
     */
    std::vector<DetectionResult> parse_yolo_outputs(const cv::Mat& output) {
        std::vector<DetectionResult> result;

        if (output.empty()) {
            return result;
        }

        cv::Mat parsed = output;

        // Handle 3D output tensor
        if (parsed.dims == 3) {
            if (parsed.size[1] == 84 && parsed.size[2] > 0) {
                // Shape [1, 84, N] -> reshape to [N, 84]
                parsed = parsed.reshape(1, parsed.size[2]);
            } else if (parsed.size[2] == 84 && parsed.size[1] > 0) {
                // Shape [1, N, 84] -> reshape to [N, 84]
                parsed = parsed.reshape(1, parsed.size[1]);
            }
        }

        if (parsed.rows == 0 || parsed.cols < 84) {
            return result;
        }

        // Parse each detection
        for (int i = 0; i < parsed.rows; ++i) {
            const float cx = parsed.at<float>(i, 0);
            const float cy = parsed.at<float>(i, 1);
            const float w = parsed.at<float>(i, 2);
            const float h = parsed.at<float>(i, 3);
            const float obj_conf = parsed.at<float>(i, 4);

            // Find class with maximum score
            float max_class_score = 0.0f;
            int class_id = 0;

            for (int c = 5; c < parsed.cols; ++c) {
                const float score = parsed.at<float>(i, c);
                if (score > max_class_score) {
                    max_class_score = score;
                    class_id = c - 5;
                }
            }

            // Combined confidence = objectness * class_score
            const float conf = obj_conf * max_class_score;

            // Create detection (confidence filtering done in post-processor)
            DetectionResult det;
            det.class_id = class_id;
            det.confidence = conf;
            det.class_name = (class_id < static_cast<int>(class_names_.size()))
                ? class_names_[class_id]
                : "unknown";

            // Convert center coordinates to top-left
            const int x1 = cvRound(cx - w * 0.5f);
            const int y1 = cvRound(cy - h * 0.5f);
            det.box = cv::Rect(x1, y1, cvRound(w), cvRound(h));

            if (det.box.width <= 0 || det.box.height <= 0) {
                continue;
            }

            result.push_back(det);
        }

        return result;
    }

    std::shared_ptr<SafeQueue<FrameData>> input_queue_;
    std::shared_ptr<SafeQueue<FrameData>> output_queue_;
    Config config_;
    PostProcessor postprocessor_;
    cv::dnn::Net model_;
    bool model_loaded_{false};
    std::vector<std::string> class_names_;
};

} // namespace rtsp_ai
