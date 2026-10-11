#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "core/frame_data.hpp"

using namespace rtsp_ai;

/**
 * Test DetectionResult validity
 */
TEST(DetectionResultTest, ValidityCheck) {
    DetectionResult det1;
    EXPECT_FALSE(det1.is_valid()); // Default values are invalid
    
    DetectionResult det2;
    det2.class_id = 1;
    det2.confidence = 0.9f;
    det2.box = cv::Rect(0, 0, 100, 100);
    EXPECT_TRUE(det2.is_valid());
}

/**
 * Test DetectionResult properties
 */
TEST(DetectionResultTest, Properties) {
    DetectionResult det;
    det.class_id = 0;
    det.confidence = 0.85f;
    det.class_name = "person";

    // Test set/get properties
    det.set_property("track_id", 42.0f);
    EXPECT_EQ(det.get_property("track_id"), 42.0f);

    det.set_property("confidence_alt", 0.9f);
    EXPECT_EQ(det.get_property("confidence_alt"), 0.9f);

    // Test default value for non-existent property
    EXPECT_EQ(det.get_property("non_existent", -1.0f), -1.0f);
}

/**
 * Test FrameData validity
 */
TEST(FrameDataTest, ValidityCheck) {
    FrameData frame;
    EXPECT_FALSE(frame.is_valid()); // Empty frame is invalid

    frame.frame_id = 1;
    frame.raw_frame = cv::Mat(480, 640, CV_8UC3);
    EXPECT_TRUE(frame.is_valid());
}

/**
 * Test FrameData size calculation
 */
TEST(FrameDataTest, SizeCalculation) {
    FrameData frame;
    frame.raw_frame = cv::Mat(480, 640, CV_8UC3);
    frame.processed_blob = cv::Mat(640, 640, CV_32F);

    // Add detections
    for (int i = 0; i < 5; ++i) {
        DetectionResult det;
        det.class_id = i;
        det.confidence = 0.9f;
        frame.detections.push_back(det);
    }

    size_t size = frame.get_size_bytes();
    EXPECT_GT(size, 0);

    // Verify rough calculation
    // raw_frame: 480 * 640 * 3 = 921,600 bytes
    // processed_blob: 640 * 640 * 4 =1,638,400 bytes
    // detections: 5 * sizeof(DetectionResult)
    size_t expected_min = 480 * 640 * 3 + 640 * 640 * 4;
    EXPECT_GE(size, expected_min);
}

/**
 * Test FrameData metadata
 */
TEST(FrameDataTest, Metadata) {
    FrameData frame;

    // String metadata
    frame.set_metadata("source", "rtsp://example.com");
    EXPECT_EQ(frame.get_metadata("source"), "rtsp://example.com");
    EXPECT_EQ(frame.get_metadata("non_existent", "default"), "default");

    // Float metadata
    frame.set_metadata("confidence_threshold", 0.5f);
    EXPECT_EQ(frame.get_metadata_float("confidence_threshold"), 0.5f);

    // Int metadata
    frame.set_metadata("frame_number", 42);
    EXPECT_EQ(frame.get_metadata_int("frame_number"), 42);
}

/**
 * Test latency calculation
 */
TEST(FrameDataTest, LatencyCalculation) {
    FrameData frame;
    frame.capture_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(100);

    auto latency = frame.get_latency();
    EXPECT_GE(latency.count(), 95);     // Allow 5ms tolerance
    EXPECT_LE(latency.count(), 150);
}

/**
 * Test processing timing
 */
TEST(FrameDataTest, ProcessingTiming) {
    FrameData frame;

    // Record preprocess timing
    frame.record_preprocess_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    frame.record_preprocess_end();

    EXPECT_GE(frame.metrics.capture_to_preprocess_ms.count(), 40);
    EXPECT_LE(frame.metrics.capture_to_preprocess_ms.count(), 100);

    // Record inference timing
    frame.record_inference_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    frame.record_inference_end();

    EXPECT_GE(frame.metrics.preprocess_to_inference_ms.count(), 20);
    EXPECT_LE(frame.metrics.preprocess_to_inference_ms.count(), 100);

    // Record postprocess timing
    frame.record_postprocess_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    frame.record_postprocess_end();

    EXPECT_GE(frame.metrics.inference_to_postprocess_ms.count(), 10);
    EXPECT_LE(frame.metrics.inference_to_postprocess_ms.count(), 100);

    // Total should be sum of all three
    auto total = frame.metrics.capture_to_preprocess_ms.count() +
                 frame.metrics.preprocess_to_inference_ms.count() +
                 frame.metrics.inference_to_postprocess_ms.count();

    EXPECT_EQ(frame.metrics.total_processing_ms.count(), total);
}

/**
 * Test frame status transitions
 */
TEST(FrameDataTest, StatusTransition) {
    FrameData frame;

    EXPECT_EQ(frame.status, FrameData::FrameStatus::ACQUIRED);

    frame.status = FrameData::FrameStatus::PREPROCESSING;
    EXPECT_EQ(frame.status, FrameData::FrameStatus::PREPROCESSING);

    frame.status = FrameData::FrameStatus::INFERENCE;
    EXPECT_EQ(frame.status, FrameData::FrameStatus::INFERENCE);

    frame.status = FrameData::FrameStatus::POSTPROCESSING;
    EXPECT_EQ(frame.status, FrameData::FrameStatus::POSTPROCESSING);

    frame.status = FrameData::FrameStatus::READY;
    EXPECT_EQ(frame.status, FrameData::FrameStatus::READY);
}

/**
 * Test detection results container
 */
TEST(FrameDataTest, DetectionContainer) {
    FrameData frame;

    // Add multiple detections
    for (int i = 0; i < 5; ++i) {
        DetectionResult det;
        det.class_id = i;
        det.confidence = 0.7f + (i * 0.05f);
        det.box = cv::Rect(i * 10, i * 10, 50, 50);
        det.class_name = "object_" + std::to_string(i);
        frame.detections.push_back(det);
    }

    EXPECT_EQ(frame.detections.size(), 5);

    // Verify each detection
    for (size_t i = 0; i < frame.detections.size(); ++i) {
        EXPECT_EQ(frame.detections[i].class_id, static_cast<int>(i));
        EXPECT_GT(frame.detections[i].confidence, 0.69f);
    }
}

/**
 * Test source_id for multi-camera scenarios
 */
TEST(FrameDataTest, MultiSourceSupport) {
    FrameData frame1;
    frame1.source_id = 0;
    frame1.frame_id = 1;

    FrameData frame2;
    frame2.source_id = 1;
    frame2.frame_id = 1;

    EXPECT_EQ(frame1.source_id, 0);
    EXPECT_EQ(frame2.source_id, 1);
    EXPECT_EQ(frame1.frame_id, frame2.frame_id); // Both source 0 frame 1
}

/**
 * Test frame copy and move semantics
 */
TEST(FrameDataTest, CopySemantics) {
    FrameData frame1;
    frame1.frame_id = 42;
    frame1.raw_frame = cv::Mat(480, 640, CV_8UC3);
    frame1.set_metadata("test", "value");

    // Copy constructor (implicit)
    FrameData frame2 = frame1;

    EXPECT_EQ(frame2.frame_id, 42);
    EXPECT_EQ(frame2.get_metadata("test"), "value");
    EXPECT_EQ(frame2.raw_frame.rows, 480);
}

/**
 * Test metadata overflow handling
 */
TEST(FrameDataTest, MetadataOverflow) {
    FrameData frame;

    // Add many metadata entries
    for (int i = 0; i < 100; ++i) {
        frame.set_metadata("key_" + std::to_string(i), "value_" + std::to_string(i));
        frame.set_metadata("float_" + std::to_string(i), static_cast<float>(i) * 1.5f);
        frame.set_metadata("int_" + std::to_string(i), i);
    }

    // Verify they're all retrievable
    EXPECT_EQ(frame.get_metadata("key_0"), "value_0");
    EXPECT_EQ(frame.get_metadata("key_99"), "value_99");
    EXPECT_EQ(frame.get_metadata_int("int_50"), 50);
}
